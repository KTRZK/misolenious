//compilator: gcc -std=gnu11 -Wall -Wextra -o kopalnia kopalnia.c
./kopalnia 2 4 8


#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

/* --- pomocnicze funkcje niskopoziomowe (z Twojego startowego kodu) --- */

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void ms_sleep(unsigned int milli)
{
    time_t sec = (int)(milli / 1000);
    milli = milli - (sec * 1000);
    struct timespec ts = {0};
    ts.tv_sec = sec;
    ts.tv_nsec = milli * 1000000L;
    if (TEMP_FAILURE_RETRY(nanosleep(&ts, &ts)))
        ERR("nanosleep");
}

/* --- koniec kopii startowego kodu --- */

/* globalna zmienna w potomku: zlicza SIGUSR1
   volatile sig_atomic_t, bo zmieniana w handlerze sygnału */
static volatile sig_atomic_t miner_sigcnt = 0;

/* handler prosty — tylko inkrementuje licznik.
   Nie wolno tu robić malloc/write itp. */
void sigusr1_handler(int signo)
{
    (void)signo;
    miner_sigcnt++;
}

/* losuje s w KB: [10..100] */
int random_kb_size(void)
{
    return (rand() % 91) + 10; // 10..100
}

/* otwiera plik miner_<pid>.bin, zwraca deskryptor (lub ERR) */
int open_miner_file(pid_t pid)
{
    char name[64];
    snprintf(name, sizeof(name), "miner_%d.bin", (int)pid);
    int fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) ERR("open miner file");
    return fd;
}

/* logika potomka (górnika) */
void miner_process(int id)
{
    pid_t pid = getpid();

    /* różne ziarno losowania dla każdego potomka */
    srand((unsigned)time(NULL) ^ (unsigned)pid);

    int s_kb = random_kb_size();              // rozmiar bloku w KB
    size_t block_size = (size_t)s_kb * 1024;  // w bajtach

    /* pokaż co wylosowaliśmy (pierwszy etap zadania) */
    printf("miner pid=%d id=%d s=%dKB\n", (int)pid, id, s_kb);
    fflush(stdout);

    /* utwórz plik od razu (będzie pusty do czasu zapisu) */
    {
        int fd = open_miner_file(pid);
        if (close(fd) < 0) ERR("close");
    }

    /* ustaw handler na SIGUSR1 */
    sethandler(sigusr1_handler, SIGUSR1);

    /* czekamy ~1s, w międzyczasie reagujemy na sygnały i pokazujemy aktualny licznik
       (handler tylko inkrementuje miner_sigcnt; tu wypisujemy bezpiecznie) */
    int last_cnt = 0;
    const unsigned int total_ms = 1000;
    unsigned int elapsed = 0;
    while (elapsed < total_ms) {
        if (miner_sigcnt != last_cnt) {
            last_cnt = miner_sigcnt;
            printf("miner pid=%d cnt=%d s=%dKB\n", (int)pid, last_cnt, s_kb);
            fflush(stdout);
        }
        ms_sleep(10);
        elapsed += 10;
    }

    /* po upływie czasu życia: zapisz cnt bloków do pliku */
    int cnt = miner_sigcnt;
    if (cnt <= 0) {
        /* nic nie zapisujemy, kończymy */
        printf("miner pid=%d: otrzymano 0 sygnałów — koniec (brak zapisu)\n", (int)pid);
        fflush(stdout);
        _exit(EXIT_SUCCESS);
    }

    /* przygotuj bufor jednego bloku wypełnionego odpowiednią cyfrą */
    char digit = (char)('0' + (id % 10));
    char *buf = malloc(block_size);
    if (!buf) ERR("malloc");
    memset(buf, digit, block_size);

    /* otwórz plik do zapisu i dopisz cnt bloków używając bulk_write (chronimy przed partial writes) */
    char filename[64];
    snprintf(filename, sizeof(filename), "miner_%d.bin", (int)pid);
    int fd = open(filename, O_WRONLY | O_APPEND);
    if (fd < 0) {
        /* jeśli append nie działa, próbuj zwykłego otwarcia do zapisu (truncate już zrobiliśmy wcześniej) */
        fd = open(filename, O_WRONLY);
        if (fd < 0) ERR("open for write");
    }

    for (int i = 0; i < cnt; ++i) {
        ssize_t written = bulk_write(fd, buf, block_size);
        if (written != (ssize_t)block_size) {
            fprintf(stderr, "miner pid=%d: błąd zapisu bloku %d (wrote=%zd expected=%zu)\n", (int)pid, i, written, block_size);
            ERR("bulk_write");
        }
    }

    if (close(fd) < 0) ERR("close after write");
    free(buf);

    printf("miner pid=%d: zapisano %d bloków po %dKB (plik: %s)\n", (int)pid, cnt, s_kb, filename);
    fflush(stdout);

    _exit(EXIT_SUCCESS);
}

/* rodzic: wysyła SIGUSR1 co 10 ms przez 1s do wszystkich potomków */
void foreman_send_signals(pid_t *children, int count)
{
    const unsigned int interval_ms = 10;
    const unsigned int duration_ms = 1000;
    unsigned int elapsed = 0;

    while (elapsed < duration_ms) {
        for (int i = 0; i < count; ++i) {
            if (children[i] > 0) {
                if (kill(children[i], SIGUSR1) < 0) {
                    // moze juz nie istnieje — zignoruj
                    if (errno != ESRCH) ERR("kill");
                }
            }
        }
        ms_sleep(interval_ms);
        elapsed += interval_ms;
    }
}

/* prosty parser: przyjmujemy 1..4 argumenty: cyfry 0..9 (id górników) */
void usage(const char *prog)
{
    fprintf(stderr, "Użycie: %s id1 [id2 ... id4]\n", prog);
    fprintf(stderr, "\tidX to cyfra 0..9 (identyfikator górnika)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 5) usage(argv[0]);

    int n = argc - 1;
    int ids[4];
    for (int i = 0; i < n; ++i) {
        char *end;
        long v = strtol(argv[i+1], &end, 10);
        if (*end != '\0' || v < 0 || v > 9) {
            fprintf(stderr, "Błędny id: %s\n", argv[i+1]);
            usage(argv[0]);
        }
        ids[i] = (int)v;
    }

    pid_t children[4];
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");
        if (pid == 0) {
            /* w procesie potomnym od razu wykonujemy logikę górnika */
            miner_process(ids[i]);
            /* nigdy nie wracamy */
        } else {
            children[i] = pid;
        }
    }

    /* rodzic wysyła sygnały przez ~1s co 10ms */
    foreman_send_signals(children, n);

    /* czekamy na potomków */
    int status;
    for (int i = 0; i < n; ++i) {
        pid_t r = waitpid(children[i], &status, 0);
        if (r < 0) ERR("waitpid");
        if (WIFEXITED(status)) {
            printf("foreman: child %d exited with %d\n", (int)r, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("foreman: child %d killed by signal %d\n", (int)r, WTERMSIG(status));
        } else {
            printf("foreman: child %d ended unexpectedly\n", (int)r);
        }
    }

    printf("foreman: wszyscy potomkowie zakończeni. Koniec.\n");
    return EXIT_SUCCESS;
}
