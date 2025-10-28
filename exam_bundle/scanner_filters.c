/* === scanner_filters.c ===
 * Filtered recursive scanner: -p path (multi), -d depth, -e ext, -o out
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 scanner_filters.c -o scanner_filters
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <getopt.h>

#define MAXFD 20
static FILE *out = NULL;
static int max_depth = -1;
static const char *ext = NULL;
static const char *current_root = NULL;

int walk_fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void) sb;
    if (typeflag == FTW_F) {
        if (max_depth >= 0 && ftwbuf->level > max_depth) return 0;
        const char *name = fpath + ftwbuf->base;
        if (!ext || (strrchr(name, '.') && strcmp(strrchr(name, '.') + 1, ext) == 0)) {
            fprintf(out, "path: %s\n%s %lld\n", current_root, name, (long long)(sb ? sb->st_size : 0));
        }
    }
    return 0;
}

void usage(const char *p) { fprintf(stderr, "Usage: %s -p path [-p path ...] [-d depth] [-e ext] [-o out]\n", p); exit(EXIT_FAILURE); }

int main(int argc, char **argv) {
    char *outs = NULL; int opt; char *paths[256]; int pc = 0;
    while ((opt = getopt(argc, argv, "p:d:e:o:")) != -1) {
        switch (opt) {
            case 'p': paths[pc++] = optarg; break;
            case 'd': max_depth = atoi(optarg); break;
            case 'e': ext = optarg; break;
            case 'o': outs = optarg; break;
            default: usage(argv[0]);
        }
    }
    if (pc == 0) usage(argv[0]);
    out = stdout;
    if (outs) { out = fopen(outs, "w"); if (!out) ERR("fopen out"); }
    for (int i = 0; i < pc; ++i) {
        current_root = paths[i];
        fprintf(out, "path: %s\n", paths[i]);
        if (nftw(paths[i], walk_fn, MAXFD, FTW_PHYS) != 0)
            fprintf(stderr, "Error scanning %s\n", paths[i]);
    }
    if (outs) fclose(out);
    return 0;
}
