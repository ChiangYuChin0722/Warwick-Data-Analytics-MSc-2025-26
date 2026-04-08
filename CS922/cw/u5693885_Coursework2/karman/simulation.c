#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

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

/* Exchange pressure halo rows for distributed Poisson */
static void exchange_pressure_halo(float **p, int imax, int jmax_local)
{
    int count = imax + 2;
    int i;

    float *send_up   = (float *)malloc(count * sizeof(float));
    float *send_down = (float *)malloc(count * sizeof(float));
    float *recv_up   = (float *)malloc(count * sizeof(float));
    float *recv_down = (float *)malloc(count * sizeof(float));

    if (!send_up || !send_down || !recv_up || !recv_down) {
        fprintf(stderr, "rank %d: exchange_pressure_halo allocation failed\n", proc);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i <= imax + 1; i++) {
        send_up[i]   = p[i][1];
        send_down[i] = p[i][jmax_local];
    }

    {
        int up   = (proc == 0) ? MPI_PROC_NULL : proc - 1;
        int down = (proc == nprocs - 1) ? MPI_PROC_NULL : proc + 1;

        MPI_Sendrecv(send_up, count, MPI_FLOAT, up, 900,
                     recv_down, count, MPI_FLOAT, down, 900,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(send_down, count, MPI_FLOAT, down, 901,
                     recv_up, count, MPI_FLOAT, up, 901,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (i = 0; i <= imax + 1; i++) {
            if (up != MPI_PROC_NULL) {
                p[i][0] = recv_up[i];
            }
            if (down != MPI_PROC_NULL) {
                p[i][jmax_local + 1] = recv_down[i];
            }
        }
    }

    free(send_up);
    free(send_down);
    free(recv_up);
    free(recv_down);
}

/* Computation of tentative velocity field (f, g) */
void compute_tentative_velocity(float **u, float **v, float **f, float **g,
    char **flag, int imax, int jmax, float del_t, float delx, float dely,
    float gamma, float Re)
{
    int i, j;
    float du2dx, duvdy, duvdx, dv2dy, laplu, laplv;

    #pragma omp parallel for collapse(2) private(du2dx,duvdy,laplu) schedule(static)
    for (i = 1; i <= imax - 1; i++) {
        for (j = 1; j <= jmax; j++) {
            /* only if both adjacent cells are fluid cells */
            if ((flag[i][j] & C_F) && (flag[i+1][j] & C_F)) {
                du2dx = ((u[i][j]+u[i+1][j])*(u[i][j]+u[i+1][j])+
                    gamma*fabs(u[i][j]+u[i+1][j])*(u[i][j]-u[i+1][j])-
                    (u[i-1][j]+u[i][j])*(u[i-1][j]+u[i][j])-
                    gamma*fabs(u[i-1][j]+u[i][j])*(u[i-1][j]-u[i][j]))
                    /(4.0f*delx);

                duvdy = ((v[i][j]+v[i+1][j])*(u[i][j]+u[i][j+1])+
                    gamma*fabs(v[i][j]+v[i+1][j])*(u[i][j]-u[i][j+1])-
                    (v[i][j-1]+v[i+1][j-1])*(u[i][j-1]+u[i][j])-
                    gamma*fabs(v[i][j-1]+v[i+1][j-1])*(u[i][j-1]-u[i][j]))
                    /(4.0f*dely);

                laplu = (u[i+1][j]-2.0f*u[i][j]+u[i-1][j])/(delx*delx)+
                    (u[i][j+1]-2.0f*u[i][j]+u[i][j-1])/(dely*dely);

                f[i][j] = u[i][j]+del_t*(laplu/Re-du2dx-duvdy);
            } else {
                f[i][j] = u[i][j];
            }
        }
    }

    #pragma omp parallel for collapse(2) private(duvdx,dv2dy,laplv) schedule(static)
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax - 1; j++) {
            /* only if both adjacent cells are fluid cells */
            if ((flag[i][j] & C_F) && (flag[i][j+1] & C_F)) {
                duvdx = ((u[i][j]+u[i][j+1])*(v[i][j]+v[i+1][j])+
                    gamma*fabs(u[i][j]+u[i][j+1])*(v[i][j]-v[i+1][j])-
                    (u[i-1][j]+u[i-1][j+1])*(v[i-1][j]+v[i][j])-
                    gamma*fabs(u[i-1][j]+u[i-1][j+1])*(v[i-1][j]-v[i][j]))
                    /(4.0f*delx);

                dv2dy = ((v[i][j]+v[i][j+1])*(v[i][j]+v[i][j+1])+
                    gamma*fabs(v[i][j]+v[i][j+1])*(v[i][j]-v[i][j+1])-
                    (v[i][j-1]+v[i][j])*(v[i][j-1]+v[i][j])-
                    gamma*fabs(v[i][j-1]+v[i][j])*(v[i][j-1]-v[i][j]))
                    /(4.0f*dely);

                laplv = (v[i+1][j]-2.0f*v[i][j]+v[i-1][j])/(delx*delx)+
                    (v[i][j+1]-2.0f*v[i][j]+v[i][j-1])/(dely*dely);

                g[i][j] = v[i][j]+del_t*(laplv/Re-duvdx-dv2dy);
            } else {
                g[i][j] = v[i][j];
            }
        }
    }

    /* f at external x-boundaries */
    #pragma omp parallel for schedule(static)
    for (j = 1; j <= jmax; j++) {
        f[0][j]    = u[0][j];
        f[imax][j] = u[imax][j];
    }

    /* g at physical y-boundaries only */
    #pragma omp parallel for schedule(static)
    for (i = 1; i <= imax; i++) {
        if (proc == 0) {
            g[i][0] = v[i][0];
        }
        if (proc == nprocs - 1) {
            g[i][jmax] = v[i][jmax];
        }
    }
}

