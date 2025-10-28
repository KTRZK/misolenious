/* === group_by_extension.c ===
 * Group files by extension and sum sizes.
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 group_by_extension.c -o group_by_extension
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

struct extstat { char *ext; long long count; long long size; struct extstat *next; };
static struct extstat *exts = NULL;

void add_ext(const char *ext, long long size) {
    struct extstat *p = exts;
    while (p) { if ((ext && p->ext && strcmp(p->ext, ext)==0) || (!ext && !p->ext)) { p->count++; p->size += size; return; } p = p->next; }
    struct extstat *n = malloc(sizeof(*n));
    if (!n) ERR("malloc");
    n->ext = ext ? strdup(ext) : NULL;
    n->count = 1; n->size = size; n->next = exts; exts = n;
}

int ext_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void)typeflag; (void)ftw;
    if (!sb) return 0;
    if (!S_ISREG(sb->st_mode)) return 0;
    const char *base = strrchr(fpath, '/'); base = base ? base+1 : fpath;
    const char *dot = strrchr(base, '.');
    if (!dot) add_ext(NULL, sb->st_size);
    else add_ext(dot+1, sb->st_size);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    if (nftw(argv[1], ext_cb, 20, FTW_PHYS) != 0) ERR("nftw");
    struct extstat *p = exts;
    printf("ext\tcount\tsize\n");
    while (p) { printf("%s\t%lld\t%lld\n", p->ext ? p->ext : "(none)", p->count, p->size); p = p->next; }
    return 0;
}
