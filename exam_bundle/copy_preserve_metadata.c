/* === copy_preserve_metadata.c ===
 * Copy file preserving metadata where possible.
 * gcc -Wall -Wextra -std=c11 copy_preserve_metadata.c -o copy_preserve_metadata
 */
#include "util.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF 8192

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s src dst\n", argv[0]); return 1; }
    const char *src = argv[1], *dst = argv[2];
    int fd_src = open(src, O_RDONLY);
    if (fd_src == -1) ERR("open src");
    struct stat st;
    if (fstat(fd_src, &st) == -1) { close(fd_src); ERR("fstat"); }
    int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 07777);
    if (fd_dst == -1) { close(fd_src); ERR("open dst"); }
    char buf[BUF];
    ssize_t r;
    while ((r = TEMP_FAILURE_RETRY(read(fd_src, buf, BUF))) > 0) {
        if (bulk_write(fd_dst, buf, (size_t)r) == -1) { close(fd_src); close(fd_dst); ERR("write"); }
    }
    if (r < 0) { close(fd_src); close(fd_dst); ERR("read"); }
    if (fchmod(fd_dst, st.st_mode & 07777) == -1) ;
    if (fchown(fd_dst, st.st_uid, st.st_gid) == -1 && errno != EPERM) ;
#ifdef __linux__
    struct timespec times[2];
    times[0] = st.st_atim; times[1] = st.st_mtim;
    futimens(fd_dst, times);
#endif
    if (close(fd_src) == -1) ERR("close src");
    if (close(fd_dst) == -1) ERR("close dst");
    return 0;
}
