/*
Kompilacja:
gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 find_broken_symlinks.c -o find_broken_symlinks
 */

/* === NAZWA: find_broken_symlinks.c ===
 * Znajduje złamane linki symboliczne w drzewie katalogów (nftw).
 * Kompilacja: see above
 * Użycie: ./find_broken_symlinks /path
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <sys/stat.h>

int cb_broken(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void)sb; (void)ftw;
    if (typeflag == FTW_SL) {
        /* jeśli link, sprawdź czy docelowy plik istnieje */
        char *target = readlink_alloc(fpath);
        if (!target) {
            printf("BROKEN (cannot readlink): %s\n", fpath);
            return 0;
        }
        char *resolved = realpath_dup(fpath); /* realpath on link resolves target if exists */
        if (!resolved) {
            printf("BROKEN: %s -> %s\n", fpath, target);
        }
        free(target);
        free(resolved);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    for (int i = 1; i < argc; ++i) {
        if (nftw(argv[i], cb_broken, 20, FTW_PHYS) != 0) {
            fprintf(stderr, "Error scanning %s\n", argv[i]);
        }
    }
    return 0;
}



/* Kompilacja
gcc -Wall -Wextra -std=c11 copy_preserve_metadata.c -o copy_preserve_metadata
 */

/* === NAZWA: copy_preserve_metadata.c ===
 * Kopiuje plik zachowując metadane (mode, owner, times).
 * Użycie: ./copy_preserve_metadata src dst
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

    /* ustaw tryb (ponownie, bo open z maską może przyciąć), właściciela i czasy */
    if (fchmod(fd_dst, st.st_mode & 07777) == -1) /* ignore non-fatal */ ;
    if (fchown(fd_dst, st.st_uid, st.st_gid) == -1 && errno != EPERM) /* ignore if not permitted */ ;
    struct timespec times[2];
#ifdef __linux__
    times[0] = st.st_atim; times[1] = st.st_mtim;
    futimens(fd_dst, times); /* ignore errors */
#endif

    if (close(fd_src) == -1) ERR("close src");
    if (close(fd_dst) == -1) ERR("close dst");
    return 0;
}

/*
Kompilacja
gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 recursive_delete.c -o recursive_delete
 */

/* === NAZWA: recursive_delete.c ===
 * Rekurencyjne usuwanie katalogu i zawartości (używa nftw z FTW_DEPTH).
 * Użycie: ./recursive_delete path
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

static int rm_err = 0;
int rm_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void)sb; (void)typeflag; (void)ftw;
    if (remove(fpath) != 0) { rm_err = errno; return -1; }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    if (nftw(argv[1], rm_cb, 20, FTW_DEPTH | FTW_PHYS) != 0) {
        fprintf(stderr, "Failed to remove %s: %s\n", argv[1], strerror(rm_err ? rm_err : errno));
        return 1;
    }
    return 0;
}

/*
kompilacja
gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 group_by_extension.c -o group_by_extension
 */

/* === NAZWA: group_by_extension.c ===
 * Dla podanego drzewa (nftw) zlicza ile plików każdego rozszerzenia i sumuje ich rozmiary.
 * Użycie: ./group_by_extension /path
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <string.h>

struct extstat { char *ext; long long count; long long size; struct extstat *next; };
static struct extstat *exts = NULL;

void add_ext(const char *ext, long long size) {
    struct extstat *p = exts;
    while (p) { if ((ext && p->ext && strcmp(p->ext, ext)==0) || (!ext && !p->ext)) { p->count++; p->size += size; return; } p = p->next; }
    struct extstat *n = malloc(sizeof(*n));
    if (!n) ERR("malloc");
    n->ext = ext ? strdup(ext) : NULL;
    n->count = 1; n->size = size; n->next = exts; exts = n;
}

int ext_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void) typeflag; (void) ftw;
    if (!sb) return 0;
    if (!S_ISREG(sb->st_mode)) return 0;
    const char *base = strrchr(fpath, '/');
    base = base ? base+1 : fpath;
    const char *dot = strrchr(base, '.');
    if (!dot) add_ext(NULL, sb->st_size);
    else add_ext(dot+1, sb->st_size);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
    if (nftw(argv[1], ext_cb, 20, FTW_PHYS) != 0) ERR("nftw");
    struct extstat *p = exts;
    printf("ext\tcount\tsize\n");
    while (p) { printf("%s\t%lld\t%lld\n", p->ext ? p->ext : "(none)", p->count, p->size); p = p->next; }
    /* free list omitted for brevity */
    return 0;
}

/*
Kompilacja
gcc -Wall -Wextra -std=c11 tail_n_lines.c -o tail_n_lines
 */

