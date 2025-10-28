/* === recursive_delete.c ===
 * Remove directory tree recursively using nftw.
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 recursive_delete.c -o recursive_delete
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

static int rm_err = 0;
int rm_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void)sb; (void)typeflag; (void)ftw;
    if (remove(fpath) != 0) { rm_err = errno; return -1; }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    if (nftw(argv[1], rm_cb, 20, FTW_DEPTH | FTW_PHYS) != 0) {
        fprintf(stderr, "Failed to remove %s: %s\n", argv[1], strerror(rm_err ? rm_err : errno));
        return 1;
    }
    return 0;
}
