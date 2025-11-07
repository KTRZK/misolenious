/* === l1_1_fixed.c ===
 * Zlicza pliki, katalogi, linki i inne obiekty w wskazanym katalogu (bez rekurencji).
 * Kompilacja:
 *   gcc -Wall -Wextra -std=c11 l1_1_fixed.c util.h -o l1_1_fixed
 *
 * Użycie:
 *   ./l1_1_fixed            # skanuje bieżący katalog
 *   ./l1_1_fixed /tmp       # skanuje /tmp
 *
 * Wymaga: util.h (ERR, TEMP_FAILURE_RETRY, bulk helpers) - dostarczone w exam_bundle.
 */

#include "util.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int scan_dir_one(const char *dirpath, int *files, int *dirs, int *links, int *other) {
    DIR *dirp = opendir(dirpath);
    if (!dirp) return -1;
    struct dirent *dp;
    struct stat st;
    char fullpath[4096];
    *files = *dirs = *links = *other = 0;

    while (1) {
        errno = 0; /* konieczne: sprawdzamy NULL zwrócone przez readdir */
        dp = readdir(dirp);
        if (dp == NULL) break;
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;

        /* zbuduj pełną ścieżkę do pliku */
        if ((size_t)snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, dp->d_name) >= sizeof(fullpath)) {
            /* nazwa zbyt długa - traktujemy jako 'other' i pomijamy bez crasha */
            (*other)++;
            continue;
        }

        if (lstat(fullpath, &st) != 0) {
            /* błąd lstat - zwolnij i przekaż błąd */
            closedir(dirp);
            return -1;
        }

        if (S_ISDIR(st.st_mode)) (*dirs)++;
        else if (S_ISREG(st.st_mode)) (*files)++;
        else if (S_ISLNK(st.st_mode)) (*links)++;
        else (*other)++;
    }

    if (errno != 0) { /* readdir zwróciło NULL z powodu błędu */
        closedir(dirp);
        return -1;
    }

    if (closedir(dirp) != 0) return -1;
    return 0;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : ".";
    int files, dirs, links, other;
    if (scan_dir_one(path, &files, &dirs, &links, &other) != 0) ERR("scan_dir_one");
    printf("Path: %s\nFiles: %d, Dirs: %d, Links: %d, Other: %d\n", path, files, dirs, links, other);
    return EXIT_SUCCESS;
}
