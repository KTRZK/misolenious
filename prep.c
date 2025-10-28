/*
-------------------------
ZADANIE A — ZLICZANIE W KATALOGU (BEZ REKURSYWNIE)
-------------------------
Plik/funkcja: count_dir_once(...)  (zdefiniowana wcześniej)
Program: exam_count_simple.c (wrapper wywołujący count_dir_once)

Kompilacja:
    gcc -Wall -Wextra -std=c11 exam_count_simple.c -o exam_count_simple

Uruchomienie / przykłady:
    ./exam_count_simple            # skanuje bieżący katalog
    ./exam_count_simple /tmp      # skanuje /tmp

Co sprawdzić:
 - w katalogu bez podkatalogów wynik: Dirs=0
 - utwórz symlink i zwykły plik, sprawdź czy liczby rosną poprawnie:
    ln -s existing_file linktest
    touch filetest
    ./exam_count_simple

Uwaga:
 - funkcja używa lstat, więc linki liczone są jako linki.

-------------------------
ZADANIE B — ZLICZANIE REKURENCYJNE (nftw)
-------------------------
Plik/funkcja: nftw_count_wrapper(...) oraz exam_nftw_count.c

Kompilacja (wymaga deklaracji _XOPEN_SOURCE):
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 exam_nftw_count.c -o exam_nftw_count

Uruchomienie / przykłady:
    ./exam_nftw_count /path/to/dir1 /path/to/dir2

Co sprawdzić:
 - sygnalizuje błędy dostępu (permission denied) dla niedostępnych katalogów.
 - FTW_PHYS użyty, więc linki liczone jako linki (nie jako docelowe pliki).

-------------------------
ZADANIE C — KOPIOWANIE PLIKÓW (niskopoziomowo open/read/write)
-------------------------
Plik: copy_lowlevel.c (używa bulk_read/bulk_write)
Albo funkcja: copy_file_lowlevel(...) (dostępna jako snippet)

Kompilacja:
    gcc -Wall -Wextra -std=c11 copy_lowlevel.c -o copy_lowlevel
    # albo (jeśli używasz wrappera z main):
    gcc -Wall -Wextra -std=c11 copy_file_lowlevel.c -o copy_file_lowlevel -DCOPY_FILE_LOWLEVEL_MAIN

Uruchomienie / przykłady:
    ./copy_lowlevel src.bin dest.bin
    cmp src.bin dest.bin || echo "Różne!"

Uwagi:
 - program obsługuje EINTR (TEMP_FAILURE_RETRY).
 - bulk_write zapewnia zapisanie całego bufora (pętla write).

-------------------------
ZADANIE D — TWORZENIE PLIKU O ZADANYM ROZMIARZE I 10% LOSOWYCH ZNAKÓW
-------------------------
Plik: make_file.c (implementuje make_file(name,size,perms,10))

Kompilacja:
    gcc -Wall -Wextra -std=c11 make_file.c -o make_file

Uruchomienie / przykłady:
    ./make_file -n sample.bin -p 0644 -s 2000
    ls -l sample.bin        # rozmiar powinien być 2000
    hexdump -C sample.bin | head

Uwagi:
 - używa ftruncate() by zagwarantować dokładny rozmiar,
 - pwrite() do losowego zapisu liter bez zmiany offsetu,
 - umask ustawione tymczasowo by wymusić uprawnienia.

-------------------------
ZADANIE E — LISTING Z FILTRAMI (głębia, rozszerzenia)
-------------------------
Plik: scanner_filters.c (implementuje -p -d -e -o)

Kompilacja:
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 scanner_filters.c -o scanner_filters

Uruchomienie / przykłady:
    ./scanner_filters -p ./dir1 -d 2 -e txt -o out.txt
    cat out.txt

Uwagi:
 - -p można podać wielokrotnie,
 - -d to maksymalna głębokość (0 = tylko katalog podany),
 - -e to rozszerzenie bez kropki (np. txt),
 - program korzysta z nftw (FTW_PHYS).

-------------------------
ZADANIE F — PROSTY MENADŻER VENV (sop-venv)
-------------------------
Plik: sop-venv.c (zawiera create_env, install_pkg, remove_pkg)

Kompilacja:
    gcc -Wall -Wextra -std=c11 sop-venv.c -o sop-venv

Podstawowe wywołania:
    # tworzenie środowiska
    ./sop-venv -c -v new_environment
    ls new_environment     # powinien zawierać 'requirements'

    # instalacja pakietu
    ./sop-venv -v new_environment -i numpy==1.0.0
    cat new_environment/requirements
    ls -l new_environment

    # usuwanie pakietu
    ./sop-venv -v new_environment -r numpy

Błędy:
 - jeśli środowisko nie istnieje, program wypisze błąd i zwróci kod != 0.
 - instalacja nie dubluje pakietów; usuwanie nieistniejącego pakietu powoduje błąd.

-------------------------
ZADANIE G — ATOMOWY ZAPIS PLIKU WYNIKOWEGO
-------------------------
Funkcja: atomic_write_file(path, buf, len, mode)

Użycie (przykład):
    const char *out = "wynik.txt";
    atomic_write_file(out, buffer, buflen, 0644);

Opis:
 - zapisuje do pliku tymczasowego tmpXXXXXX, fchmod, close, rename(tmp,path)
 - zabezpiecza przed utratą danych gdy zapis przerwany.

-------------------------
ZADANIE H — UŻYCIE writev/readv
-------------------------
Plik: writev_example.c

Kompilacja:
    gcc -Wall -Wextra -std=c11 writev_example.c -o writev_example

Uruchomienie / przykłady:
    ./writev_example
    cat writev_out.txt

Opis:
 - pokazuje, jak zapisać kilka buforów jednym syscall writev.

-------------------------
ZADANIE I — POPRAWNE UŻYCIE readdir I errno
-------------------------
Snippet: readdir_errno_example (zawarty w helperach / count_dir_once)


-------------------------
DODATKOWE PRZYDATNE NARZĘDZIA I JEGO KOMPILACJA
-------------------------
Pliki pomocnicze (dostarczone wcześniej):
 - util.h   (ERR, TEMP_FAILURE_RETRY, safe_malloc, safe_strdup)
 - copy_stdio.c    (kopiowanie z fopen/fread/fwrite)
 - listing_program.c (prosty listing -p ... -o ...)
 - walk_recursive_manual.c (rekursja bez nftw)
 - writev_example.c, readv_example.c
 - test_scripts.sh (skrypt testowy — uruchom po skompilowaniu programów)

Sygnały i EINTR:
 - dla wywołań read/write używaj TEMP_FAILURE_RETRY lub pętli sprawdzającej errno==EINTR.

Przykładowy Makefile (jeśli chcesz kompilować wszystko naraz):
    make (użyj Makefile dostarczonego wcześniej: komenda 'make' skompiluje większość programów)

-------------------------
SZYBKIE TESTY (sekwencja do uruchomienia ręcznie)
-------------------------
1) Kompilacja:
    gcc -Wall -Wextra -std=c11 exam_count_simple.c -o exam_count_simple
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 exam_nftw_count.c -o exam_nftw_count
    gcc -Wall -Wextra -std=c11 copy_lowlevel.c -o copy_lowlevel
    gcc -Wall -Wextra -std=c11 make_file.c -o make_file
    gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 scanner_filters.c -o scanner_filters
    gcc -Wall -Wextra -std=c11 sop-venv.c -o sop-venv
    gcc -Wall -Wextra -std=c11 writev_example.c -o writev_example

2) Przygotowanie folderu testowego:
    mkdir -p testdir/subdir
    echo "hello" > testdir/file1.txt
    ln -s file1.txt testdir/link1
    truncate -s 1024 testdir/big.bin

3) Uruchomienia:
    ./exam_count_simple testdir
    ./exam_nftw_count testdir
    ./make_file -n sample.bin -p 0644 -s 2000
    ./copy_lowlevel sample.bin sample.copy
    cmp sample.bin sample.copy || echo "Nieidentyczne"
    ./scanner_filters -p testdir -d 2 -e txt -o listing.txt
    ./sop-venv -c -v myenv
    ./sop-venv -v myenv -i pkg==0.1
    ./sop-venv -v myenv -r pkg
*/






