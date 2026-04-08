#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _OPENMP
  #include <omp.h>
#else
  static inline double omp_get_wtime(void) { return 0.0; }
  static inline int omp_get_max_threads(void) { return 1; }
#endif

#include "datadef.h"
#include "init.h"

#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)<(y)?(x):(y))

extern int *ileft, *iright;
extern int nprocs, proc;

/* =========================================================
   Computation of tentative velocity field (f, g)

   Important MPI note:
   - For local subdomains, jmax refers to local_jmax.
   - Only the true global top/bottom ranks should impose
     physical y-boundary conditions on g.
   ========================================================= */
void compute_tentative_velocity(float **u, float **v, float **f, float **g,
    char **flag, int imax, int jmax, float del_t, float delx, float dely,
    float gamma, float Re)
{
    int i, j;
    float du2dx, duvdy, duvdx, dv2dy, laplu, laplv;

    /* Compute f */
    for (i = 1; i <= imax - 1; i++) {
        for (j = 1; j <= jmax; j++) {
            /* only if both adjacent cells are fluid cells */
            if ((flag[i][j] & C_F) && (flag[i+1][j] & C_F)) {
                du2dx =
                    ((u[i][j] + u[i+1][j]) * (u[i][j] + u[i+1][j]) +
                     gamma * fabs(u[i][j] + u[i+1][j]) * (u[i][j] - u[i+1][j]) -
                     (u[i-1][j] + u[i][j]) * (u[i-1][j] + u[i][j]) -
                     gamma * fabs(u[i-1][j] + u[i][j]) * (u[i-1][j] - u[i][j]))
                    / (4.0f * delx);

                duvdy =
                    ((v[i][j] + v[i+1][j]) * (u[i][j] + u[i][j+1]) +
                     gamma * fabs(v[i][j] + v[i+1][j]) * (u[i][j] - u[i][j+1]) -
                     (v[i][j-1] + v[i+1][j-1]) * (u[i][j-1] + u[i][j]) -
                     gamma * fabs(v[i][j-1] + v[i+1][j-1]) * (u[i][j-1] - u[i][j]))
                    / (4.0f * dely);

                laplu =
                    (u[i+1][j] - 2.0f * u[i][j] + u[i-1][j]) / (delx * delx) +
                    (u[i][j+1] - 2.0f * u[i][j] + u[i][j-1]) / (dely * dely);

                f[i][j] = u[i][j] + del_t * (laplu / Re - du2dx - duvdy);
            } else {
                f[i][j] = u[i][j];
            }
        }
    }

    /* Compute g */
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax - 1; j++) {
            /* only if both adjacent cells are fluid cells */
            if ((flag[i][j] & C_F) && (flag[i][j+1] & C_F)) {
                duvdx =
                    ((u[i][j] + u[i][j+1]) * (v[i][j] + v[i+1][j]) +
                     gamma * fabs(u[i][j] + u[i][j+1]) * (v[i][j] - v[i+1][j]) -
                     (u[i-1][j] + u[i-1][j+1]) * (v[i-1][j] + v[i][j]) -
                     gamma * fabs(u[i-1][j] + u[i-1][j+1]) * (v[i-1][j] - v[i][j]))
                    / (4.0f * delx);

                dv2dy =
                    ((v[i][j] + v[i][j+1]) * (v[i][j] + v[i][j+1]) +
                     gamma * fabs(v[i][j] + v[i][j+1]) * (v[i][j] - v[i][j+1]) -
                     (v[i][j-1] + v[i][j]) * (v[i][j-1] + v[i][j]) -
                     gamma * fabs(v[i][j-1] + v[i][j]) * (v[i][j-1] - v[i][j]))
                    / (4.0f * dely);

                laplv =
                    (v[i+1][j] - 2.0f * v[i][j] + v[i-1][j]) / (delx * delx) +
                    (v[i][j+1] - 2.0f * v[i][j] + v[i][j-1]) / (dely * dely);

                g[i][j] = v[i][j] + del_t * (laplv / Re - duvdx - dv2dy);
            } else {
                g[i][j] = v[i][j];
            }
        }
    }

    /* f boundaries in x-direction are always global physical boundaries */
    for (j = 1; j <= jmax; j++) {
        f[0][j]    = u[0][j];
        f[imax][j] = u[imax][j];
    }

    /* g boundaries in y-direction are only physical on the true top/bottom ranks */
    for (i = 1; i <= imax; i++) {
        if (proc == 0) {
            g[i][0] = v[i][0];
        }
        if (proc == nprocs - 1) {
            g[i][jmax] = v[i][jmax];
        }
    }
}

/* =========================================================
   Calculate the right hand side of the pressure equation
   ========================================================= */
void compute_rhs(float **f, float **g, float **rhs, char **flag, int imax,
    int jmax, float del_t, float delx, float dely)
{
    int i, j;

    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax; j++) {
            if (flag[i][j] & C_F) {
                rhs[i][j] =
                    ((f[i][j] - f[i-1][j]) / delx +
                     (g[i][j] - g[i][j-1]) / dely) / del_t;
            }
        }
    }
}

/* =========================================================
   Red/Black SOR to solve the poisson equation
   ========================================================= */