/* Calculate the right hand side of the pressure equation */
void compute_rhs(float **f, float **g, float **rhs, char **flag, int imax,
    int jmax, float del_t, float delx, float dely)
{
    int i, j;

    #pragma omp parallel for collapse(2) schedule(static)
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax; j++) {
            if (flag[i][j] & C_F) {
                rhs[i][j] = (
                             (f[i][j]-f[i-1][j])/delx +
                             (g[i][j]-g[i][j-1])/dely
                            ) / del_t;
            }
        }
    }
}

/* Original serial red/black SOR Poisson */
int poisson(float **p, float **rhs, char **flag, int imax, int jmax,
    float delx, float dely, float eps, int itermax, float omega,
    float *res, int ifull)
{
    int i, j, iter, rb;
    float add, beta_2, beta_mod;
    float p0 = 0.0f;

    float rdx2 = 1.0f/(delx*delx);
    float rdy2 = 1.0f/(dely*dely);
    beta_2 = -omega/(2.0f*(rdx2+rdy2));

    #pragma omp parallel for collapse(2) reduction(+:p0) schedule(static)
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax; j++) {
            if (flag[i][j] & C_F) {
                p0 += p[i][j]*p[i][j];
            }
        }
    }

    p0 = sqrt(p0/ifull);
    if (p0 < 0.0001f) {
        p0 = 1.0f;
    }

    for (iter = 0; iter < itermax; iter++) {
        for (rb = 0; rb <= 1; rb++) {
            #pragma omp parallel for collapse(2) schedule(static) private(beta_mod)
            for (i = 1; i <= imax; i++) {
                for (j = 1; j <= jmax; j++) {
                    if ((i+j) % 2 != rb) continue;

                    if (flag[i][j] == (C_F | B_NSEW)) {
                        p[i][j] = (1.0f-omega)*p[i][j] -
                              beta_2*(
                                    (p[i+1][j]+p[i-1][j])*rdx2
                                  + (p[i][j+1]+p[i][j-1])*rdy2
                                  - rhs[i][j]
                              );
                    } else if (flag[i][j] & C_F) {
                        beta_mod = -omega/((eps_E+eps_W)*rdx2+(eps_N+eps_S)*rdy2);
                        p[i][j] = (1.0f-omega)*p[i][j] -
                            beta_mod*(
                                  (eps_E*p[i+1][j]+eps_W*p[i-1][j])*rdx2
                                + (eps_N*p[i][j+1]+eps_S*p[i][j-1])*rdy2
                                - rhs[i][j]
                            );
                    }
                }
            }
        }

        {
            float res_sum = 0.0f;

            #pragma omp parallel for collapse(2) reduction(+:res_sum) private(add) schedule(static)
            for (i = 1; i <= imax; i++) {
                for (j = 1; j <= jmax; j++) {
                    if (flag[i][j] & C_F) {
                        add = (eps_E*(p[i+1][j]-p[i][j]) -
                               eps_W*(p[i][j]-p[i-1][j])) * rdx2 +
                              (eps_N*(p[i][j+1]-p[i][j]) -
                               eps_S*(p[i][j]-p[i][j-1])) * rdy2 -
                              rhs[i][j];
                        res_sum += add*add;
                    }
                }
            }

            *res = sqrt(res_sum/ifull)/p0;
        }

        if (*res < eps) break;
    }

    return iter;
}