/* === NAZWA: ERR_MACRO ===
 * Opis: Makro do wypisywania błędu i natychmiastowego exit.
 * Użycie: ERR("opis");
 * Wymagania: <errno.h>, <stdio.h>, <stdlib.h>
 */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))



/* === NAZWA: TEMP_FAILURE_RETRY_FALLBACK ===
 * Opis: Makro retryujące wywołanie po EINTR (jeśli nie ma w systemie).
 * Użycie: ssize_t c = TEMP_FAILURE_RETRY(read(fd, buf, n));
 * Wymagania: <errno.h>
 */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
(__extension__ ({ long int __result; \
do __result = (long int)(expression); \
while (__result == -1L && errno == EINTR); \
__result; }))
#endif




/* === PROGRAM: exam_count_simple.c ===
 * Używa funkcji count_dir_once(...) z "Pliki i ścieżki".
 * Kompilacja: gcc -Wall -Wextra -std=c11 exam_count_simple.c -o exam_count_simple
 */

#include "util.h"
#include <stdio.h>
#include <sys/stat.h>

/* Prototyp funkcji zdefiniowanej wcześniej */
int count_dir_once(const char *dirpath, int *files, int *dirs, int *links, int *other);

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : ".";
    int files, dirs, links, other;
    if (count_dir_once(path, &files, &dirs, &links, &other) != 0) ERR("count_dir_once");
    printf("%s: Files=%d Dirs=%d Links=%d Other=%d\n", path, files, dirs, links, other);
    return 0;
}
/* === NAZWA: copy_preserve_metadata.c ===
 * Kopiuje plik (zawartość) i stara się zachować właściciela, uprawnienia i czasy.
 * - używa open/read/write, fstat, fchmod, fchown, utimensat
 * Kompilacja:
 *   gcc -Wall -Wextra -std=c11 copy_preserve_metadata.c -o copy_preserve_metadata
 * Uwaga: ustawianie właściciela (fchown) zadziała tylko jako root.
 */

#include "util.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>

#define BUF 8192

int copy_preserve_metadata(const char *src, const char *dst) {
    int fd_src = open(src, O_RDONLY);
    if (fd_src == -1) return -1;
    struct stat st;
    if (fstat(fd_src, &st) != 0) { close(fd_src); return -1; }

    /* utwórz plik docelowy z tymczasowymi uprawnieniami, potem ustawimy właściwe */
    int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (fd_dst == -1) { close(fd_src); return -1; }

    char buf[BUF];
    ssize_t r;
    while ((r = TEMP_FAILURE_RETRY(read(fd_src, buf, BUF))) > 0) {
        if (TEMP_FAILURE_RETRY(write(fd_dst, buf, r)) != r) { close(fd_src); close(fd_dst); return -1; }
    }
    if (r < 0) { close(fd_src); close(fd_dst); return -1; }

    /* zachowaj tryb (fchmod), właściciela (fchown), czasy (utimensat) */
    if (fchmod(fd_dst, st.st_mode & 07777) != 0) { /* nie krytyczne */ ; }
    if (fchown(fd_dst, st.st_uid, st.st_gid) != 0 && errno != EPERM) { /* jeśli nie root, EPERM możliwe */ ; }

    struct timespec times[2];
    times[0] = st.st_atim;
    times[1] = st.st_mtim;
    if (futimens(fd_dst, times) != 0) { /* może nie działać na niektórych FS */ ; }

    close(fd_src); close(fd_dst);
    return 0;
}


/* === NAZWA: list_dir_sorted.c ===
 * Listuje katalog posortowany alfabetycznie używając scandir (wygodny do egzaminu).
 * Kompilacja:
 *   gcc -Wall -Wextra -std=c11 list_dir_sorted.c -o list_dir_sorted
 * Użycie:
 *   ./list_dir_sorted /path
 */

#include "util.h"
#include <dirent.h>

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : ".";
    struct dirent **namelist;
    int n = scandir(path, &namelist, NULL, alphasort);
    if (n < 0) ERR("scandir");
    for (int i = 0; i < n; ++i) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);
    return 0;
}


/* === NAZWA: recursive_delete.c ===
 * Usuwa rekurencyjnie drzewo katalogu (bez nftw - ale z nftw jest prościej).
 * Kompilacja:
 *   gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 recursive_delete.c -o recursive_delete
 * Uwaga: używa nftw z FTW_DEPTH (usuwa pliki przed folderami)
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

static int _rm_err = 0;
int _rm_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void) sb; (void) typeflag; (void) ftw;
    if (remove(fpath) != 0) { _rm_err = errno; return -1; }
    return 0;
}

int recursive_delete(const char *path) {
    _rm_err = 0;
    if (nftw(path, _rm_cb, 20, FTW_DEPTH | FTW_PHYS) != 0) {
        errno = _rm_err ? _rm_err : errno;
        return -1;
    }
    return 0;
}

/* main test:
int main(int argc, char **argv) {
  if (argc != 2) { fprintf(stderr, "Usage: %s path\n", argv[0]); return 1; }
  if (recursive_delete(argv[1]) != 0) ERR("recursive_delete");
  return 0;
}
*/


/* === NAZWA: top_n_largest.c ===
 * Znajduje N największych plików w podanym drzewie (nftw).
 * Kompilacja:
 *   gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 top_n_largest.c -o top_n_largest
 * Użycie:
 *   ./top_n_largest /path 10
 *
 * Implementacja trzyma prostą listę max N (sortowana przez wstawianie) - wystarczy dla N małego (np. 10).
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>

struct item { off_t size; char *path; };
static struct item *top = NULL;
static int top_n = 0;
static int top_cap = 0;

void top_init(int n) {
    top = calloc(n, sizeof(*top));
    if (!top) ERR("calloc");
    top_cap = n; top_n = 0;
}

void top_free(void) {
    for (int i=0;i<top_n;++i) free(top[i].path);
    free(top); top = NULL; top_cap = top_n = 0;
}

void top_try_add(const char *path, off_t size) {
    /* jeśli jeszcze nie pełne - dodaj i posortuj malejąco */
    int i;
    if (top_n < top_cap) {
        top[top_n].size = size;
        top[top_n].path = strdup(path);
        ++top_n;
    } else if (size <= top[top_n-1].size) {
        return;
    } else {
        free(top[top_n-1].path);
        top[top_n-1].size = size;
        top[top_n-1].path = strdup(path);
    }
    /* sort descending */
    for (i = top_n-1; i > 0; --i) {
        if (top[i].size > top[i-1].size) {
            struct item tmp = top[i]; top[i] = top[i-1]; top[i-1] = tmp;
        } else break;
    }
}

