/* === find_broken_symlinks.c ===
 * Find broken symbolic links in tree.
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 find_broken_symlinks.c -o find_broken_symlinks
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <sys/stat.h>

int cb_broken(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void)sb; (void)ftw;
    if (typeflag == FTW_SL) {
        char *target = readlink_alloc(fpath);
        char *resolved = realpath_dup(fpath);
        if (!resolved) {
            printf("BROKEN: %s -> %s\n", fpath, target ? target : "(?)");
        }
        free(target);
        free(resolved);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    for (int i = 1; i < argc; ++i) {
        if (nftw(argv[i], cb_broken, 20, FTW_PHYS) != 0) fprintf(stderr, "Error scanning %s\n", argv[i]);
    }
    return 0;
}
