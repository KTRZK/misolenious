/* === make_file.c ===
 * Create file with given name, perms, size; ~10% random letters, rest zeros (ftruncate+pwrite).
 * gcc -Wall -Wextra -std=c11 make_file.c -o make_file
 */
#include "util.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void usage(const char *p) { fprintf(stderr, "USAGE: %s -n NAME -p OCTAL -s SIZE\n", p); exit(EXIT_FAILURE); }

void make_file(const char *name, off_t size, mode_t perms, int percent) {
    mode_t old = umask(~perms & 0777);
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    umask(old);
    if (fd == -1) ERR("open");
    if (ftruncate(fd, size) == -1) { close(fd); ERR("ftruncate"); }
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    for (off_t i = 0; i < (size * percent) / 100; ++i) {
        char ch = 'A' + (rand() % ('Z' - 'A' + 1));
        off_t pos = (size > 0) ? (rand() % size) : 0;
        if (pwrite(fd, &ch, 1, pos) != 1) { close(fd); ERR("pwrite"); }
    }
    if (close(fd) == -1) ERR("close");
}

int main(int argc, char **argv) {
    int opt; char *name = NULL; mode_t perms = (mode_t)-1; off_t size = -1;
    while ((opt = getopt(argc, argv, "n:p:s:")) != -1) {
        switch (opt) {
            case 'n': name = optarg; break;
            case 'p': perms = (mode_t)strtol(optarg, NULL, 8); break;
            case 's': size = (off_t)strtoll(optarg, NULL, 10); break;
            default: usage(argv[0]);
        }
    }
    if (!name || perms == (mode_t)-1 || size < 0) usage(argv[0]);
    if (unlink(name) && errno != ENOENT) ERR("unlink");
    make_file(name, size, perms, 10);
    return 0;
}