int top_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
    (void) typeflag; (void) ftw;
    if (sb && S_ISREG(sb->st_mode)) top_try_add(fpath, sb->st_size);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s path N\n", argv[0]); return 1; }
    int N = atoi(argv[2]);
    top_init(N);
    if (nftw(argv[1], top_cb, 20, FTW_PHYS) != 0) ERR("nftw");
    for (int i = 0; i < top_n; ++i) {
        printf("%3d: %12lld %s\n", i+1, (long long)top[i].size, top[i].path);
    }
    top_free();
    return 0;
}


/* === NAZWA: safe_getcwd.c ===
 * Wrapper zwracający dynamicznie alokowany current working directory (użyj free()).
 * Kompilacja: (możesz to włączyć w inny plik)
 * Użycie: char *cwd = safe_getcwd(); if (cwd) { puts(cwd); free(cwd); }
 */

#include "util.h"
#include <limits.h>

char *safe_getcwd(void) {
#ifdef PATH_MAX
    size_t sz = PATH_MAX;
#else
    size_t sz = 1024;
#endif
    char *buf = NULL;
    for (;;) {
        buf = realloc(buf, sz);
        if (!buf) ERR("realloc");
        if (getcwd(buf, sz) != NULL) return buf;
        if (errno != ERANGE) { free(buf); return NULL; }
        sz *= 2;
    }
}


/* === NAZWA: print_mode.c ===
 * Funkcja drukująca tryb pliku (jak ls -l: rwxr-xr-x) wraz z typem (-, d, l, c, b, p, s)
 * Użycie: print_mode(st.st_mode, out_stream);
 */

#include "util.h"
#include <sys/stat.h>

void print_mode(mode_t m, FILE *out) {
    char t = '?';
    if (S_ISREG(m)) t='-';
    else if (S_ISDIR(m)) t='d';
    else if (S_ISLNK(m)) t='l';
    else if (S_ISCHR(m)) t='c';
    else if (S_ISBLK(m)) t='b';
    else if (S_ISFIFO(m)) t='p';
    else if (S_ISSOCK(m)) t='s';

    fprintf(out, "%c", t);
    char perms[10] = "rwxrwxrwx";
    for (int i=0;i<9;++i) {
        fprintf(out, "%c", (m & (1 << (8-i))) ? perms[i] : '-');
    }
    /* setuid/setgid/sticky */
    if (m & S_ISUID) fprintf(out, " (SUID)");
    if (m & S_ISGID) fprintf(out, " (SGID)");
    if (m & S_ISVTX) fprintf(out, " (STICKY)");
}


/* === NAZWA: secure_tempfile_mkstemp.c ===
 * Wrapper tworzący bezpieczny plik tymczasowy i zwracający fd oraz scieżkę (malloc).
 * Użycie:
 *   char *template = strdup("/tmp/mytmp.XXXXXX");
 *   int fd = secure_mkstemp(&template);
 *   // template zawiera rzeczywistą nazwę (free po zamknięciu)
 */

#include "util.h"

int secure_mkstemp(char **out_template) {
    if (!out_template || !*out_template) { errno = EINVAL; return -1; }
    int fd = mkstemp(*out_template);
    if (fd == -1) return -1;
    return fd;
}

/* === NAZWA: readlink_realpath_helpers.c ===
 * Małe helpery: readlink (bez bufor overflow) i realpath wrapper.
 * Użycie: char *r = readlink_alloc("link"); free(r);
 */

#include "util.h"

char *readlink_alloc(const char *path) {
    ssize_t bufsize = 128;
    for (;;) {
        char *buf = malloc(bufsize);
        if (!buf) ERR("malloc");
        ssize_t r = readlink(path, buf, bufsize);
        if (r < 0) { free(buf); return NULL; }
        if (r < bufsize) { buf[r] = '\0'; return buf; }
        free(buf);
        bufsize *= 2;
    }
}

char *realpath_dup(const char *path) {
    char *res = realpath(path, NULL); /* glibc pozwala na NULL to dynamic alloc */
    if (!res) return NULL;
    return res;
}


/* === NAZWA: file_lock_flock.c ===
 * Prosty przykład użycia flock (advisory lock) i fcntl (porównanie).
 * Użycie: flock(fd, LOCK_EX) / flock(fd, LOCK_UN)
 */

#include "util.h"
#include <sys/file.h>

int try_flock_exclusive(int fd) {
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) return -1; /* errno EWOULDBLOCK jeśli inny */
    return 0;
}





/* === PROGRAM: exam_nftw_count.c ===
 * Wrapper używający nftw_count_wrapper(...) (definicja wcześniej).
 * Kompilacja: gcc -D_XOPEN_SOURCE=700 -Wall -Wextra -std=c11 exam_nftw_count.c -o exam_nftw_count
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <stdio.h>

/* Prototyp zdefiniowany wcześniej */
int nftw_count_wrapper(const char *path, int *files, int *dirs, int *links, int *other);

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s path [path...]\n", argv[0]); return 1; }
    for (int i = 1; i < argc; ++i) {
        int files, dirs, links, other;
        if (nftw_count_wrapper(argv[i], &files, &dirs, &links, &other) != 0) {
            fprintf(stderr, "Error scanning %s\n", argv[i]);
            continue;
        }
        printf("%s:\n files: %d\n dirs: %d\n links: %d\n other: %d\n", argv[i], files, dirs, links, other);
    }
    return 0;
}


/* === FUNCTION: copy_file_lowlevel (nowa, korzysta z bulk_read/bulk_write) ===
 * Prosty wrapper kopiujący plik src->dst używając read/write (bulk_*).
 * Kompilacja: gcc -Wall -Wextra -std=c11 copy_file_lowlevel.c -o copy_file_lowlevel
 */

#include "util.h"
#include <fcntl.h>
#include <unistd.h>

#define FILE_BUF_LEN 4096
ssize_t bulk_read(int fd, void *buf, size_t count);   /* już zdefiniowane wcześniej */
ssize_t bulk_write(int fd, const void *buf, size_t count);

int copy_file_lowlevel(const char *src, const char *dst) {
    int fd1 = open(src, O_RDONLY);
    if (fd1 == -1) return -1;
    int fd2 = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 == -1) { close(fd1); return -1; }

    char buf[FILE_BUF_LEN];
    ssize_t r;
    while ((r = read(fd1, buf, FILE_BUF_LEN)) > 0) {
        if (bulk_write(fd2, buf, (size_t)r) == -1) { close(fd1); close(fd2); return -1; }
    }
    if (r < 0) { close(fd1); close(fd2); return -1; }

    close(fd1); close(fd2);
    return 0;
}

/* Krótki main (opcjonalny) */
#ifdef COPY_FILE_LOWLEVEL_MAIN
int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s src dst\n", argv[0]); return 1; }
    if (copy_file_lowlevel(argv[1], argv[2]) != 0) ERR("copy_file_lowlevel");
    return 0;
}
#endif


