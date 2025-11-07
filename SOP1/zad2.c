/* === l1_2_fixed.c ===
 * Dla katalogów podanych jako argumenty zlicza obiekty (wywołuje scan w katalogu).
 * Kompilacja:
 *   gcc -Wall -Wextra -std=c11 l1_2_fixed.c util.h -o l1_2_fixed
 *
 * Użycie:
 *   ./l1_2_fixed dir1 dir2 ...
 *
 * Uwaga: program pamięta bieżący katalog i wraca do niego po przetworzeniu każdego parametru.
 */

#include "util.h"
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Deklaracja funkcji z l1_1_fixed.c - tutaj zakładamy, że ta funkcja jest dostępna
   (np. przez skompilowanie obu plików razem lub dołączenie implementacji).
   Dla samodzielnej kompilacji można skopiować implementację scan_dir_one do tego pliku. */
int scan_dir_one(const char *dirpath, int *files, int *dirs, int *links, int *other);

/* Bezpieczne pobranie bieżącego katalogu, alokuje bufor (free po użyciu) */
char *safe_getcwd(void) {
    size_t sz = 1024;
    char *buf = NULL;
    for (;;) {
        char *n = realloc(buf, sz);
        if (!n) { free(buf); return NULL; }
        buf = n;
        if (getcwd(buf, sz) != NULL) return buf;
        if (errno != ERANGE) { free(buf); return NULL; }
        sz *= 2;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s path [path...]\n", argv[0]);
        return 1;
    }

    char *orig = safe_getcwd();
    if (!orig) ERR("getcwd");

    for (int i = 1; i < argc; ++i) {
        const char *target = argv[i];
        if (chdir(target) != 0) {
            fprintf(stderr, "Cannot chdir to %s: %s\n", target, strerror(errno));
            continue; /* nie przerywamy dla pozostałych katalogów */
        }

        printf("%s:\n", target);
        int files=0, dirs=0, links=0, other=0;
        /* scan the current directory using "." */
        if (scan_dir_one(".", &files, &dirs, &links, &other) != 0) {
            fprintf(stderr, "Error scanning %s: %s\n", target, strerror(errno));
        } else {
            printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", files, dirs, links, other);
        }

        /* wróć do oryginalnego katalogu */
        if (chdir(orig) != 0) {
            /* krytyczny błąd - nie możemy wrócić */
            free(orig);
            ERR("chdir back");
        }
    }

    free(orig);
    return 0;
}