/* === NAZWA: tail_n_lines.c ===
 * Wypisuje ostatnie N linii pliku. Prosty i działający na tekstowych plikach.
 * Użycie: ./tail_n_lines filename N
 */

#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s file N\n", argv[0]); return 1; }
    const char *path = argv[1];
    int N = atoi(argv[2]);
    if (N <= 0) { fprintf(stderr, "N must be > 0\n"); return 1; }

    FILE *f = fopen(path, "r");
    if (!f) ERR("fopen");

    /* prosta metoda: cykliczny bufor wskaźników na linie */
    char **buf = malloc(sizeof(char*) * N);
    if (!buf) ERR("malloc");
    for (int i=0;i<N;i++) buf[i]=NULL;
    size_t len = 0; ssize_t read;
    char *line = NULL;
    int idx = 0, cnt = 0;
    while ((read = getline(&line, &len, f)) != -1) {
        free(buf[idx]);
        buf[idx] = strdup(line);
        idx = (idx+1) % N;
        if (cnt < N) cnt++;
    }
    /* print */
    int start = (cnt < N) ? 0 : idx;
    for (int i=0;i<cnt;i++) {
        int j = (start + i) % N;
        fputs(buf[j], stdout);
        free(buf[j]);
    }
    free(buf);
    free(line);
    fclose(f);
    return 0;
}

/*
Kompilacja
gcc -Wall -Wextra -std=c11 atomic_replace.c -o atomic_replace
 */

/* === NAZWA: atomic_replace.c ===
 * Pokazuje jak atomowo nadpisać plik — przydatne do wyników - używa mkstemp + rename.
 * Użycie: ./atomic_replace target_file "new content"
 */

#include "util.h"
#include <fcntl.h>
#include <unistd.h>

int atomic_write(const char *path, const char *data, size_t len) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmpXXXXXX", path);
    int fd = mkstemp(tmp);
    if (fd == -1) return -1;
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

/*
Kompilacja
gcc -Wall -Wextra -std=c11 safe_temp_write.c -o safe_temp_write
 */

/* === NAZWA: safe_temp_write.c ===
 * Tworzy bezpieczny plik tymczasowy i zapisuje w nim dane.
 * Użycie: ./safe_temp_write "some text"
 */

#include "util.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s text\n", argv[0]); return 1; }
    char tmpl[] = "/tmp/examtmp.XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) ERR("mkstemp");
    printf("Temp file: %s\n", tmpl);
    const char *s = argv[1];
    if (write(fd, s, strlen(s)) != (ssize_t)strlen(s)) { close(fd); unlink(tmpl); ERR("write"); }
    close(fd);
    return 0;
}


/*
Kompilacja
gcc -Wall -Wextra -std=c11 lock_modify_requirements.c -o lock_modify_requirements
 */

/* === NAZWA: lock_modify_requirements.c ===
 * Dodaje lub usuwa wiersz z pliku requirements zabezpieczając operację flock.
 * Użycie:
 *   ./lock_modify_requirements add env/requirements "pkg 1.0.0"
 *   ./lock_modify_requirements remove env/requirements "pkg"
 */

#include "util.h"
#include <sys/file.h>
#include <fcntl.h>

int append_if_missing_locked(int fd, const char *line) {
    /* załóż lock */
    if (flock(fd, LOCK_EX) != 0) return -1;
    FILE *f = fdopen(dup(fd), "r+");
    if (!f) { flock(fd, LOCK_UN); return -1; }
    char buf[512];
    rewind(f);
    while (fgets(buf, sizeof(buf), f)) {
        if (strncmp(buf, line, strlen(line)) == 0) { fclose(f); flock(fd, LOCK_UN); return 0; }
    }
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
        /* prosta implementacja: lock, write to tmp, rename */
        int fd = open(path, O_RDONLY);
        if (fd == -1) ERR("open req read");
        if (flock(fd, LOCK_EX) != 0) { close(fd); ERR("flock"); }
        FILE *f = fdopen(fd, "r");
        char tmp[4096]; snprintf(tmp, sizeof(tmp), "%s.tmp", path);
        FILE *t = fopen(tmp, "w");
        if (!t) { fclose(f); unlink(tmp); ERR("fopen tmp"); }
        char linebuf[512]; int removed = 0;
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strncmp(linebuf, arg, strlen(arg)) == 0) { removed = 1; continue; }
            fputs(linebuf, t);
        }
        fclose(t);
        fclose(f); /* also releases fd for flock? ensure unlock */
        /* rename */
        if (rename(tmp, path) != 0) { unlink(tmp); ERR("rename"); }
        return removed ? 0 : 1;
    } else {
        fprintf(stderr, "Unknown op\n"); return 1;
    }
}



/*
Kompilacja
gcc -Wall -Wextra -std=c11 read_range.c -o read_range
 */