/* === PROGRAM: exam_make_file_exact.c ===
 * Wywołuje make_file(name, size, perms, 10)
 * Kompilacja: gcc -Wall -Wextra -std=c11 exam_make_file_exact.c -o exam_make_file_exact
 */

/* zakładam, że make_file(...) jest dostępna (wcześniej zdefiniowane w make_file.c) */
#include "util.h"
#include <stdio.h>
#include <sys/stat.h>

void make_file(const char *name, off_t size, mode_t perms, int percent); /* zdefiniowane wcześniej */

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "Usage: %s name octal_perms size\n", argv[0]); return 1; }
    const char *name = argv[1];
    mode_t perms = (mode_t) strtol(argv[2], NULL, 8);
    off_t size = (off_t) strtoll(argv[3], NULL, 10);
    make_file(name, size, perms, 10);
    return 0;
}




/* === NAZWA: safe_malloc ===
 * Opis: wrapper malloc sprawdzający NULL i wywołujący ERR.
 * Użycie: char *buf = safe_malloc(1024);
 */
static inline void *safe_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) ERR("malloc");
    return p;
}



/* === NAZWA: safe_strdup ===
 * Opis: wrapper strdup z kontrolą błędów (zwraca zalokowany string).
 * Użycie: char *s = safe_strdup("abc");
 */
static inline char *safe_strdup(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s);
    if (!r) ERR("strdup");
    return r;
}



/* === NAZWA: path_join ===
 * Opis: Skleja path i name w jedną ścieżkę (bez alokacji wielokrotnej).
 * Użycie: path_join(buf, sizeof(buf), "/tmp", "file.txt");
 */
static inline void path_join(char *out, size_t outsz, const char *path, const char *name) {
    if (!out || !path || !name) { if (out && outsz) out[0]='\0'; return; }
    size_t lp = strlen(path);
    if (lp == 0) snprintf(out, outsz, "%s", name);
    else {
        if (path[lp-1] == '/') snprintf(out, outsz, "%s%s", path, name);
        else snprintf(out, outsz, "%s/%s", path, name);
    }
}



/* === NAZWA: file_exists ===
 * Opis: Sprawdza, czy plik/ścieżka istnieje (stat).
 * Zwraca: 1 jeśli istnieje, 0 jeśli nie, -1 w razie błędu stat innego niż ENOENT.
 */
static inline int file_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return 1;
    if (errno == ENOENT) return 0;
    return -1;
}




/* === NAZWA: is_dir ===
 * Opis: Czy ścieżka wskazuje katalog?
 * Zwraca: 1 jeśli katalog, 0 jeśli nie lub nie istnieje, ERR() w razie błędu stat.
 */
static inline int is_dir(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        if (errno == ENOENT) return 0;
        ERR("lstat");
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}



/* === NAZWA: get_file_size ===
 * Opis: Zwraca rozmiar pliku (w bajtach) lub -1 przy błędzie.
 */
static inline off_t get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_size;
}



/* === NAZWA: count_dir_once ===
 * Opis: Zlicza pliki, katalogi, linki, inne w podanym katalogu (bez rekurencji).
 * Prototyp: int count_dir_once(const char *dirpath, int *files, int *dirs, int *links, int *other);
 * Zwraca: 0 sukces, -1 błąd (errno ustawiony).
 * Uwaga: używa lstat.
 */
int count_dir_once(const char *dirpath, int *files, int *dirs, int *links, int *other) {
    DIR *dirp = opendir(dirpath);
    if (!dirp) return -1;
    struct dirent *dp;
    struct stat st;
    *files = *dirs = *links = *other = 0;
    char full[4096];
    while (1) {
        errno = 0;
        dp = readdir(dirp);
        if (dp == NULL) break;
        if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0) continue;
        snprintf(full, sizeof(full), "%s/%s", dirpath, dp->d_name);
        if (lstat(full, &st) != 0) { closedir(dirp); return -1; }
        if (S_ISDIR(st.st_mode)) (*dirs)++;
        else if (S_ISREG(st.st_mode)) (*files)++;
        else if (S_ISLNK(st.st_mode)) (*links)++;
        else (*other)++;
    }
    if (errno != 0) { closedir(dirp); return -1; }
    if (closedir(dirp) != 0) return -1;
    return 0;
}



/* === NAZWA: list_dir_with_sizes ===
 * Opis: Wypisuje (do FILE *out) nazwy i rozmiary obiektów w katalogu (nie rekurencyjnie).
 * Użycie: list_dir_with_sizes("/tmp", stdout);
 */
int list_dir_with_sizes(const char *dirpath, FILE *out) {
    DIR *d = opendir(dirpath);
    if (!d) return -1;
    struct dirent *dp;
    struct stat st;
    char full[4096];
    while ((dp = readdir(d)) != NULL) {
        if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0) continue;
        snprintf(full, sizeof(full), "%s/%s", dirpath, dp->d_name);
        if (lstat(full, &st) == -1) {
            closedir(d); return -1;
        }
        fprintf(out, "%s %lld\n", dp->d_name, (long long) st.st_size);
    }
    closedir(d);
    return 0;
}





/* === NAZWA: nftw_count_wrapper ===
 * Opis: Liczy pliki/dir/links/other rekurencyjnie przy użyciu nftw.
 * Prototyp: int nftw_count_wrapper(const char *path, int *files, int *dirs, int *links, int *other);
 * Uwaga: definiuj _XOPEN_SOURCE przed includami w pliku źródłowym.
 */
#include <ftw.h>
typedef struct { int files, dirs, links, other; } cnts_t;

static cnts_t _nftw_cnts;

int _nftw_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)fpath; (void)sb; (void)ftwbuf;
    switch (typeflag) {
    case FTW_DNR:
    case FTW_D: _nftw_cnts.dirs++; break;
    case FTW_F: _nftw_cnts.files++; break;
    case FTW_SL: _nftw_cnts.links++; break;
    default: _nftw_cnts.other++;
    }
    return 0;
}

int nftw_count_wrapper(const char *path, int *files, int *dirs, int *links, int *other) {
    _nftw_cnts.files = _nftw_cnts.dirs = _nftw_cnts.links = _nftw_cnts.other = 0;
    if (nftw(path, _nftw_cb, 20, FTW_PHYS) != 0) return -1;
    *files = _nftw_cnts.files; *dirs = _nftw_cnts.dirs; *links = _nftw_cnts.links; *other = _nftw_cnts.other;
    return 0;
}



/* === NAZWA: bulk_read ===
 * Opis: Czyta dokładnie count bajtów (jeśli możliwe), wielokrotne wywołania read.
 * Zwraca liczbę przeczytanych bajtów lub -1 na błąd.
 */
ssize_t bulk_read(int fd, void *buf, size_t count) {
    size_t left = count;
    char *ptr = buf;
    while (left > 0) {
        ssize_t r = TEMP_FAILURE_RETRY(read(fd, ptr, left));
        if (r < 0) return -1;
        if (r == 0) return count - left; /* EOF */
        left -= r; ptr += r;
    }
    return count;
}

/* === NAZWA: bulk_write ===
 * Opis: Zapisuje wszystko (pętla write).
 */
