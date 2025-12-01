/* gcc -std=gnu11 -Wall -Wextra -o forest_watch forest_watch.c
./forest_watch 10 3 5 20
*/



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
#include <sys/types.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

/* --- startowy kod pomocniczy (bulk read/write, sethandler, ms_sleep) --- */

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
            return len;  /* EOF */
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

/* ustawienie handlera (prostsze) */
void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

/* milczenie na milisekundy */
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

/* --- koniec pomocniczych funkcji --- */

/* --- GLOBAL W RODZICU --- */
static volatile sig_atomic_t simulation_ended = 0; /* ustawione przez handler SIGALRM */

/* handler SIGALRM w rodzicu: wysyła SIGTERM do grupy i oznajmia koniec */
void parent_sigalrm_handler(int signo)
{
    (void)signo;
    simulation_ended = 1;
    /* wysyłamy SIGTERM do grupy procesów (łącznie z potomkami) */
    if (killpg(getpgrp(), SIGTERM) < 0) {
        if (errno != ESRCH) /* ignore if none */
            perror("killpg");
    }
    /* nie możemy używać printf tutaj (nie async-signal-safe) */
}

/* --- HANDLERY W POTOMNYCH (watchers) --- */
/* tylko lekkie operacje: ustawiamy flagę, pamiętamy PID nadawcy */
static volatile sig_atomic_t watcher_got_alert = 0;
static volatile sig_atomic_t watcher_last_sender = 0;
static void watcher_sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig; (void)ucontext;
    if (info && info->si_pid > 0) {
        watcher_last_sender = (sig_atomic_t)info->si_pid;
    } else {
        watcher_last_sender = 0;
    }
    watcher_got_alert = 1; /* oznacz, że mamy do obsłużenia alert */
}

/* SIGTERM w watcherze: ustaw flagę zakończenia */
static volatile sig_atomic_t watcher_terminate = 0;
static void watcher_sigterm_handler(int signo)
{
    (void)signo;
    watcher_terminate = 1;
}

/* pomocnicza: losowa liczba w [a,b] */
static int rand_range(int a, int b)
{
    return a + rand() % (b - a + 1);
}

/* zapis wyniku do pliku watcher_<pid>.txt (treść: liczba alertów w ASCII) */
static void write_result_file(pid_t pid, long alerts)
{
    char name[64];
    snprintf(name, sizeof(name), "watcher_%d.txt", (int)pid);
    int fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) return; /* nie fatalne dla procesu - tylko próbujemy zapisać */
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%ld\n", alerts);
    if (len > 0) {
        ssize_t w = bulk_write(fd, buf, (size_t)len);
        (void)w;
    }
    close(fd);
}