/* Distributed MPI Poisson with OpenMP on local loops */
int poisson_mpi(float **p, float **rhs, char **flag, int imax, int jmax,
    float delx, float dely, float eps, int itermax, float omega,
    float *res, int ifull_local)
{
    int i, j, iter, rb;
    float add, beta_2, beta_mod;
    float p0_local = 0.0f, p0_global = 0.0f;
    float res_local = 0.0f, res_global = 0.0f;
    int ifull_global = 0;

    float rdx2 = 1.0f/(delx*delx);
    float rdy2 = 1.0f/(dely*dely);
    beta_2 = -omega/(2.0f*(rdx2+rdy2));

    #pragma omp parallel for collapse(2) reduction(+:p0_local) schedule(static)
    for (i = 1; i <= imax; i++) {
        for (j = 1; j <= jmax; j++) {
            if (flag[i][j] & C_F) {
                p0_local += p[i][j]*p[i][j];
            }
        }
    }

    MPI_Allreduce(&p0_local, &p0_global, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&ifull_local, &ifull_global, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (ifull_global <= 0) {
        *res = 0.0f;
        return 0;
    }

    p0_global = sqrt(p0_global/ifull_global);
    if (p0_global < 0.0001f) {
        p0_global = 1.0f;
    }

    for (iter = 0; iter < itermax; iter++) {
        for (rb = 0; rb <= 1; rb++) {
            #pragma omp parallel for collapse(2) schedule(static) private(beta_mod)
            for (i = 1; i <= imax; i++) {
                for (j = 1; j <= jmax; j++) {
                    if ((i+j) % 2 != rb) continue;

                    if (flag[i][j] == (C_F | B_NSEW)) {
                        p[i][j] = (1.0f-omega)*p[i][j] -
                              beta_2*(
                                    (p[i+1][j]+p[i-1][j])*rdx2
                                  + (p[i][j+1]+p[i][j-1])*rdy2
                                  - rhs[i][j]
                              );
                    } else if (flag[i][j] & C_F) {
                        beta_mod = -omega/((eps_E+eps_W)*rdx2+(eps_N+eps_S)*rdy2);
                        p[i][j] = (1.0f-omega)*p[i][j] -
                            beta_mod*(
                                  (eps_E*p[i+1][j]+eps_W*p[i-1][j])*rdx2
                                + (eps_N*p[i][j+1]+eps_S*p[i][j-1])*rdy2
                                - rhs[i][j]
                            );
                    }
                }
            }

            exchange_pressure_halo(p, imax, jmax);
        }

        res_local = 0.0f;

        #pragma omp parallel for collapse(2) reduction(+:res_local) private(add) schedule(static)
        for (i = 1; i <= imax; i++) {
            for (j = 1; j <= jmax; j++) {
                if (flag[i][j] & C_F) {
                    add = (eps_E*(p[i+1][j]-p[i][j]) -
                           eps_W*(p[i][j]-p[i-1][j])) * rdx2 +
                          (eps_N*(p[i][j+1]-p[i][j]) -
                           eps_S*(p[i][j]-p[i][j-1])) * rdy2 -
                          rhs[i][j];
                    res_local += add*add;
                }
            }
        }

        MPI_Allreduce(&res_local, &res_global, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

        *res = sqrt(res_global/ifull_global)/p0_global;
        if (*res < eps) break;
    }

    return iter;
}

/* Update the velocity values based on the tentative velocity values and the new pressure matrix */
void update_velocity(float **u, float **v, float **f, float **g, float **p,
    char **flag, int imax, int jmax, float del_t, float delx, float dely)
{
    int i, j;

    #pragma omp parallel for collapse(2) schedule(static)
    for (i = 1; i <= imax - 1; i++) {
        for (j = 1; j <= jmax; j++) {
            if ((flag[i][j] & C_F) && (flag[i+1][j] & C_F)) {
                u[i][j] = f[i][j]-(p[i+1][j]-p[i][j])*del_t/delx;
            }
        }
    }

    {
        int jv_end = (proc == nprocs - 1) ? (jmax - 1) : jmax;

        #pragma omp parallel for collapse(2) schedule(static)
        for (i = 1; i <= imax; i++) {
            for (j = 1; j <= jv_end; j++) {
                if ((flag[i][j] & C_F) && (flag[i][j+1] & C_F)) {
                    v[i][j] = g[i][j]-(p[i][j+1]-p[i][j])*del_t/dely;
                }
            }
        }
    }
}

/* Set timestep size satisfying CFL conditions */
void set_timestep_interval(float *del_t, int imax, int jmax, float delx,
    float dely, float **u, float **v, float Re, float tau)
{
    int i, j;
    float umax_local = 1.0e-10f;
    float vmax_local = 1.0e-10f;
    float umax_global, vmax_global;
    float deltu, deltv, deltRe;

    if (tau < 1.0e-10f) {
        return;
    }

    #pragma omp parallel
    {
        float upriv = 1.0e-10f;
        float vpriv = 1.0e-10f;

        #pragma omp for nowait collapse(2) schedule(static)
        for (i = 0; i <= imax + 1; i++) {
            for (j = 1; j <= jmax + 1; j++) {
                upriv = max(fabs(u[i][j]), upriv);
            }
        }

        #pragma omp for nowait collapse(2) schedule(static)
        for (i = 1; i <= imax + 1; i++) {
            for (j = 0; j <= jmax + 1; j++) {
                vpriv = max(fabs(v[i][j]), vpriv);
            }
        }

        #pragma omp critical
        {
            umax_local = max(umax_local, upriv);
            vmax_local = max(vmax_local, vpriv);
        }
    }

    MPI_Allreduce(&umax_local, &umax_global, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&vmax_local, &vmax_global, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);

    deltu = delx/umax_global;
    deltv = dely/vmax_global;
    deltRe = 1.0f/(1.0f/(delx*delx)+1.0f/(dely*dely))*Re/2.0f;

    if (deltu < deltv) {
        *del_t = min(deltu, deltRe);
    } else {
        *del_t = min(deltv, deltRe);
    }

    *del_t = tau * (*del_t);
}