/* === NAZWA: read_range.c ===
 * Wczytaj zakres [offset, offset+len) z pliku i wypisz na stdout.
 * Użycie: ./read_range file offset len
 */

#include "util.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Usage: %s file offset len\n", argv[0]); return 1; }
    const char *path = argv[1];
    off_t offset = atoll(argv[2]);
    size_t len = (size_t) atoll(argv[3]);

    int fd = open(path, O_RDONLY);
    if (fd == -1) ERR("open");
    if (lseek(fd, offset, SEEK_SET) == -1) { close(fd); ERR("lseek"); }

    char *buf = malloc(len);
    if (!buf) { close(fd); ERR("malloc"); }
    ssize_t r = bulk_read(fd, buf, len);
    if (r < 0) { free(buf); close(fd); ERR("read"); }
    write(STDOUT_FILENO, buf, r);
    free(buf);
    close(fd);
    return 0;
}


/*
Kompilacja
gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 dir_diff.c -o dir_diff
 */

/* === NAZWA: dir_diff.c ===
 * Porównuje dwa drzewa katalogów pod kątem obecności plików i rozmiarów.
 * Wypisuje pliki tylko w A, tylko w B, oraz różnice rozmiarów.
 * Użycie: ./dir_diff pathA pathB
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct entry { char *path; off_t size; struct entry *next; };
static struct entry *A = NULL, *B = NULL;
static const char *rootA, *rootB;

void add_entry(struct entry **list, const char *root, const char *full, off_t size) {
    const char *rel = full + strlen(root);
    if (*rel == '/') ++rel;
    struct entry *e = malloc(sizeof(*e));
    e->path = strdup(rel);
    e->size = size;
    e->next = *list;
    *list = e;
}

int cbA(const char *fpath, const struct stat *sb, int t, struct FTW *ftw) {
    (void)t; (void)ftw;
    if (sb && S_ISREG(sb->st_mode)) add_entry(&A, rootA, fpath, sb->st_size);
    return 0;
}
int cbB(const char *fpath, const struct stat *sb, int t, struct FTW *ftw) {
    (void)t; (void)ftw;
    if (sb && S_ISREG(sb->st_mode)) add_entry(&B, rootB, fpath, sb->st_size);
    return 0;
}

struct entry *find_entry(struct entry *list, const char *path) {
    for (; list; list = list->next) if (strcmp(list->path, path) == 0) return list;
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s A B\n", argv[0]); return 1; }
    rootA = argv[1]; rootB = argv[2];
    if (nftw(rootA, cbA, 20, FTW_PHYS) != 0) ERR("nftw A");
    if (nftw(rootB, cbB, 20, FTW_PHYS) != 0) ERR("nftw B");
    /* tylko w A */
    for (struct entry *p = A; p; p = p->next) {
        struct entry *q = find_entry(B, p->path);
        if (!q) printf("ONLY IN A: %s (size %lld)\n", p->path, (long long)p->size);
        else if (p->size != q->size) printf("DIFF SIZE: %s A=%lld B=%lld\n", p->path, (long long)p->size, (long long)q->size);
    }
    for (struct entry *p = B; p; p = p->next) {
        if (!find_entry(A, p->path)) printf("ONLY IN B: %s (size %lld)\n", p->path, (long long)p->size);
    }
    return 0;
}



/* === robust_main_template.c ===
 * Safe program template: argument handling, logging, exit codes.
 * gcc -Wall -Wextra -std=c11 robust_main_template.c -o robust_main_template
 */
#include "util.h"
#include <getopt.h>

void usage(const char *p) {
    fprintf(stderr, "Usage: %s -i input -o output [-v]\n", p);
    fprintf(stderr, "  -i --input   path to input\n  fprintf(stderr, "  -o --output  path to output\n  ");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *input = NULL, *output = NULL;
    int verbose = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:o:v")) != -1) {
        switch (opt) {
        case 'i': input = optarg; break;
        case 'o': output = optarg; break;
        case 'v': verbose = 1; break;
        default: usage(argv[0]);
        }
    }
    if (!input || !output) usage(argv[0]);

    if (verbose) fprintf(stderr, "DEBUG: input=%s output=%s\n", input, output);

    /* ---- main logic skeleton ---- */
    /* jeśli błąd: ERR("somecall"); */

    return EXIT_SUCCESS;
}



/* === signal_cleanup.c ===
 * Example: register handlers to cleanup on Ctrl+C or termination.
 * gcc -Wall -Wextra -std=c11 signal_cleanup.c -o signal_cleanup
 */
#include "util.h"
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t stop_flag = 0;
static char tmpfile[1024] = "/tmp/exam_temp_file.xxx";

