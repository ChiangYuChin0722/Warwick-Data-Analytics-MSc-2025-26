#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "timer.h"
#include "alloc.h"
#include "boundary.h"
#include "datadef.h"
#include "init.h"
#include "simulation.h"

void write_bin(float **u, float **v, float **p, char **flag,
     int imax, int jmax, float xlength, float ylength, char *file);

int read_bin(float **u, float **v, float **p, char **flag,
    int imax, int jmax, float xlength, float ylength, char *file);

static void print_usage(void);
static void print_version(void);
static void print_help(void);

static char *progname;

int proc = 0;
int nprocs = 0;

int *ileft, *iright;

#define PACKAGE "karman"
#define VERSION "1.0"

static struct option long_opts[] = {
    { "del-t",   1, NULL, 'd' },
    { "help",    0, NULL, 'h' },
    { "imax",    1, NULL, 'x' },
    { "infile",  1, NULL, 'i' },
    { "jmax",    1, NULL, 'y' },
    { "outfile", 1, NULL, 'o' },
    { "t-end",   1, NULL, 't' },
    { "verbose", 1, NULL, 'v' },
    { "version", 0, NULL, 'V' },
    { 0,         0, 0,    0   }
};
#define GETOPTS "d:hi:o:t:v:Vx:y:"

/* =========================================================
   Pack / unpack helpers for interior blocks
   ========================================================= */
static void pack_float_block(float **src, float *buf,
                             int imax, int jstart, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 0; j < local_jmax; j++) {
            int gj = jstart + j;
            buf[idx++] = src[i][gj];
        }
    }
}

static void unpack_float_block(float *buf, float **dst,
                               int imax, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 1; j <= local_jmax; j++) {
            dst[i][j] = buf[idx++];
        }
    }
}

static void pack_char_block(char **src, char *buf,
                            int imax, int jstart, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 0; j < local_jmax; j++) {
            int gj = jstart + j;
            buf[idx++] = src[i][gj];
        }
    }
}

static void unpack_char_block(char *buf, char **dst,
                              int imax, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 1; j <= local_jmax; j++) {
            dst[i][j] = buf[idx++];
        }
    }
}

/* =========================================================
   Pack / unpack one full row
   ========================================================= */
static void pack_float_row(float **src, float *buf, int imax, int gj)
{
    int i;
    for (i = 0; i <= imax + 1; i++) {
        buf[i] = src[i][gj];
    }
}

static void unpack_float_row(float *buf, float **dst, int imax, int lj)
{
    int i;
    for (i = 0; i <= imax + 1; i++) {
        dst[i][lj] = buf[i];
    }
}

static void pack_char_row(char **src, char *buf, int imax, int gj)
{
    int i;
    for (i = 0; i <= imax + 1; i++) {
        buf[i] = src[i][gj];
    }
}

static void unpack_char_row(char *buf, char **dst, int imax, int lj)
{
    int i;
    for (i = 0; i <= imax + 1; i++) {
        dst[i][lj] = buf[i];
    }
}

/* =========================================================
   Pack local interior back into contiguous buffer
   ========================================================= */
static void pack_float_local_interior(float **src, float *buf,
                                      int imax, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 1; j <= local_jmax; j++) {
            buf[idx++] = src[i][j];
        }
    }
}

static void unpack_float_to_global(float *buf, float **dst,
                                   int imax, int jstart, int local_jmax)
{
    int i, j, idx = 0;
    for (i = 0; i <= imax + 1; i++) {
        for (j = 0; j < local_jmax; j++) {
            int gj = jstart + j;
            dst[i][gj] = buf[idx++];
        }
    }
}

/* =========================================================
   Halo exchange for arrays split along j-direction
   ========================================================= */
