#ifndef UTIL_H
#define UTIL_H

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

/* ERR - print perror and exit */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* TEMP_FAILURE_RETRY fallback */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
    (__extension__ ({ long int __result; \
       do __result = (long int)(expression); \
       while (__result == -1L && errno == EINTR); \
       __result; }))
#endif

static inline void *safe_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) ERR("malloc");
    return p;
}

static inline char *safe_strdup(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s);
    if (!r) ERR("strdup");
    return r;
}

/* bulk_read / bulk_write */
static inline ssize_t bulk_read(int fd, void *buf, size_t count) {
    size_t left = count;
    char *ptr = buf;
    while (left > 0) {
        ssize_t r = TEMP_FAILURE_RETRY(read(fd, ptr, left));
        if (r < 0) return -1;
        if (r == 0) return (ssize_t)(count - left);
        left -= r; ptr += r;
    }
    return (ssize_t)count;
}
static inline ssize_t bulk_write(int fd, const void *buf, size_t count) {
    size_t left = count;
    const char *ptr = buf;
    while (left > 0) {
        ssize_t w = TEMP_FAILURE_RETRY(write(fd, ptr, left));
        if (w < 0) return -1;
        left -= w; ptr += w;
    }
    return (ssize_t)count;
}

/* readlink_alloc: readlink with dynamic buffer */
static inline char *readlink_alloc(const char *path) {
    ssize_t bufsize = 128;
    for (;;) {
        char *buf = malloc((size_t)bufsize);
        if (!buf) return NULL;
        ssize_t r = readlink(path, buf, (size_t)bufsize);
        if (r < 0) { free(buf); return NULL; }
        if (r < bufsize) { buf[r] = '\0'; return buf; }
        free(buf);
        bufsize *= 2;
    }
}

/* realpath_dup: wrapper to realpath that allocates */
static inline char *realpath_dup(const char *path) {
    char *res = realpath(path, NULL);
    return res;
}

/* atomic_write_file: write data to tmp and rename */
static inline int atomic_write_file(const char *path, const void *buf, size_t len, mode_t mode) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmpXXXXXX", path);
    int fd = mkstemp(tmp);
    if (fd == -1) return -1;
    if (fchmod(fd, mode) == -1) { close(fd); unlink(tmp); return -1; }
    if (bulk_write(fd, buf, len) != (ssize_t)len) { close(fd); unlink(tmp); return -1; }
    if (close(fd) == -1) { unlink(tmp); return -1; }
    if (rename(tmp, path) == -1) { unlink(tmp); return -1; }
    return 0;
}

#endif /* UTIL_H */
