/* === tail_n_lines.c ===
 * Prints last N lines of a text file.
 * gcc -Wall -Wextra -std=c11 tail_n_lines.c -o tail_n_lines
 */
#include "util.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s file N\n", argv[0]); return 1; }
    const char *path = argv[1];
    int N = atoi(argv[2]); if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }
    FILE *f = fopen(path, "r"); if (!f) ERR("fopen");
    char **buf = malloc(sizeof(char*) * N); if (!buf) ERR("malloc");
    for (int i=0;i<N;i++) buf[i]=NULL;
    size_t len = 0; char *line = NULL; ssize_t read; int idx=0, cnt=0;
    while ((read = getline(&line, &len, f)) != -1) {
        free(buf[idx]); buf[idx]=strdup(line); idx=(idx+1)%N; if (cnt<N) cnt++;
    }
    int start = (cnt < N) ? 0 : idx;
    for (int i=0;i<cnt;i++) { int j=(start+i)%N; fputs(buf[j], stdout); free(buf[j]); }
    free(buf); free(line); fclose(f); return 0;
}