ssize_t bulk_write(int fd, const void *buf, size_t count) {
    size_t left = count; const char *ptr = buf;
    while (left > 0) {
        ssize_t w = TEMP_FAILURE_RETRY(write(fd, ptr, left));
        if (w < 0) return -1;
        left -= w; ptr += w;
    }
    return count;
}



/* === NAZWA: atomic_write_file ===
 * Opis: Atomowo zapisuje zawartość (zapisywanie do pliku tymczasowego i rename).
 * Użycie: atomic_write_file("out.txt", data, len, 0644);
 */
int atomic_write_file(const char *path, const void *buf, size_t len, mode_t mode) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmpXXXXXX", path);
    int fd = mkstemp(tmp);
    if (fd == -1) return -1;
    if (fchmod(fd, mode) == -1) { close(fd); unlink(tmp); return -1; }
    if (bulk_write(fd, (void*)buf, len) == -1) { close(fd); unlink(tmp); return -1; }
    if (close(fd) == -1) { unlink(tmp); return -1; }
    if (rename(tmp, path) == -1) { unlink(tmp); return -1; }
    return 0;
}



/* === NAZWA: append_line_if_missing ===
 * Opis: Dodaje linię "line" do pliku path jeśli jej tam nie ma (porównanie nazwa pola).
 * Użycie: append_line_if_missing("env/requirements", "numpy 1.0.0");
 */
int append_line_if_missing(const char *path, const char *line_prefix) {
    FILE *f = fopen(path, "r+");
    if (!f) { if (errno == ENOENT) f = fopen(path, "w+"); else return -1; }
    char buf[512]; rewind(f);
    while (fgets(buf, sizeof(buf), f)) {
        if (strncmp(buf, line_prefix, strlen(line_prefix)) == 0) { fclose(f); return 0; }
    }
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s\n", line_prefix);
    fclose(f);
    return 1; /* 1 = appended, 0 = existed */
}

/* === NAZWA: remove_line_prefix ===
 * Opis: Usuwa linię zaczynającą się od prefix (tworzy tmp + rename).
 * Użycie: remove_line_prefix("env/requirements", "numpy");
 */
int remove_line_prefix(const char *path, const char *prefix) {
    char tmp[4096]; snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    FILE *t = fopen(tmp, "w");
    if (!t) { fclose(f); return -1; }
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, prefix, strlen(prefix)) == 0) { found = 1; continue; }
        fputs(line, t);
    }
    fclose(f); fclose(t);
    if (!found) { unlink(tmp); return 1; } /* 1 = nothing removed */
    if (rename(tmp, path) != 0) { unlink(tmp); return -1; }
    return 0; /* 0 = success */
}



/* === NAZWA: file_lock_fd_fcntl ===
 * Opis: Blokuje lub odblokowuje region pliku za pomocą fcntl (advisory).
 * Użycie: file_lock_fd_fcntl(fd, F_WRLCK) ; file_lock_fd_fcntl(fd, F_UNLCK);
 * Zwraca 0 przy sukcesie.
 */
int file_lock_fd_fcntl(int fd, short type) {
    struct flock fl = {0};
    fl.l_type = type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0; /* cały plik */
    if (fcntl(fd, F_SETLKW, &fl) == -1) return -1;
    return 0;
}


/* === NAZWA: set_permissions_recursive ===
 * Opis: Rekurencyjnie ustawia uprawnienia dla drzewa (uwaga: używać ostrożnie).
 * Użycie: set_permissions_recursive("/path", 0755);
 */
int set_permissions_recursive(const char *path, mode_t mode) {
    /* prosta wersja: nftw i chmod */
    int err = 0;
    int cb(const char *fpath, const struct stat *sb, int t, struct FTW *ftw) {
        (void)sb; (void)ftw;
        if (chmod(fpath, mode) != 0) { err = errno; return -1; }
        return 0;
    }
    if (nftw(path, cb, 20, FTW_PHYS) != 0) {
        if (err) errno = err;
        return -1;
    }
    return 0;
}



/* === NAZWA: create_symlink_if_not_exists ===
 * Opis: Tworzy symlink target->linkpath jeśli link nie istnieje.
 * Użycie: create_symlink_if_not_exists("target", "link");
 */
int create_symlink_if_not_exists(const char *target, const char *linkpath) {
    if (lstat(linkpath, &(struct stat){0}) == 0) return 0; /* istnieje */
    if (symlink(target, linkpath) != 0) return -1;
    return 0;
}


/* === NAZWA: touch_file_update_mtime ===
 * Opis: Tworzy plik jeśli nie istnieje i ustawia czas modyfikacji (now).
 * Użycie: touch_file_update_mtime("file");
 */
int touch_file_update_mtime(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT, 0666);
    if (fd == -1) return -1;
    if (close(fd) == -1) return -1;
    struct timespec times[2];
    times[0].tv_nsec = UTIME_NOW; times[1].tv_nsec = UTIME_NOW;
    if (utimensat(AT_FDCWD, path, times, 0) == -1) return -1;
    return 0;
}



/* === NAZWA: create_random_file ===
 * Opis: Tworzy plik size bajtów z zapisanymi losowymi bajtami (użycie /dev/urandom).
 * Użycie: create_random_file("rand.bin", 1024);
 */
int create_random_file(const char *path, size_t size) {
    int in = open("/dev/urandom", O_RDONLY);
    if (in == -1) return -1;
    int out = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) { close(in); return -1; }
    char buf[4096];
    size_t left = size;
    while (left) {
        size_t to = left < sizeof(buf) ? left : sizeof(buf);
        if (bulk_read(in, buf, to) != (ssize_t)to) { close(in); close(out); return -1; }
        if (bulk_write(out, buf, to) != (ssize_t)to) { close(in); close(out); return -1; }
        left -= to;
    }
    close(in); close(out);
    return 0;
}






/* util.h
 * Wspólne makra i pomocnicze funkcje do wszystkich programów na egzamin.
 * Kompilacja: nie kompiluj samo util.h — dołącz do .c przez #include "util.h"
 */

#ifndef UTIL_H
#define UTIL_H

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ERR macro - wypisz błąd i zakończ */
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

/* TEMP_FAILURE_RETRY jeśli nie ma w systemie (często w unistd.h) */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
(__extension__                         \
({ long int __result;                 \
do __result = (long int)(expression); \
while (__result == -1L && errno == EINTR); \
__result; }))
#endif

/* Safe strdup wrapper */
static inline char *xstrdup(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s);
    if (!r) ERR("strdup");
    return r;
}

#endif /* UTIL_H */


/* scan_dir.c
 * Liczy pliki, katalogi, linki i inne w aktualnym katalogu (bez podkatalogów).
 * Kompilacja: gcc -Wall -Wextra -std=c11 scan_dir.c -o scan_dir
 * Uruchomienie: ./scan_dir
 */

#include "util.h"
#include <dirent.h>
#include <sys/stat.h>

void scan_dir(void)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int dirs = 0, files = 0, links = 0, other = 0;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");

    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            if (lstat(dp->d_name, &filestat))
                ERR("lstat");
            if (S_ISDIR(filestat.st_mode))
                dirs++;
            else if (S_ISREG(filestat.st_mode))
                files++;
            else if (S_ISLNK(filestat.st_mode))
                links++;
            else
                other++;
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", files, dirs, links, other);
}

