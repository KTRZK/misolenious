/* === safe_temp_write.c ===
 * Create secure temp file and write argument.
 * gcc -Wall -Wextra -std=c11 safe_temp_write.c -o safe_temp_write
 */
#include "util.h"
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s text\n", argv[0]); return 1; }
    char tmpl[] = "/tmp/examtmp.XXXXXX";
    int fd = mkstemp(tmpl); if (fd == -1) ERR("mkstemp");
    printf("Temp file: %s\n", tmpl);
    const char *s = argv[1];
    if (write(fd, s, strlen(s)) != (ssize_t)strlen(s)) { close(fd); unlink(tmpl); ERR("write"); }
    close(fd);
    return 0;
}
