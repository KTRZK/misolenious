/* === read_range.c ===
 * Read a byte range from file and output to stdout.
 * gcc -Wall -Wextra -std=c11 read_range.c -o read_range
 */
#include "util.h"
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Usage: %s file offset len\n", argv[0]); return 1; }
    const char *path = argv[1]; off_t offset = atoll(argv[2]); size_t len = (size_t)atoll(argv[3]);
    int fd = open(path, O_RDONLY); if (fd == -1) ERR("open");
    if (lseek(fd, offset, SEEK_SET) == -1) { close(fd); ERR("lseek"); }
    char *buf = malloc(len); if (!buf) ERR("malloc");
    ssize_t r = bulk_read(fd, buf, len);
    if (r < 0) { free(buf); close(fd); ERR("read"); }
    write(STDOUT_FILENO, buf, r);
    free(buf); close(fd); return 0;
}