int main(void)
{
    scan_dir();
    return EXIT_SUCCESS;
}




/* scan_dirs_args.c
 * Dla każdego katalogu podanego jako parametr wypisuje wynik scan_dir() (bez rekurencji).
 * Kompilacja: gcc -Wall -Wextra -std=c11 scan_dirs_args.c -o scan_dirs_args
 * Uruchomienie: ./scan_dirs_args dir1 dir2
 */

#include "util.h"
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH 4096

void scan_dir(void)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int dirs = 0, files = 0, links = 0, other = 0;

    if ((dirp = opendir(".")) == NULL)
        ERR("opendir");

    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            if (lstat(dp->d_name, &filestat))
                ERR("lstat");
            if (S_ISDIR(filestat.st_mode))
                dirs++;
            else if (S_ISREG(filestat.st_mode))
                files++;
            else if (S_ISLNK(filestat.st_mode))
                links++;
            else
                other++;
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", files, dirs, links, other);
}

int main(int argc, char **argv)
{
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        ERR("getcwd");

    if (argc < 2) {
        printf("Usage: %s dir1 [dir2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        if (chdir(argv[i]) != 0) {
            fprintf(stderr, "Cannot chdir to %s: %s\n", argv[i], strerror(errno));
            continue; /* można kontynuować dla kolejnych katalogów */
        }
        printf("%s:\n", argv[i]);
        scan_dir();
        if (chdir(cwd) != 0)
            ERR("chdir restore");
    }

    return EXIT_SUCCESS;
}


/* nftw_count.c
 * Liczy rekursywnie (poddrzewa) pliki/dir/link/other przy użyciu nftw.
 * Kompilacja: gcc -Wall -Wextra -std=c11 -D_XOPEN_SOURCE=700 nftw_count.c -o nftw_count
 * Uruchomienie: ./nftw_count dir1 dir2
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <sys/stat.h>

#define MAXFD 20

static int dirs = 0, files = 0, links = 0, other = 0;

int walk_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void)ftwbuf;
    switch (typeflag) {
    case FTW_DNR:
    case FTW_D:
        dirs++; break;
    case FTW_F:
        files++; break;
    case FTW_SL:
        links++; break;
    default:
        other++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s dir1 [dir2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        dirs = files = links = other = 0;
        if (nftw(argv[i], walk_callback, MAXFD, FTW_PHYS) == 0) {
            printf("%s:\nfiles:%d\ndirs:%d\nlinks:%d\nother:%d\n", argv[i], files, dirs, links, other);
        } else {
            fprintf(stderr, "%s: access denied or error\n", argv[i]);
        }
    }
    return EXIT_SUCCESS;
}



/* walk_recursive_manual.c
 * Rekurencyjne przechodzenie drzewa katalogów bez nftw - przydatne na systemach bez nftw.
 * Kompilacja: gcc -Wall -Wextra -std=c11 walk_recursive_manual.c -o walk_recursive_manual
 */

#include "util.h"
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

static int files = 0, dirs = 0, links = 0, other = 0;

void traverse(const char *path)
{
    DIR *dirp = opendir(path);
    if (!dirp) { fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno)); return; }

    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", path, dp->d_name);

        struct stat st;
        if (lstat(full, &st) != 0) { fprintf(stderr, "lstat %s failed: %s\n", full, strerror(errno)); continue; }

        if (S_ISDIR(st.st_mode)) {
            dirs++;
            traverse(full);
        } else if (S_ISREG(st.st_mode)) files++;
        else if (S_ISLNK(st.st_mode)) links++;
        else other++;
    }
    closedir(dirp);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s dir1 [dir2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        files = dirs = links = other = 0;
        traverse(argv[i]);
        printf("%s:\nfiles:%d\ndirs:%d\nlinks:%d\nother:%d\n", argv[i], files, dirs, links, other);
    }
    return EXIT_SUCCESS;
}


/* make_file.c
 * Tworzy plik o nazwie -n NAME, rozmiarze -s SIZE, uprawnieniach -p OCTAL.
 * Zawartość: ~10% losowych znaków [A-Z], reszta bajtów to '\0' (prawdziwe zera).
 * Gwarantowany rozmiar przez ftruncate.
 * Kompilacja: gcc -Wall -Wextra -std=c11 make_file.c -o make_file
 * Przykład: ./make_file -n test.bin -s 1000 -p 0644
 */

#include "util.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

void usage(const char *pname) {
    fprintf(stderr, "USAGE: %s -n NAME -p OCTAL -s SIZE\n", pname);
    exit(EXIT_FAILURE);
}

void make_file(const char *name, off_t size, mode_t perms, int percent)
{
    /* ustawiamy umask aby nadpisać bitmaskę tworzenia pliku */
    mode_t oldmask = umask(~perms & 0777);
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    umask(oldmask);
    if (fd == -1) ERR("open");

    /* ustalamy wymagany rozmiar */
    if (ftruncate(fd, size) == -1) ERR("ftruncate");

    /* losowo zapisujemy percent% liter [A-Z] w losowych pozycjach */
    srand((unsigned) time(NULL) ^ (unsigned)getpid());
    for (off_t i = 0; i < (size * percent) / 100; ++i) {
        char ch = 'A' + (rand() % ('Z' - 'A' + 1));
        off_t pos = rand() % (size ? size : 1);
        if (pwrite(fd, &ch, 1, pos) != 1) ERR("pwrite");
    }

    if (close(fd) == -1) ERR("close");
}

int main(int argc, char **argv)
{
    int opt;
    const char *name = NULL;
    mode_t perms = (mode_t)-1;
    off_t size = -1;

    while ((opt = getopt(argc, argv, "n:p:s:")) != -1) {
        switch (opt) {
            case 'n': name = optarg; break;
            case 'p': perms = (mode_t) strtol(optarg, NULL, 8); break;
            case 's': size = (off_t) strtoll(optarg, NULL, 10); break;
            default: usage(argv[0]);
        }
    }
    if (!name || perms == (mode_t)-1 || size < 0) usage(argv[0]);

    /* usuwamy istniejący plik (by ustawić uprawnienia od nowa) */
    if (unlink(name) && errno != ENOENT) ERR("unlink");

    make_file(name, size, perms, 10);
    return EXIT_SUCCESS;
}

/* copy_lowlevel.c
 * Kopiuje plik z path_1 do path_2 używając open/read/write.
 * Kompilacja: gcc -Wall -Wextra -std=c11 copy_lowlevel.c -o copy_lowlevel
 * Uruchomienie: ./copy_lowlevel src dest
 */

#include "util.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_BUF_LEN 4096

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c, len = 0;
    while (count > 0) {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0) return c;
        if (c == 0) return len; /* EOF */
        buf += c; len += c; count -= c;
    }
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c, len = 0;
    while (count > 0) {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0) return c;
        buf += c; len += c; count -= c;
    }
    return len;
}

void usage(const char *pname) {
    fprintf(stderr, "USAGE: %s path_1 path_2\n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 3) usage(argv[0]);
    const char *src = argv[1], *dst = argv[2];

    int fd1 = open(src, O_RDONLY);
    if (fd1 == -1) ERR("open src");

    int fd2 = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 == -1) ERR("open dst");

    char buf[FILE_BUF_LEN];
    for (;;) {
        ssize_t r = TEMP_FAILURE_RETRY(read(fd1, buf, FILE_BUF_LEN));
        if (r < 0) ERR("read");
        if (r == 0) break;
        if (bulk_write(fd2, buf, r) == -1) ERR("bulk_write");
    }

    if (close(fd1) == -1) ERR("close src");
    if (close(fd2) == -1) ERR("close dst");
    return EXIT_SUCCESS;
}



