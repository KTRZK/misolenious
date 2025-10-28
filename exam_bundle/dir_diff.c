/* === dir_diff.c ===
 * Compare two directory trees for file presence and sizes.
 * gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 dir_diff.c -o dir_diff
 */
#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

struct entry { char *path; off_t size; struct entry *next; };
static struct entry *A = NULL, *B = NULL;
static const char *rootA, *rootB;

void add_entry(struct entry **list, const char *root, const char *full, off_t size) {
    const char *rel = full + strlen(root);
    if (*rel == '/') ++rel;
    struct entry *e = malloc(sizeof(*e)); e->path = strdup(rel); e->size = size; e->next = *list; *list = e;
}
int cbA(const char *fpath, const struct stat *sb, int t, struct FTW *ftw) { (void)t; (void)ftw; if (sb && S_ISREG(sb->st_mode)) add_entry(&A, rootA, fpath, sb->st_size); return 0; }
int cbB(const char *fpath, const struct stat *sb, int t, struct FTW *ftw) { (void)t; (void)ftw; if (sb && S_ISREG(sb->st_mode)) add_entry(&B, rootB, fpath, sb, sb->st_size); return 0; }
struct entry *find_entry(struct entry *list, const char *path) { for (; list; list=list->next) if (strcmp(list->path, path)==0) return list; return NULL; }

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s A B\n", argv[0]); return 1; }
    rootA = argv[1]; rootB = argv[2];
    if (nftw(rootA, cbA, 20, FTW_PHYS) != 0) ERR("nftw A");
    if (nftw(rootB, cbB, 20, FTW_PHYS) != 0) ERR("nftw B");
    for (struct entry *p=A; p; p=p->next) {
        struct entry *q = find_entry(B, p->path);
        if (!q) printf("ONLY IN A: %s (size %lld)\n", p->path, (long long)p->size);
        else if (p->size != q->size) printf("DIFF SIZE: %s A=%lld B=%lld\n", p->path, (long long)p->size, (long long)q->size);
    }
    for (struct entry *p=B; p; p=p->next) if (!find_entry(A, p->path)) printf("ONLY IN B: %s (size %lld)\n", p->path, (long long)p->size);
    return 0;
}