/* watcher process logic */
void watcher_process(int id, int k, int p)
{
    pid_t pid = getpid();

    /* ustaw grupę procesów na grupę rodzica (dziedziczone) - nic do robienia, zachowują getpgrp() */
    /* seed random */
    srand((unsigned)time(NULL) ^ (unsigned)pid);

    /* losujemy s (10..100 KB) - użyteczne jeśli chcemy pisać plik większy */
    int s_kb = rand_range(10, 100);
    size_t block_size = (size_t)s_kb * 1024; /* not strictly needed, ale zgodne z zadaniem */

    /* informacja startowa */
    printf("Watcher[%d] starts, id: %d, initial_active: %d\n", (int)pid, id, (id==0 ? 1 : 0));
    fflush(stdout);

    /* ustaw handler SIGUSR1 z SA_SIGINFO */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = watcher_sigusr1_handler;
    act.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(SIGUSR1, &act, NULL)) ERR("sigaction SIGUSR1");

    /* ustaw SIGTERM handler */
    sethandler(watcher_sigterm_handler, SIGTERM);

    /* we will track state */
    int active = (id == 0) ? 1 : 0; /* first watcher initially active (id==0) */
    time_t activation_time = active ? time(NULL) : 0;
    long sent_alerts = 0;

    /* we will record whether watcher was picked up (exit before simulation end) */
    int picked_up = 0;

    /* main loop: obsługa alertów i emisja alertów, aż do picked_up lub SIGTERM */
    while (!watcher_terminate && !picked_up) {
        /* obsłuż otrzymany alert (jeśli jakiś) */
        if (watcher_got_alert) {
            int sender = (int)watcher_last_sender;
            watcher_got_alert = 0;
            watcher_last_sender = 0;
            /* wypisz informację o otrzymanym alercie (poza handlerem) */
            printf("Watcher[%d]: %d sent alert!\n", (int)pid, sender);
            fflush(stdout);

            /* jeśli nieaktywny -> z probabilty p% stanie się aktywny */
            if (!active) {
                int roll = rand_range(1, 100);
                if (roll <= p) {
                    active = 1;
                    activation_time = time(NULL);
                    printf("Watcher[%d] activated!\n", (int)pid);
                    fflush(stdout);
                }
            }
        }

        if (active) {
            /* check if picked up (k seconds since activation) */
            time_t now = time(NULL);
            if ((int)(now - activation_time) >= k) {
                /* picked up by rescue team */
                printf("Watcher[%d] picked up after %ld alerts\n", (int)pid, sent_alerts);
                fflush(stdout);
                /* zapisz wynik do pliku z pełną liczbą alertów */
                write_result_file(pid, sent_alerts);
                picked_up = 1;
                break;
            }

            /* emituj alert co losowy czas 50..200 ms */
            int delay = rand_range(50, 200);
            ms_sleep((unsigned)delay);

            /* emituj - do grupy procesów (killpg) */
            if (killpg(getpgrp(), SIGUSR1) < 0) {
                if (errno != ESRCH) perror("killpg");
            }
            sent_alerts++;
            printf("Watcher[%d] emitted alert (count: %ld)\n", (int)pid, sent_alerts);
            fflush(stdout);
            /* continue loop: will catch own alert too, but handler stores sender PID (own PID) - we print it but
               we can ignore self-activation because active == 1 already */
        } else {
            /* nieaktywny - nie robi nic aktywnego, małe opóźnienie */
            ms_sleep(50);
        }
    } /* koniec głównej pętli */

    /* jeśli zakończyliśmy z powodu SIGTERM albo picked_up, kończymy wypisując exit message i zwracając liczbę alertów */
    if (watcher_terminate) {
        /* zakończenie symulacji */
        /* zapisz plik z wynikiem (pełna liczba) */
        write_result_file(pid, sent_alerts);
        int exit_code = (sent_alerts > 255) ? 255 : (int)sent_alerts;
        printf("Watcher[%d] exits with %d\n", (int)pid, exit_code);
        fflush(stdout);
        _exit(exit_code);
    }

    if (picked_up) {
        /* exit with min(255, sent_alerts) but we already wrote full count to file */
        int exit_code = (sent_alerts > 255) ? 255 : (int)sent_alerts;
        printf("Watcher[%d] exits with %d\n", (int)pid, exit_code);
        fflush(stdout);
        _exit(exit_code);
    }

    /* safety net: just exit */
    write_result_file(pid, sent_alerts);
    int exit_code = (sent_alerts > 255) ? 255 : (int)sent_alerts;
    printf("Watcher[%d] exits with %d\n", (int)pid, exit_code);
    fflush(stdout);
    _exit(exit_code);
}

/* --- RODZIC: tworzenie watcherów, alarm, zbieranie wyników i raport --- */