/* copy_stdio.c
 * Kopiowanie z fopen/fread/fwrite (buferowane).
 * Kompilacja: gcc -Wall -Wextra -std=c11 copy_stdio.c -o copy_stdio
 */

#include "util.h"
#include <stdio.h>

#define BUFSIZE 8192

void usage(const char *pname) { fprintf(stderr, "USAGE: %s src dst\n", pname); exit(EXIT_FAILURE); }

int main(int argc, char **argv)
{
    if (argc != 3) usage(argv[0]);
    FILE *in = fopen(argv[1], "rb");
    if (!in) ERR("fopen in");
    FILE *out = fopen(argv[2], "wb");
    if (!out) { fclose(in); ERR("fopen out"); }

    char buf[BUFSIZE];
    size_t r;
    while ((r = fread(buf, 1, BUFSIZE, in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) ERR("fwrite");
    }
    if (ferror(in)) ERR("fread");

    if (fclose(in)) ERR("fclose in");
    if (fclose(out)) ERR("fclose out");
    return EXIT_SUCCESS;
}



/* writev_example.c
 * Przykład użycia writev do zapisania kilku buforów w jednym write.
 * Kompilacja: gcc -Wall -Wextra -std=c11 writev_example.c -o writev_example
 */

#include "util.h"
#include <sys/uio.h>
#include <fcntl.h>

int main(void)
{
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
    return EXIT_SUCCESS;
}



/* listing_program.c
 * -p <path>  (można powtarzać)
 * -o <file>  (opcja)
 * Wypisuje listing plików i ich rozmiary (w katalogu, nie rekurencyjnie).
 * Kompilacja: gcc -Wall -Wextra -std=c11 listing_program.c -o listing_program
 */

#include "util.h"
#include <dirent.h>
#include <sys/stat.h>
#include <getopt.h>

void list_one_dir(const char *path, FILE *out)
{
    DIR *d = opendir(path);
    if (!d) { fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno)); return; }
    fprintf(out, "PATH:\n%s\nLISTA PLIKÓW:\n", path);
    struct dirent *dp;
    struct stat st;
    char fullname[4096];
    while ((dp = readdir(d)) != NULL) {
        if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0) continue;
        snprintf(fullname, sizeof(fullname), "%s/%s", path, dp->d_name);
        if (lstat(fullname, &st) == -1) {
            fprintf(stderr, "lstat %s failed: %s\n", fullname, strerror(errno));
            continue;
        }
        if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode) || S_ISDIR(st.st_mode)) {
            fprintf(out, "%s %lld\n", dp->d_name, (long long) st.st_size);
        } else {
            fprintf(out, "%s %lld (other)\n", dp->d_name, (long long) st.st_size);
        }
    }
    closedir(d);
}

