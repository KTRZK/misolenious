/* === writev_example.c ===
 * Demonstrates writev usage.
 * gcc -Wall -Wextra -std=c11 writev_example.c -o writev_example
 */
#include "util.h"
#include <sys/uio.h>
#include <fcntl.h>

int main(void) {
    int fd = open("writev_out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) ERR("open");
    struct iovec iov[3];
    char *a = "Line1: ";
    char *b = "Hello writev!";
    char *c = "\n";
    iov[0].iov_base = a; iov[0].iov_len = strlen(a);
    iov[1].iov_base = b; iov[1].iov_len = strlen(b);
    iov[2].iov_base = c; iov[2].iov_len = 1;
    if (writev(fd, iov, 3) == -1) ERR("writev");
    if (close(fd) == -1) ERR("close");
    return 0;
}
