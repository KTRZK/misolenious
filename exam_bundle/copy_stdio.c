/* === copy_stdio.c ===
 * Copy using fopen/fread/fwrite.
 * gcc -Wall -Wextra -std=c11 copy_stdio.c -o copy_stdio
 */
#include "util.h"
#include <stdio.h>

#define BUFSIZE 8192

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s src dst\n", argv[0]); return 1; }
    FILE *in = fopen(argv[1], "rb");
    if (!in) ERR("fopen in");
    FILE *out = fopen(argv[2], "wb");
    if (!out) { fclose(in); ERR("fopen out"); }
    char buf[BUFSIZE];
    size_t r;
    while ((r = fread(buf, 1, BUFSIZE, in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) ERR("fwrite");
    }
    if (ferror(in)) ERR("fread");
    if (fclose(in)) ERR("fclose in");
    if (fclose(out)) ERR("fclose out");
    return 0;
}
