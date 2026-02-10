/* === listing_program.c ===
 * Simple listing: -p path (multi) -o outfile
 * gcc -Wall -Wextra -std=c11 listing_program.c -o listing_program
 */
#include "util.h"
#include <dirent.h>
#include <sys/stat.h>
#include <getopt.h>

void list_one_dir(const char *path, FILE *out) {
    DIR *d = opendir(path);
    if (!d) { fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno)); return; }
    fprintf(out, "PATH:\n%s\nLISTA PLIKÓW:\n", path);
    struct dirent *dp; struct stat st; char fullname[4096];
    while ((dp = readdir(d)) != NULL) {
        if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0) continue;
        snprintf(fullname, sizeof(fullname), "%s/%s", path, dp->d_name);
        if (lstat(fullname, &st) == -1) { fprintf(stderr, "lstat %s failed: %s\n", fullname, strerror(errno)); continue; }
        fprintf(out, "%s %lld\n", dp->d_name, (long long) st.st_size);
    }
    closedir(d);
}

int main(int argc, char **argv) {
    char *outfile = NULL; int opt; char *paths[256]; int paths_cnt = 0;
    while ((opt = getopt(argc, argv, "p:o:")) != -1) {
        switch (opt) {
            case 'p': paths[paths_cnt++] = optarg; break;
            case 'o': outfile = optarg; break;
            default: fprintf(stderr, "Usage: %s -p PATH [-p PATH ...] [-o OUTPUT]\n", argv[0]); exit(EXIT_FAILURE);
        }
    }
    if (paths_cnt == 0) { fprintf(stderr, "At least one -p PATH required\n"); exit(EXIT_FAILURE); }
    FILE *out = stdout;
    if (outfile) { out = fopen(outfile, "w"); if (!out) ERR("fopen outfile"); }
    for (int i = 0; i < paths_cnt; ++i) list_one_dir(paths[i], out);
    if (outfile) fclose(out);
    return 0;
}