void usage(const char *name)
{
    fprintf(stderr, "Usage: %s t k n p\n", name);
    fprintf(stderr, " t - simulation time seconds (1..100)\n");
    fprintf(stderr, " k - time after activation to pick up (1..100)\n");
    fprintf(stderr, " n - number of watchers (1..30)\n");
    fprintf(stderr, " p - probability percent (1..100)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 5) usage(argv[0]);

    int t = atoi(argv[1]);
    int k = atoi(argv[2]);
    int n = atoi(argv[3]);
    int p = atoi(argv[4]);

    if (t < 1 || t > 100 || k < 1 || k > 100 || n < 1 || n > 30 || p < 1 || p > 100) usage(argv[0]);

    /* ustawiamy własną grupę procesów (rodzic jako lider) - dzieci będą w tej samej grupie */
    if (setpgid(0, 0) < 0) ERR("setpgid");

    /* ignorujemy SIGUSR1 w rodzicu - chcemy żeby sygnały grupowe trafiły tylko do watcherów */
    struct sigaction ign;
    memset(&ign, 0, sizeof(ign));
    ign.sa_handler = SIG_IGN;
    if (-1 == sigaction(SIGUSR1, &ign, NULL)) ERR("sigaction ignore SIGUSR1");

    /* ustaw handler SIGALRM (który wyśle SIGTERM do grupy) */
    struct sigaction salrm;
    memset(&salrm, 0, sizeof(salrm));
    salrm.sa_handler = parent_sigalrm_handler;
    if (-1 == sigaction(SIGALRM, &salrm, NULL)) ERR("sigaction SIGALRM");

    pid_t *children = calloc((size_t)n, sizeof(pid_t));
    if (!children) ERR("calloc");

    int *ids = calloc((size_t)n, sizeof(int));
    if (!ids) ERR("calloc");

    for (int i = 0; i < n; ++i) ids[i] = i % 10; /* id 0..9 cycling */

    /* fork watchers */
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");
        if (pid == 0) {
            /* child */
            /* ensure child part of same group as parent (inherited) */
            watcher_process(ids[i], k, p);
            /* never returns */
            _exit(EXIT_FAILURE);
        } else {
            children[i] = pid;
        }
    }

    /* wypisz informację i ustaw alarm */
    printf("FC[%d]: Alarm set for %d sec\n", (int)getpid(), t);
    fflush(stdout);
    if (alarm((unsigned) t) != 0) {
        /* if alarm was already set - ignore */
    }

    /* teraz czekamy na dzieci; chcemy wiedzieć, które zakończyły się przed alarmem (picked up)
       aby to osiągnąć: pętla sprawdza WNOHANG dopóki nie pojawi się flag simulation_ended.
       po ustawieniu simulation_ended handler SIGALRM wysyła SIGTERM do grupy; potem czekamy blokująco. */

    int remaining = n;
    int *reaped = calloc((size_t)n, sizeof(int));
    long *exit_counts = calloc((size_t)n, sizeof(long)); /* store exit code or -1 */
    int *picked_up = calloc((size_t)n, sizeof(int)); /* 1 = picked up before simulation end */
    if (!reaped || !exit_counts || !picked_up) ERR("calloc2");
    for (int i = 0; i < n; ++i) { exit_counts[i] = -1; reaped[i] = 0; picked_up[i] = 0; }

    /* najpierw pętla sprawdzająca WNOHANG dopóki nie dostaniemy SIGALRM (simulation_ended) */
    while (!simulation_ended && remaining > 0) {
        for (int i = 0; i < n; ++i) {
            if (reaped[i]) continue;
            int status;
            pid_t r = waitpid(children[i], &status, WNOHANG);
            if (r == 0) {
                continue; /* not exited */
            }
            if (r == -1) {
                if (errno == ECHILD) { reaped[i] = 1; remaining--; continue; }
                ERR("waitpid WNOHANG");
            }
            /* child exited before simulation end */
            reaped[i] = 1;
            remaining--;
            if (WIFEXITED(status)) {
                exit_counts[i] = (long)WEXITSTATUS(status);
            } else {
                exit_counts[i] = -1;
            }
            picked_up[i] = 1; /* since this happened before alarm fired */
            /* continue scanning */
        }
        ms_sleep(50); /* nie spinujemy na 100% */
    }

    /* jeśli simulation_ended ustawiona handlerem - handler wysłał SIGTERM do grupy i wypisał nic nie (w handlerze nie drukujemy).
       Teraz musimy zebrać pozostałe dzieci (blokująco). */
    if (simulation_ended) {
        printf("FC[%d]: Simulation ended\n", (int)getpid());
        fflush(stdout);
    }

    /* collect remaining children blocking */
    for (int i = 0; i < n; ++i) {
        if (reaped[i]) continue;
        int status;
        pid_t r = waitpid(children[i], &status, 0);
        if (r < 0) ERR("waitpid blocking");
        reaped[i] = 1;
        if (WIFEXITED(status)) {
            exit_counts[i] = (long)WEXITSTATUS(status);
        } else {
            exit_counts[i] = -1;
        }
        /* If simulation_ended was 0 when they exited they were picked up; otherwise they were terminated by simulation end.
           But we already marked picks that occurred before alarm. Here if simulation_ended==1 then these exited after sim end -> not picked up. */
        if (!picked_up[i]) {
            if (simulation_ended == 0) picked_up[i] = 1; else picked_up[i] = 0;
        }
    }

    /* przygotowanie raportu: chcemy wypisać pełne liczby alertów. Jeśli exit_codes == 255, odczytujemy plik watcher_<pid>.txt */
    long *full_counts = calloc((size_t)n, sizeof(long));
    if (!full_counts) ERR("calloc full_counts");
    for (int i = 0; i < n; ++i) {
        full_counts[i] = exit_counts[i];
        if (exit_counts[i] == 255) {
            /* próbujemy odczytać plik z pełną wartością */
            char name[64];
            snprintf(name, sizeof(name), "watcher_%d.txt", (int)children[i]);
            int fd = open(name, O_RDONLY);
            if (fd >= 0) {
                char buf[128];
                ssize_t r = bulk_read(fd, buf, sizeof(buf)-1);
                if (r > 0) {
                    buf[r] = '\0';
                    long val = atol(buf);
                    full_counts[i] = val;
                }
                close(fd);
            }
        }
        if (full_counts[i] < 0) full_counts[i] = 0;
    }

    /* final summary format */
    printf("No. | Watcher ID | Status\n");
    int stayed = 0;
    for (int i = 0; i < n; ++i) {
        if (picked_up[i]) {
            printf("%3d | %8d | Picked up after %ld alerts\n", i+1, (int)children[i], full_counts[i]);
        } else {
            printf("%3d | %8d | Emitted %ld alerts and still in the forest!\n", i+1, (int)children[i], full_counts[i]);
            stayed++;
        }
    }
    printf("%d out of %d watchers stayed in the forest!\n", stayed, n);

    /* sprzątanie */
    free(children);
    free(ids);
    free(reaped);
    free(exit_counts);
    free(picked_up);
    free(full_counts);

    return 0;
}
