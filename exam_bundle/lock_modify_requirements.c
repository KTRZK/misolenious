/* === lock_modify_requirements.c ===
 * Add/remove line in requirements file using flock to lock.
 * gcc -Wall -Wextra -std=c11 lock_modify_requirements.c -o lock_modify_requirements
 */
#include "util.h"
#include <sys/file.h>
#include <fcntl.h>

int append_if_missing_locked(int fd, const char *line) {
    if (flock(fd, LOCK_EX) != 0) return -1;
    FILE *f = fdopen(dup(fd), "r+");
    if (!f) { flock(fd, LOCK_UN); return -1; }
    char buf[512]; rewind(f);
    while (fgets(buf, sizeof(buf), f)) if (strncmp(buf, line, strlen(line)) == 0) { fclose(f); flock(fd, LOCK_UN); return 0; }
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s\n", line);
    fclose(f);
    flock(fd, LOCK_UN);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Usage: %s add|remove file line_or_prefix\n", argv[0]); return 1; }
    const char *op = argv[1], *path = argv[2], *arg = argv[3];
    if (strcmp(op, "add") == 0) {
        int fd = open(path, O_RDWR | O_CREAT, 0666);
        if (fd == -1) ERR("open req");
        int r = append_if_missing_locked(fd, arg);
        close(fd);
        if (r < 0) ERR("append_if_missing_locked");
        return 0;
    } else if (strcmp(op, "remove") == 0) {
        int fd = open(path, O_RDONLY);
        if (fd == -1) ERR("open req read");
        if (flock(fd, LOCK_EX) != 0) { close(fd); ERR("flock"); }
        FILE *f = fdopen(fd, "r");
        char tmp[4096]; snprintf(tmp, sizeof(tmp), "%s.tmp", path);
        FILE *t = fopen(tmp, "w"); if (!t) { fclose(f); unlink(tmp); ERR("fopen tmp"); }
        char linebuf[512]; int removed = 0;
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strncmp(linebuf, arg, strlen(arg)) == 0) { removed = 1; continue; }
            fputs(linebuf, t);
        }
        fclose(t);
        fclose(f);
        if (rename(tmp, path) != 0) { unlink(tmp); ERR("rename"); }
        return removed ? 0 : 1;
    } else { fprintf(stderr, "Unknown op\n"); return 1; }
}
