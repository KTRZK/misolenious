/* === copy_lowlevel.c ===
 * Low-level file copy using open/read/write.
 * gcc -Wall -Wextra -std=c11 copy_lowlevel.c -o copy_lowlevel
 */
#include "util.h"
#include <fcntl.h>
#include <unistd.h>

#define FILE_BUF_LEN 4096

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s src dst\n", argv[0]); return 1; }
    const char *src = argv[1], *dst = argv[2];
    int fd1 = open(src, O_RDONLY);
    if (fd1 == -1) ERR("open src");
    int fd2 = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 == -1) { close(fd1); ERR("open dst"); }
    char buf[FILE_BUF_LEN];
    for (;;) {
        ssize_t r = TEMP_FAILURE_RETRY(read(fd1, buf, FILE_BUF_LEN));
        if (r < 0) { close(fd1); close(fd2); ERR("read"); }
        if (r == 0) break;
        if (bulk_write(fd2, buf, (size_t)r) == -1) { close(fd1); close(fd2); ERR("write"); }
    }
    if (close(fd1) == -1) ERR("close src");
    if (close(fd2) == -1) ERR("close dst");
    return 0;
}
