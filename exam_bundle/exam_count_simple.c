/* === exam_count_simple.c ===
 * Counts files, dirs, links, other in a single directory (non-recursive).
 * gcc -Wall -Wextra -std=c11 exam_count_simple.c -o exam_count_simple
 */
#include "util.h"
#include <dirent.h>
#include <sys/stat.h>

int count_dir_once(const char *dirpath, int *files, int *dirs, int *links, int *other) {
    DIR *dirp = opendir(dirpath);
    if (!dirp) return -1;
    struct dirent *dp;
    struct stat st;
    *files = *dirs = *links = *other = 0;
    char full[4096];
    while (1) {
        errno = 0;
        dp = readdir(dirp);
        if (dp == NULL) break;
        if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0) continue;
        snprintf(full, sizeof(full), "%s/%s", dirpath, dp->d_name);
        if (lstat(full, &st) != 0) { closedir(dirp); return -1; }
        if (S_ISDIR(st.st_mode)) (*dirs)++;
        else if (S_ISREG(st.st_mode)) (*files)++;
        else if (S_ISLNK(st.st_mode)) (*links)++;
        else (*other)++;
    }
    if (errno != 0) { closedir(dirp); return -1; }
    if (closedir(dirp) != 0) return -1;
    return 0;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : ".";
    int files, dirs, links, other;
    if (count_dir_once(path, &files, &dirs, &links, &other) != 0) ERR("count_dir_once");
    printf("%s: Files=%d Dirs=%d Links=%d Other=%d\n", path, files, dirs, links, other);
    return 0;
}