int main(int argc, char **argv)
{
    char *outfile = NULL;
    int opt;
    /* proste zbieranie -p może być wielokrotne */
    char *paths[256]; int paths_cnt = 0;

    while ((opt = getopt(argc, argv, "p:o:")) != -1) {
        switch (opt) {
            case 'p': paths[paths_cnt++] = optarg; break;
            case 'o': outfile = optarg; break;
            default:
                fprintf(stderr, "Usage: %s -p PATH [-p PATH ...] [-o OUTPUT]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (paths_cnt == 0) {
        fprintf(stderr, "At least one -p PATH required\n");
        exit(EXIT_FAILURE);
    }

    FILE *out = stdout;
    if (outfile) {
        out = fopen(outfile, "w");
        if (!out) ERR("fopen outfile");
    }

    for (int i = 0; i < paths_cnt; ++i) list_one_dir(paths[i], out);

    if (outfile) fclose(out);
    return EXIT_SUCCESS;
}


/*
./scanner_filters -p ./dir1 -p ./dir2 -d 2 -e txt -o out.txt
 */


/* scanner_filters.c
 * Filtered recursive scanner: -p path (multi), -d depth, -e ext, -o out
 * Kompilacja: gcc -Wall -Wextra -std=c11 scanner_filters.c -o scanner_filters
 */

#define _XOPEN_SOURCE 700
#include "util.h"
#include <ftw.h>
#include <sys/stat.h>
#include <getopt.h>

#define MAXFD 20
static FILE *out = NULL;
static int max_depth = -1;
static const char *ext = NULL;
static const char *current_root = NULL;

int walk_fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    if (typeflag == FTW_F) {
        if (max_depth >= 0 && ftwbuf->level - 0 > max_depth) return 0;
        const char *name = fpath + ftwbuf->base;
        if (!ext || (strrchr(name, '.') && strcmp(strrchr(name, '.') + 1, ext) == 0)) {
            /* print path relative to current_root */
            fprintf(out, "path: %s\n%s %lld\n", current_root, name, (long long) (sb ? sb->st_size : 0));
        }
    }
    return 0;
}

void usage(const char *pname) {
    fprintf(stderr, "Usage: %s -p path [-p path ...] [-d depth] [-e ext] [-o out]\n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    char *outs = NULL;
    int opt;
    char *paths[256]; int pc = 0;
    while ((opt = getopt(argc, argv, "p:d:e:o:")) != -1) {
        switch (opt) {
        case 'p': paths[pc++] = optarg; break;
        case 'd': max_depth = atoi(optarg); break;
        case 'e': ext = optarg; break;
        case 'o': outs = optarg; break;
        default: usage(argv[0]);
        }
    }
    if (pc == 0) usage(argv[0]);

    out = stdout;
    if (outs) { out = fopen(outs, "w"); if (!out) ERR("fopen out"); }

    for (int i = 0; i < pc; ++i) {
        current_root = paths[i];
        fprintf(out, "path: %s\n", paths[i]);
        if (nftw(paths[i], walk_fn, MAXFD, FTW_PHYS) != 0)
            fprintf(stderr, "Error scanning %s\n", paths[i]);
    }

    if (outs) fclose(out);
    return EXIT_SUCCESS;
}



/* sop-venv.c
 * Prosty manager środowisk:
 * -c -v <env>       : create environment
 * -v <env> -i name==ver : install
 * -v <env> -r name      : remove
 * Opcje można mieszać; błąd na jednym env przerywa.
 * Kompilacja: gcc -Wall -Wextra -std=c11 sop-venv.c -o sop-venv
 */

#include "util.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>

void usage(const char *pname) {
    fprintf(stderr, "Usage examples:\n");
    fprintf(stderr, "  %s -c -v envname\n", pname);
    fprintf(stderr, "  %s -v envname -i pkg==ver\n", pname);
    fprintf(stderr, "  %s -v envname -r pkg\n", pname);
    exit(EXIT_FAILURE);
}

int env_exists(const char *env) {
    struct stat st;
    char path[4096];
    snprintf(path, sizeof(path), "%s", env);
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int create_env(const char *env) {
    if (env_exists(env)) {
        fprintf(stderr, "Environment %s already exists\n", env);
        return -1;
    }
    if (mkdir(env, 0755) != 0) { perror("mkdir"); return -1; }
    char reqpath[4096];
    snprintf(reqpath, sizeof(reqpath), "%s/requirements", env);
    FILE *f = fopen(reqpath, "w");
    if (!f) { perror("fopen"); return -1; }
    fclose(f);
    return 0;
}

int install_pkg(const char *env, const char *pkg_eq_ver) {
    if (!env_exists(env)) { fprintf(stderr, "the environment does not exist: %s\n", env); return -1; }
    /* parse pkg==ver */
    char pkg[256], ver[256];
    char *eq = strstr(pkg_eq_ver, "==");
    if (!eq) { fprintf(stderr, "Bad package format, expected name==version\n"); return -1; }
    snprintf(pkg, eq - pkg_eq_ver + 1, "%s", pkg_eq_ver);
    strcpy(ver, eq + 2);

    /* check no duplicate in requirements */
    char reqpath[4096];
    snprintf(reqpath, sizeof(reqpath), "%s/requirements", env);
    FILE *f = fopen(reqpath, "r+");
    if (!f) { perror("fopen"); return -1; }
    char line[512]; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char name[256], version[256];
        if (sscanf(line, "%255s %255s", name, version) == 2) {
            if (strcmp(name, pkg) == 0) { found = 1; break; }
        }
    }
    if (found) { fclose(f); fprintf(stderr, "Package %s is already installed\n", pkg); return -1; }
    /* append */
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s %s\n", pkg, ver);
    fclose(f);
    /* create package file with random content and mode 0444 */
    char pkgpath[4096];
    snprintf(pkgpath, sizeof(pkgpath), "%s/%s", env, pkg);
    FILE *pf = fopen(pkgpath, "w");
    if (!pf) { perror("fopen pkg"); return -1; }
    /* fill with some random bytes */
    for (int i = 0; i < 20; ++i) fputc('a' + (rand()%26), pf);
    fclose(pf);
    if (chmod(pkgpath, 0444) != 0) { perror("chmod pkg"); return -1; }
    return 0;
}

int remove_pkg(const char *env, const char *pkg) {
    if (!env_exists(env)) { fprintf(stderr, "the environment does not exist: %s\n", env); return -1; }
    char reqpath[4096], tmp[4096], pkgpath[4096];
    snprintf(reqpath, sizeof(reqpath), "%s/requirements", env);
    snprintf(tmp, sizeof(tmp), "%s/requirements.tmp", env);

    FILE *f = fopen(reqpath, "r");
    if (!f) { perror("fopen"); return -1; }
    FILE *t = fopen(tmp, "w");
    if (!t) { fclose(f); perror("fopen tmp"); return -1; }

    char name[256], version[256];
    int found = 0;
    while (fscanf(f, "%255s %255s", name, version) == 2) {
        if (strcmp(name, pkg) == 0) { found = 1; continue; }
        fprintf(t, "%s %s\n", name, version);
    }
    fclose(f);
    fclose(t);
    if (!found) { remove(tmp); fprintf(stderr, "Package %s not installed\n", pkg); return -1; }
    /* replace */
    if (rename(tmp, reqpath) != 0) { perror("rename"); remove(tmp); return -1; }
    snprintf(pkgpath, sizeof(pkgpath), "%s/%s", env, pkg);
    if (unlink(pkgpath) != 0 && errno != ENOENT) { perror("unlink pkg"); return -1; }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) usage(argv[0]);
    int opt;
    int do_create = 0;
    char *envs[64]; int envc = 0;
    char *install_spec = NULL;
    char *remove_pkg_name = NULL;

    srand(time(NULL));

    while ((opt = getopt(argc, argv, "cv:i:r:")) != -1) {
        switch (opt) {
            case 'c': do_create = 1; break;
            case 'v': envs[envc++] = optarg; break;
            case 'i': install_spec = optarg; break;
            case 'r': remove_pkg_name = optarg; break;
            default: usage(argv[0]);
        }
    }
    if (envc == 0) usage(argv[0]);

    if (do_create) {
        if (envc != 1) { fprintf(stderr, "Creation expects exactly one -v <env>\n"); usage(argv[0]); }
        return create_env(envs[0]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    for (int i = 0; i < envc; ++i) {
        if (install_spec) {
            if (install_pkg(envs[i], install_spec) != 0) return EXIT_FAILURE;
        }
        if (remove_pkg_name) {
            if (remove_pkg(envs[i], remove_pkg_name) != 0) return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}




/*
# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: scan_dir scan_dirs_args nftw_count walk_recursive_manual make_file copy_lowlevel copy_stdio writev_example listing_program scanner_filters sop-venv

scan_dir: scan_dir.c util.h
    $(CC) $(CFLAGS) scan_dir.c -o scan_dir

scan_dirs_args: scan_dirs_args.c util.h
    $(CC) $(CFLAGS) scan_dirs_args.c -o scan_dirs_args

nftw_count: nftw_count.c util.h
    $(CC) $(CFLAGS) -D_XOPEN_SOURCE=700 nftw_count.c -o nftw_count

walk_recursive_manual: walk_recursive_manual.c util.h
    $(CC) $(CFLAGS) walk_recursive_manual.c -o walk_recursive_manual

make_file: make_file.c util.h
    $(CC) $(CFLAGS) make_file.c -o make_file

copy_lowlevel: copy_lowlevel.c util.h
    $(CC) $(CFLAGS) copy_lowlevel.c -o copy_lowlevel

copy_stdio: copy_stdio.c util.h
    $(CC) $(CFLAGS) copy_stdio.c -o copy_stdio

writev_example: writev_example.c util.h
    $(CC) $(CFLAGS) writev_example.c -o writev_example

listing_program: listing_program.c util.h
    $(CC) $(CFLAGS) listing_program.c -o listing_program

scanner_filters: scanner_filters.c util.h
    $(CC) $(CFLAGS) -D_XOPEN_SOURCE=700 scanner_filters.c -o scanner_filters

sop-venv: sop-venv.c util.h
    $(CC) $(CFLAGS) sop-venv.c -o sop-venv

clean:
    rm -f scan_dir scan_dirs_args nftw_count walk_recursive_manual make_file copy_lowlevel copy_stdio writev_example listing_program scanner_filters sop-venv *.o
 */


#!/bin/sh
set -e

# tworzenie testowego drzewa
mkdir -p testdir/subdir testdir2
echo "hello" > testdir/file1.txt
ln -s file1.txt testdir/link1
truncate -s 1000 testdir/big.bin

# test scan_dir
./scan_dir

# test scan_dirs_args
./scan_dirs_args testdir testdir2

# test nftw_count
./nftw_count testdir testdir2

# test make_file
./make_file -n sample.bin -p 0644 -s 2000
ls -l sample.bin

# test copy_lowlevel
./copy_lowlevel sample.bin sample.copy
cmp sample.bin sample.copy || echo "files differ"

# test listing_program
./listing_program -p testdir -o listing.txt
cat listing.txt

# test sop-venv
./sop-venv -c -v myenv
./sop-venv -v myenv -i numpy==1.0.0
cat myenv/requirements
ls -l myenv
./sop-venv -v myenv -r numpy
cat myenv/requirements

