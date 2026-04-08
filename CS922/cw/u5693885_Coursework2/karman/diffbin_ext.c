#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>

static void print_usage(void);
static void print_version(void);
static void print_help(void);

static char *progname;

#define PACKAGE "diffbin_ext"
#define VERSION "1.0"

/* Command line options */
static struct option long_opts[] = {
    { "help",        0, NULL, 'h' },
    { "version",     0, NULL, 'V' },
    { "epsilon",     1, NULL, 'e' },
    { "rel-epsilon", 1, NULL, 'r' },
    { "mode",        1, NULL, 'm' },
    { 0,             0, 0,    0  }
};

#define GETOPTS "e:hr:m:V"

#define MODE_DIFF 0
#define MODE_OUTPUT_U 1
#define MODE_OUTPUT_V 2
#define MODE_OUTPUT_P 3
#define MODE_OUTPUT_FLAGS 4

static inline float rel_diff_f(float a, float b, float floor_val)
{
    float denom = fmaxf(fmaxf(fabsf(a), fabsf(b)), floor_val);
    return fabsf(a - b) / denom;
}

static inline const char *var_name(int k)
{
    return (k == 0) ? "u" : (k == 1) ? "v" : "p";
}

static int read_or_die(void *ptr, size_t size, size_t nmemb, FILE *f, const char *what)
{
    size_t got = fread(ptr, size, nmemb, f);
    if (got != nmemb) {
        if (feof(f)) {
            fprintf(stderr, "Unexpected EOF while reading %s\n", what);
        } else {
            fprintf(stderr, "Read error while reading %s: %s\n", what, strerror(errno));
        }
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    FILE *f1, *f2;
    int imax, jmax, i, j;

    float *u1, *u2, *v1, *v2, *p1, *p2;
    unsigned char *flags1, *flags2;

    float epsilon = 1e-7f;
    float rel_epsilon = 1e-14f; /* default - ~machine epsilon */
    float rel_floor   = 1e-30f; /* avoids divide-by-zero */

    int mode = MODE_DIFF;
    int show_help = 0, show_usage = 0, show_version = 0;
    progname = argv[0];

    int optc;
    while ((optc = getopt_long(argc, argv, GETOPTS, long_opts, NULL)) != -1) {
        switch (optc) {
            case 'h': show_help = 1; break;
            case 'V': show_version = 1; break;
            case 'e': epsilon = atof(optarg); break;
            case 'r': rel_epsilon = atof(optarg); break;
            case 'm':
                if (strcasecmp(optarg, "diff") == 0) mode = MODE_DIFF;
                else if (strcasecmp(optarg, "plot-u") == 0) mode = MODE_OUTPUT_U;
                else if (strcasecmp(optarg, "plot-v") == 0) mode = MODE_OUTPUT_V;
                else if (strcasecmp(optarg, "plot-p") == 0) mode = MODE_OUTPUT_P;
                else if (strcasecmp(optarg, "plot-flags") == 0) mode = MODE_OUTPUT_FLAGS;
                else {
                    fprintf(stderr, "%s: Invalid mode '%s'\n", progname, optarg);
                    show_usage = 1;
                }
                break;
            default:
                show_usage = 1;
        }
    }

    if (show_version) {
        print_version();
        if (!show_help) return 0;
    }
    if (show_help) {
        print_help();
        return 0;
    }
    if (show_usage || optind != (argc - 2)) {
        print_usage();
        return 1;
    }

    if ((f1 = fopen(argv[optind], "rb")) == NULL) {
        fprintf(stderr, "Could not open '%s': %s\n", argv[optind], strerror(errno));
        return 1;
    }
    if ((f2 = fopen(argv[optind + 1], "rb")) == NULL) {
        fprintf(stderr, "Could not open '%s': %s\n", argv[optind + 1], strerror(errno));
        return 1;
    }

    if (!read_or_die(&imax, sizeof(int), 1, f1, "imax") ||
        !read_or_die(&jmax, sizeof(int), 1, f1, "jmax")) return 1;

    int i2, j2;
    if (!read_or_die(&i2, sizeof(int), 1, f2, "imax(file2)") ||
        !read_or_die(&j2, sizeof(int), 1, f2, "jmax(file2)")) return 1;

    if (i2 != imax || j2 != jmax) {
        printf("Number of cells differ! (%dx%d vs %dx%d)\n", imax, jmax, i2, j2);
        return 1;
    }

    float xlength1, ylength1, xlength2, ylength2;
    if (!read_or_die(&xlength1, sizeof(float), 1, f1, "xlength1") ||
        !read_or_die(&ylength1, sizeof(float), 1, f1, "ylength1") ||
        !read_or_die(&xlength2, sizeof(float), 1, f2, "xlength2") ||
        !read_or_die(&ylength2, sizeof(float), 1, f2, "ylength2")) return 1;

    if (xlength1 != xlength2 || ylength1 != ylength2) {
        printf("Image domain dimensions differ! (%gx%g vs %gx%g)\n",
               xlength1, ylength1, xlength2, ylength2);
        return 1;
    }

    u1 = malloc(sizeof(float) * (jmax + 2));
    u2 = malloc(sizeof(float) * (jmax + 2));
    v1 = malloc(sizeof(float) * (jmax + 2));
    v2 = malloc(sizeof(float) * (jmax + 2));
    p1 = malloc(sizeof(float) * (jmax + 2));
    p2 = malloc(sizeof(float) * (jmax + 2));
    flags1 = malloc((size_t)jmax + 2);
    flags2 = malloc((size_t)jmax + 2);
    if (!u1 || !u2 || !v1 || !v2 || !p1 || !p2 || !flags1 || !flags2) {
        fprintf(stderr, "Couldn't allocate enough memory.\n");
        return 1;
    }

    /* One overall max over u/v/p */
    float max_abs = 0.0f;
    int max_abs_i = -1, max_abs_j = -1, max_abs_var = -1;

    float max_rel = 0.0f;
    int max_rel_i = -1, max_rel_j = -1, max_rel_var = -1;

    int max_init = 0;

    int diff_found = 0;
    int invalid_found = 0;

    for (i = 0; i < imax + 2; i++) {
        if (!read_or_die(u1, sizeof(float), (size_t)jmax + 2, f1, "u1 row") ||
            !read_or_die(v1, sizeof(float), (size_t)jmax + 2, f1, "v1 row") ||
            !read_or_die(p1, sizeof(float), (size_t)jmax + 2, f1, "p1 row") ||
            !read_or_die(flags1, 1, (size_t)jmax + 2, f1, "flags1 row") ||
            !read_or_die(u2, sizeof(float), (size_t)jmax + 2, f2, "u2 row") ||
            !read_or_die(v2, sizeof(float), (size_t)jmax + 2, f2, "v2 row") ||
            !read_or_die(p2, sizeof(float), (size_t)jmax + 2, f2, "p2 row") ||
            !read_or_die(flags2, 1, (size_t)jmax + 2, f2, "flags2 row")) {
            return 1;
        }

        for (j = 0; j < jmax + 2; j++) {
            float du = u1[j] - u2[j];
            float dv = v1[j] - v2[j];
            float dp = p1[j] - p2[j];
            int dflags = (int)flags1[j] - (int)flags2[j];

            switch (mode) {
                case MODE_DIFF: {
                    /* ALWAYS initialize the max trackers on the first visited cell */
                    if (!max_init) {
                        max_abs = 0.0f;  max_abs_var = 0;  max_abs_i = i;  max_abs_j = j;
                        max_rel = 0.0f;  max_rel_var = 0;  max_rel_i = i;  max_rel_j = j;
                        max_init = 1;
                    }

                    /* If any diff is NaN/Inf, mark invalid but keep going so we still have maxima */
                    if (!isfinite(du) || !isfinite(dv) || !isfinite(dp)) {
                        invalid_found = 1;
                        diff_found = 1;
                        /* don’t use these values for maxima */
                        break;
                    }

                    float abs_u = fabsf(du);
                    float abs_v = fabsf(dv);
                    float abs_p = fabsf(dp);

                    float rel_u = rel_diff_f(u1[j], u2[j], rel_floor);
                    float rel_v = rel_diff_f(v1[j], v2[j], rel_floor);
                    float rel_p = rel_diff_f(p1[j], p2[j], rel_floor);

                    /* update ONE overall max abs */
                    if (abs_u > max_abs) { max_abs = abs_u; max_abs_i = i; max_abs_j = j; max_abs_var = 0; }
                    if (abs_v > max_abs) { max_abs = abs_v; max_abs_i = i; max_abs_j = j; max_abs_var = 1; }
                    if (abs_p > max_abs) { max_abs = abs_p; max_abs_i = i; max_abs_j = j; max_abs_var = 2; }

                    /* update ONE overall max rel */
                    if (rel_u > max_rel) { max_rel = rel_u; max_rel_i = i; max_rel_j = j; max_rel_var = 0; }
                    if (rel_v > max_rel) { max_rel = rel_v; max_rel_i = i; max_rel_j = j; max_rel_var = 1; }
                    if (rel_p > max_rel) { max_rel = rel_p; max_rel_i = i; max_rel_j = j; max_rel_var = 2; }

                    /* tolerance checks */
                    if (abs_u > epsilon || abs_v > epsilon || abs_p > epsilon || dflags != 0) {
                        diff_found = 1;
                    }
                    if (rel_epsilon > 0.0f) {
                        if (rel_u > rel_epsilon || rel_v > rel_epsilon || rel_p > rel_epsilon) {
                            diff_found = 1;
                        }
                    }
                    break;
                }

                case MODE_OUTPUT_U:
                    printf("%g%c", du, (j == jmax + 1) ? '\n' : ' ');
                    break;
                case MODE_OUTPUT_V:
                    printf("%g%c", dv, (j == jmax + 1) ? '\n' : ' ');
                    break;
                case MODE_OUTPUT_P:
                    printf("%g%c", dp, (j == jmax + 1) ? '\n' : ' ');
                    break;
                case MODE_OUTPUT_FLAGS:
                    printf("%d%c", dflags, (j == jmax + 1) ? '\n' : ' ');
                    break;
            }
        }
    }

    if (mode == MODE_DIFF) {
        printf("Max abs diff: %s=%g at (%d,%d)\n",
               (max_abs_var >= 0 ? var_name(max_abs_var) : "?"),
               max_abs, max_abs_i, max_abs_j);

        printf("Max rel diff: %s=%g at (%d,%d)\n",
               (max_rel_var >= 0 ? var_name(max_rel_var) : "?"),
               max_rel, max_rel_i, max_rel_j);

        if (invalid_found) {
            printf("Warning: NaN/Inf encountered in differences.\n");
        }
    }

    if (diff_found) {
        printf("Files differ.\n");
        return 1;
    }

    if (mode == MODE_DIFF) {
        printf("Files identical.\n");
    }
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
    fprintf(stderr, "%s. A utility to compare karman state files. Extended version checks absolute and relative differences \nand reports maximum differences and its grid location.\n\n", PACKAGE);
    fprintf(stderr, "Usage %s [OPTIONS] FILE1 FILE2\n\n", progname);
    fprintf(stderr, "  -h, --help                 Print a summary of the options\n");
    fprintf(stderr, "  -V, --version              Print the version number\n");
    fprintf(stderr, "  -e, --epsilon=EPSILON      Set epsilon: max allowed absolute difference\n");
    fprintf(stderr, "  -r, --rel-epsilon=RELEPS   Set relative tolerance \n");
    fprintf(stderr, "  -m, --mode=MODE            'diff', 'plot-u', 'plot-v', 'plot-p', 'plot-flags'\n");
}