int poisson(float **p, float **rhs, char **flag, int imax, int jmax,
    float delx, float dely, float eps, int itermax, float omega,
    float *res, int ifull)
{
    int i, j, iter;
    float add, beta_2, beta_mod;
    float p0 = 0.0f;
    double t_p0 = 0.0, t_rb = 0.0, t_res = 0.0;
    double t0;

    int rb; /* Red-black value */

    float rdx2 = 1.0f / (delx * delx);
    float rdy2 = 1.0f / (dely * dely);
    beta_2 = -omega / (2.0f * (rdx2 + rdy2));

    t0 = omp_get_wtime();

    /* Calculate sum of squares */
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax; j++) {
            if (flag[i][j] & C_F) {
                p0 += p[i][j] * p[i][j];
            }
        }
    }
    t_p0 += omp_get_wtime() - t0;

    p0 = sqrt(p0 / ifull);
    if (p0 < 0.0001f) {
        p0 = 1.0f;
    }

    /* Red/Black SOR iteration */
    for (iter = 0; iter < itermax; iter++) {
        t0 = omp_get_wtime();

        for (rb = 0; rb <= 1; rb++) {
        #pragma omp parallel for collapse(2) schedule(static) private(beta_mod)
            for (i = 1; i <= imax; i++) {
                for (j = 1; j <= jmax; j++) {
                    if ((i + j) % 2 != rb) {
                        continue;
                    }

                    if (flag[i][j] == (C_F | B_NSEW)) {
                        /* five-point star for interior fluid cells */
                        p[i][j] =
                            (1.0f - omega) * p[i][j] -
                            beta_2 * (
                                (p[i+1][j] + p[i-1][j]) * rdx2 +
                                (p[i][j+1] + p[i][j-1]) * rdy2 -
                                rhs[i][j]
                            );
                    } else if (flag[i][j] & C_F) {
                        /* modified star near boundary */
                        beta_mod = -omega / ((eps_E + eps_W) * rdx2 +
                                             (eps_N + eps_S) * rdy2);
                        p[i][j] =
                            (1.0f - omega) * p[i][j] -
                            beta_mod * (
                                (eps_E * p[i+1][j] + eps_W * p[i-1][j]) * rdx2 +
                                (eps_N * p[i][j+1] + eps_S * p[i][j-1]) * rdy2 -
                                rhs[i][j]
                            );
                    }
                }
            }
        }

        t_rb += omp_get_wtime() - t0;

        *res = 0.0f;
        t0 = omp_get_wtime();

        for (i = 1; i <= imax; i++) {
            for (j = 1; j <= jmax; j++) {
                if (flag[i][j] & C_F) {
                    add =
                        (eps_E * (p[i+1][j] - p[i][j]) -
                         eps_W * (p[i][j] - p[i-1][j])) * rdx2 +
                        (eps_N * (p[i][j+1] - p[i][j]) -
                         eps_S * (p[i][j] - p[i][j-1])) * rdy2 -
                        rhs[i][j];
                    *res += add * add;
                }
            }
        }

        t_res += omp_get_wtime() - t0;

        *res = sqrt((*res) / ifull) / p0;

        if (*res < eps) {
            break;
        }
    }

#ifdef _OPENMP
    printf("[poisson] loops: p0=%f rb=%f res=%f iters=%d threads=%d\n",
           t_p0, t_rb, t_res, iter, omp_get_max_threads());
#endif

    return iter;
}

/* =========================================================
   Update the velocity values based on tentative velocities
   and the new pressure matrix

   Important MPI fix:
   - For v, only the last rank should exclude the true global
     bottom boundary row.
   - Interior ranks must still update their local last row.
   ========================================================= */
void update_velocity(float **u, float **v, float **f, float **g, float **p,
    char **flag, int imax, int jmax, float del_t, float delx, float dely)
{
    int i, j;

    /* Update u */
    for (i = 1; i <= imax - 1; i++) {
        for (j = 1; j <= jmax; j++) {
            if ((flag[i][j] & C_F) && (flag[i+1][j] & C_F)) {
                u[i][j] = f[i][j] - (p[i+1][j] - p[i][j]) * del_t / delx;
            }
        }
    }

    /* Update v
       - interior ranks: update through local jmax
       - last rank only: exclude physical bottom boundary row
    */
    {
        int jv_end = (proc == nprocs - 1) ? (jmax - 1) : jmax;

        for (i = 1; i <= imax; i++) {
            for (j = 1; j <= jv_end; j++) {
                if ((flag[i][j] & C_F) && (flag[i][j+1] & C_F)) {
                    v[i][j] = g[i][j] - (p[i][j+1] - p[i][j]) * del_t / dely;
                }
            }
        }
    }
}

/* =========================================================
   Set timestep size satisfying CFL conditions
   ========================================================= */
void set_timestep_interval(float *del_t, int imax, int jmax, float delx,
    float dely, float **u, float **v, float Re, float tau)
{
    int i, j;
    float umax, vmax, deltu, deltv, deltRe;

    if (tau >= 1.0e-10f) {
        umax = 1.0e-10f;
        vmax = 1.0e-10f;

        for (i = 0; i <= imax + 1; i++) {
            for (j = 1; j <= jmax + 1; j++) {
                umax = max(fabs(u[i][j]), umax);
            }
        }

        for (i = 1; i <= imax + 1; i++) {
            for (j = 0; j <= jmax + 1; j++) {
                vmax = max(fabs(v[i][j]), vmax);
            }
        }

        deltu = delx / umax;
        deltv = dely / vmax;
        deltRe = 1.0f / (1.0f / (delx * delx) + 1.0f / (dely * dely)) * Re / 2.0f;

        if (deltu < deltv) {
            *del_t = min(deltu, deltRe);
        } else {
            *del_t = min(deltv, deltRe);
        }

        *del_t = tau * (*del_t);
    }
}