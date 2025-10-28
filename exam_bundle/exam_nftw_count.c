/* === exam_nftw_count.c ===
 * Recursive count using nftw.
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 exam_nftw_count.c -o exam_nftw_count
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <sys/stat.h>

static int files_g, dirs_g, links_g, other_g;

int walk_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)fpath; (void)sb; (void)ftwbuf;
    switch (typeflag) {
        case FTW_DNR:
        case FTW_D: dirs_g++; break;
        case FTW_F: files_g++; break;
        case FTW_SL: links_g++; break;
        default: other_g++;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s path [path...]\n", argv[0]); return 1; }
    for (int i = 1; i < argc; ++i) {
        files_g = dirs_g = links_g = other_g = 0;
        if (nftw(argv[i], walk_cb, 20, FTW_PHYS) != 0) {
            fprintf(stderr, "Error scanning %s\n", argv[i]);
            continue;
        }
        printf("%s:\n files:%d\n dirs:%d\n links:%d\n other:%d\n", argv[i], files_g, dirs_g, links_g, other_g);
    }
    return 0;
}