void cleanup(void) {
    /* usuń pliki tymczasowe, przywróć stan */
    unlink(tmpfile);
    fprintf(stderr, "cleanup done\n");
}

void handler(int sig) {
    (void)sig;
    stop_flag = 1;
    /* nie wolno robić zbyt wiele w handlerze */
}

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) ERR("sigaction");
    if (sigaction(SIGTERM, &sa, NULL) == -1) ERR("sigaction");

    /* rejestruj cleanup aby wykonał się przy normalnym exit */
    if (atexit(cleanup) != 0) ERR("atexit");

    /* przykład pętli */
    while (!stop_flag) {
        sleep(1);
    }

    fprintf(stderr, "Received stop, exiting safely\n");
    return EXIT_SUCCESS;
}


/* logger.h */
#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
enum { LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3 };
void log_set_level(int level);
void log_msg(int level, const char *fmt, ...);
#endif


/* logger.h */
#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
enum { LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3 };
void log_set_level(int level);
void log_msg(int level, const char *fmt, ...);
#endif


/* logger.c */
#include "logger.h"
#include <stdarg.h>
#include <time.h>

static int log_level = LOG_INFO;

void log_set_level(int level) { log_level = level; }

void log_msg(int level, const char *fmt, ...) {
    if (level > log_level) return;
    const char *labels[] = {"ERROR","WARN","INFO","DEBUG"};
    time_t t = time(NULL);
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(stderr, "[%s] %s: ", ts, labels[level]);
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}


/*
generate_tree.ssh - skrypt do stres testów
uruchmienie:
chmod +x generate_tree.sh && ./generate_tree.sh test_big 4 6 20
 */
#!/bin/sh
# === generate_tree.sh ===
# usage: ./generate_tree.sh base_dir depth breadth files_per_dir
base=${1:-test_big}
depth=${2:-3}
breadth=${3:-4}
files=${4:-10}
rm -rf "$base"
mkdir -p "$base"
# recursive create
create_level() {
    local dir=$1
    local d=$2
    if [ "$d" -le 0 ]; then
      for i in $(seq 1 $files); do
          dd if=/dev/urandom bs=1 count=100 status=none of="$dir/file_$i.bin"
        done
        return
      fi
      for i in $(seq 1 $breadth); do
          nd="$dir/dir_$i"
          mkdir -p "$nd"
          # some files
          for j in $(seq 1 $files); do
              echo "file ${i}_${j}" > "$nd/f_${j}.txt"
            done
            # maybe a symlink
            ln -s ../f_1.txt "$nd/link_to_parent_f1" 2>/dev/null || true
            create_level "$nd" $((d-1))
          done
        }
create_level "$base" "$depth"
echo "Generated tree in $base"

/*
wkrywanie wyciekó pamięci run_valgrid.sh
 */
#!/bin/sh
# usage: ./run_valgrind.sh ./your_program args...
prog="$1"; shift
if ! command -v valgrind >/dev/null; then echo "Install valgrind to use this script"; exit 1; fi
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=2 "$prog" "$@"

/*
bezpieczne łączenie ścierzek
 */
/* safe_path_join.c - implement for reuse */
#include "util.h"
#include <limits.h>

int safe_path_join(const char *a, const char *b, char *out, size_t out_sz) {
#ifdef PATH_MAX
    size_t maxp = PATH_MAX;
#else
    size_t maxp = 4096;
#endif
    if (snprintf(out, out_sz, "%s/%s", a, b) >= (int)out_sz) return -1;
    if (strlen(out) > maxp) return -1;
    return 0;
}

/*
8) argparse_long.c — przykład getopt_long z ładnym helpem

Przydatne gdy zadanie wymaga wielu opcji.
 */
/* === argparse_long.c ===
 * Example for getopt_long usage.
 * gcc -Wall -Wextra -std=c11 argparse_long.c -o argparse_long
 */
#include "util.h"
#include <getopt.h>

int main(int argc, char **argv) {
    static struct option longopts[] = {
        {"path", required_argument, NULL, 'p'},
        {"depth", required_argument, NULL, 'd'},
        {"output", required_argument, NULL, 'o'},
        {"help", no_argument, NULL, 'h'},
        {0,0,0,0}
    };
    int c; char *path=NULL; int depth=-1; char *out=NULL;
    while ((c = getopt_long(argc, argv, "p:d:o:h", longopts, NULL)) != -1) {
        switch (c) {
        case 'p': path = optarg; break;
        case 'd': depth = atoi(optarg); break;
        case 'o': out = optarg; break;
        case 'h': fprintf(stderr, "Usage: ...\n"); return 0;
        default: break;
        }
    }
    fprintf(stderr, "Got: path=%s depth=%d out=%s\n", path?path:"(null)", depth, out?out:"(null)");
    return 0;
}

/*

 */