static void exchange_float_halo(float **a, int imax, int local_jmax,
                                int rank, int size)
{
    int count = imax + 2;
    int i;

    float *send_up   = (float *)malloc(count * sizeof(float));
    float *send_down = (float *)malloc(count * sizeof(float));
    float *recv_up   = (float *)malloc(count * sizeof(float));
    float *recv_down = (float *)malloc(count * sizeof(float));

    if (!send_up || !send_down || !recv_up || !recv_down) {
        fprintf(stderr, "rank %d: halo float buffer allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i <= imax + 1; i++) {
        send_up[i]   = a[i][1];
        send_down[i] = a[i][local_jmax];
    }

    {
        int up   = (rank == 0) ? MPI_PROC_NULL : rank - 1;
        int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

        MPI_Sendrecv(send_up, count, MPI_FLOAT, up, 10,
                     recv_down, count, MPI_FLOAT, down, 10,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(send_down, count, MPI_FLOAT, down, 11,
                     recv_up, count, MPI_FLOAT, up, 11,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (i = 0; i <= imax + 1; i++) {
            if (up != MPI_PROC_NULL) {
                a[i][0] = recv_up[i];
            }
            if (down != MPI_PROC_NULL) {
                a[i][local_jmax + 1] = recv_down[i];
            }
        }
    }

    free(send_up);
    free(send_down);
    free(recv_up);
    free(recv_down);
}

static void exchange_char_halo(char **a, int imax, int local_jmax,
                               int rank, int size)
{
    int count = imax + 2;
    int i;

    char *send_up   = (char *)malloc(count * sizeof(char));
    char *send_down = (char *)malloc(count * sizeof(char));
    char *recv_up   = (char *)malloc(count * sizeof(char));
    char *recv_down = (char *)malloc(count * sizeof(char));

    if (!send_up || !send_down || !recv_up || !recv_down) {
        fprintf(stderr, "rank %d: halo char buffer allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i <= imax + 1; i++) {
        send_up[i]   = a[i][1];
        send_down[i] = a[i][local_jmax];
    }

    {
        int up   = (rank == 0) ? MPI_PROC_NULL : rank - 1;
        int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

        MPI_Sendrecv(send_up, count, MPI_CHAR, up, 20,
                     recv_down, count, MPI_CHAR, down, 20,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(send_down, count, MPI_CHAR, down, 21,
                     recv_up, count, MPI_CHAR, up, 21,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (i = 0; i <= imax + 1; i++) {
            if (up != MPI_PROC_NULL) {
                a[i][0] = recv_up[i];
            }
            if (down != MPI_PROC_NULL) {
                a[i][local_jmax + 1] = recv_down[i];
            }
        }
    }

    free(send_up);
    free(send_down);
    free(recv_up);
    free(recv_down);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (proc == 0) {
        printf("MPI started with %d ranks\n", nprocs);
    }

    int verbose = 0;
    float xlength = 22.0f;
    float ylength = 4.1f;
    int imax = 660;
    int jmax = 120;

    char *infile;
    char *outfile;

    float t_end = 2.1f;
    float del_t = 0.003f;
    float tau   = 0.5f;

    int   itermax = 100;
    float eps     = 0.001f;
    float omega   = 1.7f;
    float gamma   = 0.9f;

    float Re = 150.0f;
    float ui = 1.0f;
    float vi = 0.0f;

    float delx, dely;
    int i, j, ibound = 0;
    int init_case = 0;
    int show_help = 0, show_usage = 0, show_version = 0;

    progname = argv[0];
    infile = strdup("karman.bin");
    outfile = strdup("karman.bin");

    float **u_global = NULL, **v_global = NULL, **p_global = NULL;
    float **rhs_global = NULL, **f_global = NULL, **g_global = NULL;
    char  **flag_global = NULL;

    float **u_local = NULL, **v_local = NULL, **p_local = NULL;
    float **rhs_local = NULL, **f_local = NULL, **g_local = NULL;
    char  **flag_local = NULL;

    int optc;
    while ((optc = getopt_long(argc, argv, GETOPTS, long_opts, NULL)) != -1) {
        switch (optc) {
            case 'h':
                show_help = 1;
                break;
            case 'V':
                show_version = 1;
                break;
            case 'v':
                verbose = atoi(optarg);
                break;
            case 'x':
                imax = atoi(optarg);
                break;
            case 'y':
                jmax = atoi(optarg);
                break;
            case 'i':
                free(infile);
                infile = strdup(optarg);
                break;
            case 'o':
                free(outfile);
                outfile = strdup(optarg);
                break;
            case 'd':
                del_t = atof(optarg);
                break;
            case 't':
                t_end = atof(optarg);
                break;
            default:
                show_usage = 1;
                break;
        }
    }

    if (show_usage || optind < argc) {
        print_usage();
        MPI_Finalize();
        return 1;
    }

    if (show_version) {
        print_version();
        if (!show_help) {
            MPI_Finalize();
            return 0;
        }
    }

    if (show_help) {
        print_help();
        MPI_Finalize();
        return 0;
    }

    (void)verbose;
    (void)t_end;
    (void)tau;
    (void)itermax;
    (void)eps;
    (void)omega;

    delx = xlength / imax;
    dely = ylength / jmax;

    {
        int base = jmax / nprocs;
        int rem  = jmax % nprocs;

        int local_jmax = base + (proc < rem ? 1 : 0);
        int jstart;
        int jend;

        if (proc < rem) {
            jstart = proc * (base + 1) + 1;
        } else {
            jstart = rem * (base + 1) + (proc - rem) * base + 1;
        }
        jend = jstart + local_jmax - 1;

        printf("rank %d/%d: jstart=%d jend=%d local_jmax=%d\n",
               proc, nprocs, jstart, jend, local_jmax);

        u_local    = alloc_floatmatrix(imax + 2, local_jmax + 2);
        v_local    = alloc_floatmatrix(imax + 2, local_jmax + 2);
        f_local    = alloc_floatmatrix(imax + 2, local_jmax + 2);
        g_local    = alloc_floatmatrix(imax + 2, local_jmax + 2);
        p_local    = alloc_floatmatrix(imax + 2, local_jmax + 2);
        rhs_local  = alloc_floatmatrix(imax + 2, local_jmax + 2);
        flag_local = alloc_charmatrix(imax + 2, local_jmax + 2);

        if (!u_local || !v_local || !f_local || !g_local ||
            !p_local || !rhs_local || !flag_local) {
            fprintf(stderr, "rank %d: Couldn't allocate local matrices.\n", proc);
            MPI_Finalize();
            return 1;
        }

        printf("rank %d: local arrays allocated successfully\n", proc);

        if (proc == 0) {
            u_global    = alloc_floatmatrix(imax + 2, jmax + 2);
            v_global    = alloc_floatmatrix(imax + 2, jmax + 2);
            f_global    = alloc_floatmatrix(imax + 2, jmax + 2);
            g_global    = alloc_floatmatrix(imax + 2, jmax + 2);
            p_global    = alloc_floatmatrix(imax + 2, jmax + 2);
            rhs_global  = alloc_floatmatrix(imax + 2, jmax + 2);
            flag_global = alloc_charmatrix(imax + 2, jmax + 2);

            if (!u_global || !v_global || !f_global || !g_global ||
                !p_global || !rhs_global || !flag_global) {
                fprintf(stderr, "root: Couldn't allocate global matrices.\n");
                MPI_Finalize();
                return 1;
            }

            printf("root: global arrays allocated successfully\n");

            init_case = read_bin(u_global, v_global, p_global, flag_global,
                                 imax, jmax, xlength, ylength, infile);

            if (init_case > 0) {
                MPI_Finalize();
                return 1;
            }

            if (init_case < 0) {
                for (i = 0; i <= imax + 1; i++) {
                    for (j = 0; j <= jmax + 1; j++) {
                        u_global[i][j] = ui;
                        v_global[i][j] = vi;
                        p_global[i][j] = 0.0f;
                    }
                }

                init_flag(flag_global, imax, jmax, delx, dely, &ibound);
                apply_boundary_conditions(u_global, v_global, flag_global,
                                          imax, jmax, ui, vi);
            }

            {
                int count_root = (imax + 2) * local_jmax;
                float *buf_u = (float *)malloc(count_root * sizeof(float));
                float *buf_v = (float *)malloc(count_root * sizeof(float));
                float *buf_p = (float *)malloc(count_root * sizeof(float));
                char  *buf_flag = (char *)malloc(count_root * sizeof(char));

                if (!buf_u || !buf_v || !buf_p || !buf_flag) {
                    fprintf(stderr, "root: failed to allocate self-copy buffers\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                pack_float_block(u_global, buf_u, imax, jstart, local_jmax);
                pack_float_block(v_global, buf_v, imax, jstart, local_jmax);
                pack_float_block(p_global, buf_p, imax, jstart, local_jmax);
                pack_char_block(flag_global, buf_flag, imax, jstart, local_jmax);

                unpack_float_block(buf_u, u_local, imax, local_jmax);
                unpack_float_block(buf_v, v_local, imax, local_jmax);
                unpack_float_block(buf_p, p_local, imax, local_jmax);
                unpack_char_block(buf_flag, flag_local, imax, local_jmax);

                free(buf_u);
                free(buf_v);
                free(buf_p);
                free(buf_flag);
            }

            {
                int dest;
                for (dest = 1; dest < nprocs; dest++) {
                    int base_d = jmax / nprocs;
                    int rem_d  = jmax % nprocs;
                    int local_jmax_d = base_d + (dest < rem_d ? 1 : 0);
                    int jstart_d;
                    int count_d;

                    if (dest < rem_d) {
                        jstart_d = dest * (base_d + 1) + 1;
                    } else {
                        jstart_d = rem_d * (base_d + 1) + (dest - rem_d) * base_d + 1;
                    }

                    count_d = (imax + 2) * local_jmax_d;

                    {
                        float *send_u = (float *)malloc(count_d * sizeof(float));
                        float *send_v = (float *)malloc(count_d * sizeof(float));
                        float *send_p = (float *)malloc(count_d * sizeof(float));
                        char  *send_flag = (char *)malloc(count_d * sizeof(char));

                        if (!send_u || !send_v || !send_p || !send_flag) {
                            fprintf(stderr, "root: failed to allocate send buffers for rank %d\n", dest);
                            MPI_Abort(MPI_COMM_WORLD, 1);
                        }

                        pack_float_block(u_global, send_u, imax, jstart_d, local_jmax_d);
                        pack_float_block(v_global, send_v, imax, jstart_d, local_jmax_d);
                        pack_float_block(p_global, send_p, imax, jstart_d, local_jmax_d);
                        pack_char_block(flag_global, send_flag, imax, jstart_d, local_jmax_d);

                        MPI_Send(send_u, count_d, MPI_FLOAT, dest, 100, MPI_COMM_WORLD);
                        MPI_Send(send_v, count_d, MPI_FLOAT, dest, 101, MPI_COMM_WORLD);
                        MPI_Send(send_p, count_d, MPI_FLOAT, dest, 102, MPI_COMM_WORLD);
                        MPI_Send(send_flag, count_d, MPI_CHAR, dest, 103, MPI_COMM_WORLD);

                        free(send_u);
                        free(send_v);
                        free(send_p);
                        free(send_flag);
                    }
                }
            }
        } else {
            int count_local = (imax + 2) * local_jmax;
            float *recv_u = (float *)malloc(count_local * sizeof(float));
            float *recv_v = (float *)malloc(count_local * sizeof(float));
            float *recv_p = (float *)malloc(count_local * sizeof(float));
            char  *recv_flag = (char *)malloc(count_local * sizeof(char));

            if (!recv_u || !recv_v || !recv_p || !recv_flag) {
                fprintf(stderr, "rank %d: failed to allocate receive buffers\n", proc);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(recv_u, count_local, MPI_FLOAT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(recv_v, count_local, MPI_FLOAT, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(recv_p, count_local, MPI_FLOAT, 0, 102, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(recv_flag, count_local, MPI_CHAR, 0, 103, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            unpack_float_block(recv_u, u_local, imax, local_jmax);
            unpack_float_block(recv_v, v_local, imax, local_jmax);
            unpack_float_block(recv_p, p_local, imax, local_jmax);
            unpack_char_block(recv_flag, flag_local, imax, local_jmax);

            free(recv_u);
            free(recv_v);
            free(recv_p);
            free(recv_flag);
        }

        printf("rank %d: local block received/copied\n", proc);
        printf("rank %d: before halo u_local[1][1]=%f u_local[1][local_jmax]=%f\n",
               proc, u_local[1][1], u_local[1][local_jmax]);

        if (proc == 0) {
            float *top_u = (float *)malloc((imax + 2) * sizeof(float));
            float *top_v = (float *)malloc((imax + 2) * sizeof(float));
            float *top_p = (float *)malloc((imax + 2) * sizeof(float));
            char  *top_flag = (char *)malloc((imax + 2) * sizeof(char));

            if (!top_u || !top_v || !top_p || !top_flag) {
                fprintf(stderr, "root: failed to allocate top boundary buffers\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            pack_float_row(u_global, top_u, imax, 0);
            pack_float_row(v_global, top_v, imax, 0);
            pack_float_row(p_global, top_p, imax, 0);
            pack_char_row(flag_global, top_flag, imax, 0);

            unpack_float_row(top_u, u_local, imax, 0);
            unpack_float_row(top_v, v_local, imax, 0);
            unpack_float_row(top_p, p_local, imax, 0);
            unpack_char_row(top_flag, flag_local, imax, 0);

            free(top_u);
            free(top_v);
            free(top_p);
            free(top_flag);

            if (nprocs == 1) {
                float *bot_u = (float *)malloc((imax + 2) * sizeof(float));
                float *bot_v = (float *)malloc((imax + 2) * sizeof(float));
                float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
                char  *bot_flag = (char *)malloc((imax + 2) * sizeof(char));

                if (!bot_u || !bot_v || !bot_p || !bot_flag) {
                    fprintf(stderr, "root: failed to allocate bottom boundary buffers\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                pack_float_row(u_global, bot_u, imax, jmax + 1);
                pack_float_row(v_global, bot_v, imax, jmax + 1);
                pack_float_row(p_global, bot_p, imax, jmax + 1);
                pack_char_row(flag_global, bot_flag, imax, jmax + 1);

                unpack_float_row(bot_u, u_local, imax, local_jmax + 1);
                unpack_float_row(bot_v, v_local, imax, local_jmax + 1);
                unpack_float_row(bot_p, p_local, imax, local_jmax + 1);
                unpack_char_row(bot_flag, flag_local, imax, local_jmax + 1);

                free(bot_u);
                free(bot_v);
                free(bot_p);
                free(bot_flag);
            } else {
                int last = nprocs - 1;

                float *bot_u = (float *)malloc((imax + 2) * sizeof(float));
                float *bot_v = (float *)malloc((imax + 2) * sizeof(float));
                float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
                char  *bot_flag = (char *)malloc((imax + 2) * sizeof(char));

                if (!bot_u || !bot_v || !bot_p || !bot_flag) {
                    fprintf(stderr, "root: failed to allocate bottom boundary send buffers\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                pack_float_row(u_global, bot_u, imax, jmax + 1);
                pack_float_row(v_global, bot_v, imax, jmax + 1);
                pack_float_row(p_global, bot_p, imax, jmax + 1);
                pack_char_row(flag_global, bot_flag, imax, jmax + 1);

                MPI_Send(bot_u, imax + 2, MPI_FLOAT, last, 300, MPI_COMM_WORLD);
                MPI_Send(bot_v, imax + 2, MPI_FLOAT, last, 301, MPI_COMM_WORLD);
                MPI_Send(bot_p, imax + 2, MPI_FLOAT, last, 302, MPI_COMM_WORLD);
                MPI_Send(bot_flag, imax + 2, MPI_CHAR, last, 303, MPI_COMM_WORLD);

                free(bot_u);
                free(bot_v);
                free(bot_p);
                free(bot_flag);
            }
        } else if (proc == nprocs - 1) {
            float *bot_u = (float *)malloc((imax + 2) * sizeof(float));
            float *bot_v = (float *)malloc((imax + 2) * sizeof(float));
            float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
            char  *bot_flag = (char *)malloc((imax + 2) * sizeof(char));

            if (!bot_u || !bot_v || !bot_p || !bot_flag) {
                fprintf(stderr, "rank %d: failed to allocate bottom boundary receive buffers\n", proc);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(bot_u, imax + 2, MPI_FLOAT, 0, 300, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(bot_v, imax + 2, MPI_FLOAT, 0, 301, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(bot_p, imax + 2, MPI_FLOAT, 0, 302, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(bot_flag, imax + 2, MPI_CHAR, 0, 303, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            unpack_float_row(bot_u, u_local, imax, local_jmax + 1);
            unpack_float_row(bot_v, v_local, imax, local_jmax + 1);
            unpack_float_row(bot_p, p_local, imax, local_jmax + 1);
            unpack_char_row(bot_flag, flag_local, imax, local_jmax + 1);

            free(bot_u);
            free(bot_v);
            free(bot_p);
            free(bot_flag);
        }

        exchange_float_halo(u_local, imax, local_jmax, proc, nprocs);
        exchange_float_halo(v_local, imax, local_jmax, proc, nprocs);
        exchange_float_halo(p_local, imax, local_jmax, proc, nprocs);
        exchange_char_halo(flag_local, imax, local_jmax, proc, nprocs);

        MPI_Barrier(MPI_COMM_WORLD);

        printf("rank %d: halo exchange complete\n", proc);
        printf("rank %d: ghost samples top=%f bottom=%f\n",
               proc, u_local[1][0], u_local[1][local_jmax + 1]);

        MPI_Barrier(MPI_COMM_WORLD);

        if (proc == 0) {
            printf("Initial data distribution + halo exchange complete.\n");
            printf("Now testing local compute_tentative_velocity + compute_rhs.\n");
        }

        compute_tentative_velocity(u_local, v_local, f_local, g_local,
                                   flag_local, imax, local_jmax,
                                   del_t, delx, dely, gamma, Re);

        exchange_float_halo(f_local, imax, local_jmax, proc, nprocs);
        exchange_float_halo(g_local, imax, local_jmax, proc, nprocs);

        compute_rhs(f_local, g_local, rhs_local, flag_local,
                    imax, local_jmax, del_t, delx, dely);

        MPI_Barrier(MPI_COMM_WORLD);

        printf("rank %d: local compute complete\n", proc);
        printf("rank %d: sample f_local[10][10]=%f g_local[10][10]=%f rhs_local[10][10]=%f\n",
               proc, f_local[10][10], g_local[10][10], rhs_local[10][10]);

        if (proc == 0) {
            printf("root debug: u_local[10][10]=%f v_local[10][10]=%f\n",
                   u_local[10][10], v_local[10][10]);
            printf("root debug: f_local[10][10]=%f g_local[10][10]=%f rhs_local[10][10]=%f\n",
                   f_local[10][10], g_local[10][10], rhs_local[10][10]);
        }

        {
            int count_local = (imax + 2) * local_jmax;
            float *rhs_buf = (float *)malloc(count_local * sizeof(float));

            if (!rhs_buf) {
                fprintf(stderr, "rank %d: failed to allocate rhs send buffer\n", proc);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            pack_float_local_interior(rhs_local, rhs_buf, imax, local_jmax);

            if (proc == 0) {
                unpack_float_to_global(rhs_buf, rhs_global, imax, jstart, local_jmax);
                free(rhs_buf);

                for (int src = 1; src < nprocs; src++) {
                    int base_s = jmax / nprocs;
                    int rem_s  = jmax % nprocs;
                    int local_jmax_s = base_s + (src < rem_s ? 1 : 0);

                    int jstart_s;
                    if (src < rem_s) {
                        jstart_s = src * (base_s + 1) + 1;
                    } else {
                        jstart_s = rem_s * (base_s + 1) + (src - rem_s) * base_s + 1;
                    }

                    int count_s = (imax + 2) * local_jmax_s;
                    float *recv_buf = (float *)malloc(count_s * sizeof(float));

                    if (!recv_buf) {
                        fprintf(stderr, "root: failed to allocate rhs recv buffer for rank %d\n", src);
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }

                    MPI_Recv(recv_buf, count_s, MPI_FLOAT, src, 400,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    unpack_float_to_global(recv_buf, rhs_global, imax, jstart_s, local_jmax_s);

                    free(recv_buf);
                }
            } else {
                MPI_Send(rhs_buf, count_local, MPI_FLOAT, 0, 400, MPI_COMM_WORLD);
                free(rhs_buf);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (proc == 0) {
            printf("rhs gather complete on root\n");
            printf("sample rhs_global[10][10]=%f rhs_global[10][80]=%f\n",
                   rhs_global[10][10], rhs_global[10][80]);
        }

        int ifluid_global = imax * jmax - ibound;
        int itersor = 0;
        float res = 0.0f;

        if (proc == 0) {
            if (ifluid_global > 0) {
                itersor = poisson(p_global, rhs_global, flag_global,
                                  imax, jmax, delx, dely,
                                  eps, itermax, omega, &res, ifluid_global);
            }

            printf("root: poisson complete, iters=%d res=%e\n", itersor, res);
            printf("root: sample p_global[10][10]=%f p_global[10][80]=%f\n",
                   p_global[10][10], p_global[10][80]);
        }

        {
            int count_local = (imax + 2) * local_jmax;

            if (proc == 0) {
                float *buf_p = (float *)malloc(count_local * sizeof(float));
                if (!buf_p) {
                    fprintf(stderr, "root: failed to allocate p self-copy buffer\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                pack_float_block(p_global, buf_p, imax, jstart, local_jmax);
                unpack_float_block(buf_p, p_local, imax, local_jmax);
                free(buf_p);

                for (int dest = 1; dest < nprocs; dest++) {
                    int base_d = jmax / nprocs;
                    int rem_d  = jmax % nprocs;
                    int local_jmax_d = base_d + (dest < rem_d ? 1 : 0);

                    int jstart_d;
                    if (dest < rem_d) {
                        jstart_d = dest * (base_d + 1) + 1;
                    } else {
                        jstart_d = rem_d * (base_d + 1) + (dest - rem_d) * base_d + 1;
                    }

                    int count_d = (imax + 2) * local_jmax_d;
                    float *send_p = (float *)malloc(count_d * sizeof(float));

                    if (!send_p) {
                        fprintf(stderr, "root: failed to allocate p send buffer for rank %d\n", dest);
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }

                    pack_float_block(p_global, send_p, imax, jstart_d, local_jmax_d);
                    MPI_Send(send_p, count_d, MPI_FLOAT, dest, 500, MPI_COMM_WORLD);
                    free(send_p);
                }
            } else {
                float *recv_p = (float *)malloc(count_local * sizeof(float));
                if (!recv_p) {
                    fprintf(stderr, "rank %d: failed to allocate p recv buffer\n", proc);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                MPI_Recv(recv_p, count_local, MPI_FLOAT, 0, 500,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                unpack_float_block(recv_p, p_local, imax, local_jmax);
                free(recv_p);
            }
        }

        if (proc == 0) {
            float *top_p = (float *)malloc((imax + 2) * sizeof(float));
            if (!top_p) {
                fprintf(stderr, "root: failed to allocate top p boundary buffer\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            pack_float_row(p_global, top_p, imax, 0);
            unpack_float_row(top_p, p_local, imax, 0);
            free(top_p);

            if (nprocs == 1) {
                float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
                if (!bot_p) {
                    fprintf(stderr, "root: failed to allocate bottom p boundary buffer\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                pack_float_row(p_global, bot_p, imax, jmax + 1);
                unpack_float_row(bot_p, p_local, imax, local_jmax + 1);
                free(bot_p);
            } else {
                int last = nprocs - 1;
                float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
                if (!bot_p) {
                    fprintf(stderr, "root: failed to allocate bottom p send buffer\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                pack_float_row(p_global, bot_p, imax, jmax + 1);
                MPI_Send(bot_p, imax + 2, MPI_FLOAT, last, 501, MPI_COMM_WORLD);
                free(bot_p);
            }
        } else if (proc == nprocs - 1) {
            float *bot_p = (float *)malloc((imax + 2) * sizeof(float));
            if (!bot_p) {
                fprintf(stderr, "rank %d: failed to allocate bottom p recv buffer\n", proc);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MPI_Recv(bot_p, imax + 2, MPI_FLOAT, 0, 501,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            unpack_float_row(bot_p, p_local, imax, local_jmax + 1);
            free(bot_p);
        }

        exchange_float_halo(p_local, imax, local_jmax, proc, nprocs);

        MPI_Barrier(MPI_COMM_WORLD);

        printf("rank %d: p redistribution complete\n", proc);
        printf("rank %d: sample p_local[10][10]=%f topghost=%f botghost=%f\n",
               proc, p_local[10][10], p_local[10][0], p_local[10][local_jmax + 1]);

        MPI_Barrier(MPI_COMM_WORLD);

        if (proc == 0) {
            printf("This version now completes pressure solve and redistributes p.\n");
            printf("Now testing local update_velocity.\n");
        }

        update_velocity(u_local, v_local, f_local, g_local, p_local,
                        flag_local, imax, local_jmax, del_t, delx, dely);

        exchange_float_halo(u_local, imax, local_jmax, proc, nprocs);
        exchange_float_halo(v_local, imax, local_jmax, proc, nprocs);

        MPI_Barrier(MPI_COMM_WORLD);

        printf("rank %d: update_velocity complete\n", proc);
        printf("rank %d: sample u_local[10][10]=%f v_local[10][10]=%f\n",
               proc, u_local[10][10], v_local[10][10]);
        printf("rank %d: v i=1   top_int=%f bot_int=%f top_ghost=%f bot_ghost=%f\n",
            proc,
            v_local[1][1], v_local[1][local_jmax],
            v_local[1][0], v_local[1][local_jmax+1]);

        printf("rank %d: v i=45  top_int=%f bot_int=%f top_ghost=%f bot_ghost=%f\n",
            proc,
            v_local[45][1], v_local[45][local_jmax],
            v_local[45][0], v_local[45][local_jmax+1]);

        MPI_Barrier(MPI_COMM_WORLD);

        if (proc == 0) {
            printf("Now gathering u/v/p/flag back to root.\n");
        }

        {
            int count_local = (imax + 2) * local_jmax;

            float *buf_u = (float *)malloc(count_local * sizeof(float));
            float *buf_v = (float *)malloc(count_local * sizeof(float));
            float *buf_p = (float *)malloc(count_local * sizeof(float));
            char  *buf_flag = (char *)malloc(count_local * sizeof(char));

            if (!buf_u || !buf_v || !buf_p || !buf_flag) {
                fprintf(stderr, "rank %d: failed to allocate final gather buffers\n", proc);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            pack_float_local_interior(u_local, buf_u, imax, local_jmax);
            pack_float_local_interior(v_local, buf_v, imax, local_jmax);
            pack_float_local_interior(p_local, buf_p, imax, local_jmax);

            {
                int ii, jj, idx = 0;
                for (ii = 0; ii <= imax + 1; ii++) {
                    for (jj = 1; jj <= local_jmax; jj++) {
                        buf_flag[idx++] = flag_local[ii][jj];
                    }
                }
            }

            if (proc == 0) {
                unpack_float_to_global(buf_u, u_global, imax, jstart, local_jmax);
                unpack_float_to_global(buf_v, v_global, imax, jstart, local_jmax);
                unpack_float_to_global(buf_p, p_global, imax, jstart, local_jmax);

                {
                    int ii, jj, idx = 0;
                    for (ii = 0; ii <= imax + 1; ii++) {
                        for (jj = 0; jj < local_jmax; jj++) {
                            int gj = jstart + jj;
                            flag_global[ii][gj] = buf_flag[idx++];
                        }
                    }
                }

                free(buf_u);
                free(buf_v);
                free(buf_p);
                free(buf_flag);

                for (int src = 1; src < nprocs; src++) {
                    int base_s = jmax / nprocs;
                    int rem_s  = jmax % nprocs;
                    int local_jmax_s = base_s + (src < rem_s ? 1 : 0);

                    int jstart_s;
                    if (src < rem_s) {
                        jstart_s = src * (base_s + 1) + 1;
                    } else {
                        jstart_s = rem_s * (base_s + 1) + (src - rem_s) * base_s + 1;
                    }

                    int count_s = (imax + 2) * local_jmax_s;

                    float *recv_u = (float *)malloc(count_s * sizeof(float));
                    float *recv_v = (float *)malloc(count_s * sizeof(float));
                    float *recv_p = (float *)malloc(count_s * sizeof(float));
                    char  *recv_flag = (char *)malloc(count_s * sizeof(char));

                    if (!recv_u || !recv_v || !recv_p || !recv_flag) {
                        fprintf(stderr, "root: failed to allocate recv buffers for rank %d\n", src);
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }

                    MPI_Recv(recv_u, count_s, MPI_FLOAT, src, 600, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(recv_v, count_s, MPI_FLOAT, src, 601, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(recv_p, count_s, MPI_FLOAT, src, 602, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(recv_flag, count_s, MPI_CHAR, src, 603, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    unpack_float_to_global(recv_u, u_global, imax, jstart_s, local_jmax_s);
                    unpack_float_to_global(recv_v, v_global, imax, jstart_s, local_jmax_s);
                    unpack_float_to_global(recv_p, p_global, imax, jstart_s, local_jmax_s);

                    {
                        int ii, jj, idx = 0;
                        for (ii = 0; ii <= imax + 1; ii++) {
                            for (jj = 0; jj < local_jmax_s; jj++) {
                                int gj = jstart_s + jj;
                                flag_global[ii][gj] = recv_flag[idx++];
                            }
                        }
                    }

                    free(recv_u);
                    free(recv_v);
                    free(recv_p);
                    free(recv_flag);
                }

            } else {
                MPI_Send(buf_u, count_local, MPI_FLOAT, 0, 600, MPI_COMM_WORLD);
                MPI_Send(buf_v, count_local, MPI_FLOAT, 0, 601, MPI_COMM_WORLD);
                MPI_Send(buf_p, count_local, MPI_FLOAT, 0, 602, MPI_COMM_WORLD);
                MPI_Send(buf_flag, count_local, MPI_CHAR, 0, 603, MPI_COMM_WORLD);

                free(buf_u);
                free(buf_v);
                free(buf_p);
                free(buf_flag);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (proc == 0) {
            printf("final gather complete on root\n");
            printf("sample u_global[10][10]=%f v_global[10][10]=%f p_global[10][10]=%f\n",
                   u_global[10][10], v_global[10][10], p_global[10][10]);

            if (outfile != NULL && strcmp(outfile, "") != 0) {
                write_bin(u_global, v_global, p_global, flag_global,
                          imax, jmax, xlength, ylength, outfile);
                printf("output written to %s\n", outfile);
            }
        }
    }

    free_matrix(u_local);
    free_matrix(v_local);
    free_matrix(f_local);
    free_matrix(g_local);
    free_matrix(p_local);
    free_matrix(rhs_local);
    free_matrix(flag_local);

    if (proc == 0) {
        free_matrix(u_global);
        free_matrix(v_global);
        free_matrix(f_global);
        free_matrix(g_global);
        free_matrix(p_global);
        free_matrix(rhs_global);
        free_matrix(flag_global);
    }

    free(infile);
    free(outfile);

    MPI_Finalize();
    return 0;
}

void write_bin(float **u, float **v, float **p, char **flag,
    int imax, int jmax, float xlength, float ylength, char* file)
{
    int i;
    FILE *fp = fopen(file, "wb");

    if (fp == NULL) {
        fprintf(stderr, "Could not open file '%s': %s\n", file, strerror(errno));
        return;
    }

    fwrite(&imax, sizeof(int), 1, fp);
    fwrite(&jmax, sizeof(int), 1, fp);
    fwrite(&xlength, sizeof(float), 1, fp);
    fwrite(&ylength, sizeof(float), 1, fp);

    for (i = 0; i < imax + 2; i++) {
        fwrite(u[i],    sizeof(float), jmax + 2, fp);
        fwrite(v[i],    sizeof(float), jmax + 2, fp);
        fwrite(p[i],    sizeof(float), jmax + 2, fp);
        fwrite(flag[i], sizeof(char),  jmax + 2, fp);
    }

    fclose(fp);
}

int read_bin(float **u, float **v, float **p, char **flag,
    int imax, int jmax, float xlength, float ylength, char* file)
{
    int i, j;
    FILE *fp;

    if (file == NULL) return -1;

    if ((fp = fopen(file, "rb")) == NULL) {
        fprintf(stderr, "Could not open file '%s': %s\n", file, strerror(errno));
        fprintf(stderr, "Generating default state instead.\n");
        return -1;
    }

    fread(&i, sizeof(int), 1, fp);
    fread(&j, sizeof(int), 1, fp);

    {
        float xl, yl;
        fread(&xl, sizeof(float), 1, fp);
        fread(&yl, sizeof(float), 1, fp);

        if (i != imax || j != jmax) {
            fprintf(stderr, "Warning: imax/jmax have wrong values in %s\n", file);
            fprintf(stderr, "%s's imax = %d, jmax = %d\n", file, i, j);
            fprintf(stderr, "Program's imax = %d, jmax = %d\n", imax, jmax);
            fclose(fp);
            return 1;
        }

        if (xl != xlength || yl != ylength) {
            fprintf(stderr, "Warning: xlength/ylength have wrong values in %s\n", file);
            fprintf(stderr, "%s's xlength = %g, ylength = %g\n", file, xl, yl);
            fprintf(stderr, "Program's xlength = %g, ylength = %g\n", xlength, ylength);
            fclose(fp);
            return 1;
        }
    }

    for (i = 0; i < imax + 2; i++) {
        fread(u[i],    sizeof(float), jmax + 2, fp);
        fread(v[i],    sizeof(float), jmax + 2, fp);
        fread(p[i],    sizeof(float), jmax + 2, fp);
        fread(flag[i], sizeof(char),  jmax + 2, fp);
    }

    fclose(fp);
    return 0;
}

static void print_usage(void)
{
    fprintf(stderr, "Try '%s --help' for more information.\n", progname);
}

static void print_version(void)
{
    fprintf(stderr, "%s %s\n", PACKAGE, VERSION);
}

static void print_help(void)
{
    fprintf(stderr, "%s. A simple computational fluid dynamics tutorial.\n\n", PACKAGE);
    fprintf(stderr, "Usage: %s [OPTIONS]...\n\n", progname);
    fprintf(stderr, "  -h, --help            Print a summary of the options\n");
    fprintf(stderr, "  -V, --version         Print the version number\n");
    fprintf(stderr, "  -v, --verbose=LEVEL   Set the verbosity level. 0 is silent\n");
    fprintf(stderr, "  -x, --");
}