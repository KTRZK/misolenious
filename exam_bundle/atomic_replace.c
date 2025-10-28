/* === atomic_replace.c ===
 * Atomic replace using mkstemp + rename.
 * gcc -Wall -Wextra -std=c11 atomic_replace.c -o atomic_replace
 */
#include "util.h"
#include <fcntl.h>

int atomic_write(const char *path, const char *data, size_t len) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmpXXXXXX", path);
    int fd = mkstemp(tmp); if (fd==-1) return -1;
    if (write(fd, data, len) != (ssize_t)len) { close(fd); unlink(tmp); return -1; }
    if (close(fd) == -1) { unlink(tmp); return -1; }
    if (rename(tmp, path) == -1) { unlink(tmp); return -1; }
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s target \"text\"\n", argv[0]); return 1; }
    if (atomic_write(argv[1], argv[2], strlen(argv[2])) != 0) ERR("atomic_write");
    return 0;
}
