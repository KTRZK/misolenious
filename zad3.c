/* === l1_3_fixed.c ===
* Rekurencyjne zliczanie plików/katalogów/linków/innych przy użyciu nftw.
 * Kompilacja:
 *   gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 l1_3_fixed.c util.h -o l1_3_fixed
 *
 * Użycie:
 *   ./l1_3_fixed dir1 dir2 ...
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/* maksymalna liczba otwartych deskryptorów podczas przeszukania (bezpieczne domyślne) */
#ifndef MAXFD_LIMIT
#define MAXFD_LIMIT 20
#endif

static int files_cnt, dirs_cnt, links_cnt, other_cnt;

static int nftw_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void) fpath; (void) sb; (void) ftwbuf;
    switch (typeflag) {
    case FTW_DNR: /* katalog nie mógł być otwarty */
    case FTW_D: dirs_cnt++; break;
    case FTW_F: files_cnt++; break;
    case FTW_SL: links_cnt++; break;
    default: other_cnt++;
    }
    return 0; /* kontynuuj */
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path...]\n", argv[0]);
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        files_cnt = dirs_cnt = links_cnt = other_cnt = 0;
        const char *root = argv[i];
        int res = nftw(root, nftw_callback, MAXFD_LIMIT, FTW_PHYS);
        if (res != 0) {
            fprintf(stderr, "%s: nftw failed (return %d): %s\n", root, res, strerror(errno));
            continue;
        }
        printf("%s:\nfiles: %d\ndirs: %d\nlinks: %d\nother: %d\n", root, files_cnt, dirs_cnt, links_cnt, other_cnt);
    }
    return 0;
}
