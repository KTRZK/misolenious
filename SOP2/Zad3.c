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

/* --- pomocnicze z Twojego startowego kodu --- */

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

/* --- KONFIGURACJA STOPU --- */
#define MAX_ROUNDS 1000          /* maksymalna liczba token-passingów (bezpiecznik) */
#define MAX_COUNT_PER_CHILD 1000 /* maks iteracji pojedynczego dziecka (bezpiecznik) */

/* --- zmienne globalne rodzica --- */
static pid_t *g_children = NULL;
static int g_nchildren = 0;
static volatile sig_atomic_t g_current = 0;

/* flaga ustawiana przez handler i zapisująca PID nadawcy */
static volatile sig_atomic_t parent_got_sig = 0;
static volatile sig_atomic_t parent_sender_pid = 0;

/* handler SIGUSR1 rodzica - lekki: tylko zapisujemy info o sygnale */
static void parent_sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig; (void)ucontext;
    if (info && info->si_pid > 0) {
        parent_sender_pid = (sig_atomic_t)info->si_pid;
    } else {
        parent_sender_pid = 0;
    }
    parent_got_sig = 1;
}

/* handler SIGINT rodzica - prześlij SIGINT do dzieci (bezpiecznie) */
static volatile sig_atomic_t parent_got_sigint = 0;
static void parent_sigint_handler(int signo)
{
    (void)signo;
    parent_got_sigint = 1;
}

/* --- HANDLERY DZIECI --- */
static volatile sig_atomic_t child_running = 0;
static volatile sig_atomic_t child_terminate = 0;

/* SIGUSR1 -> resume */
static void child_sigusr1_handler(int signo)
{
    (void)signo;
    child_running = 1;
}

/* SIGUSR2 -> pause */
static void child_sigusr2_handler(int signo)
{
    (void)signo;
    child_running = 0;
}

/* SIGINT -> terminate (set flag, write file outside handler) */
static void child_sigint_handler(int signo)
{
    (void)signo;
    child_terminate = 1;
}

/* zapis licznika do pliku <PID>.txt (niskopoziomowo) */
static void write_counter_to_file(pid_t pid, long counter)
{
    char name[64];
    snprintf(name, sizeof(name), "%d.txt", (int)pid);
    int fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) return;
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%ld\n", counter);
    if (len > 0) {
        (void) bulk_write(fd, buf, (size_t)len);
    }
    close(fd);
}

/* logika dziecka */
static void child_main_loop(int index)
{
    pid_t pid = getpid();
    pid_t ppid = getppid();

    srand((unsigned)time(NULL) ^ (unsigned)pid ^ (unsigned)index);

    /* ustawione handlery (proste) */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = child_sigusr1_handler; /* actually uses sa_handler in sethandler helper below */
    if (sigaction(SIGUSR1, &(struct sigaction){.sa_handler = child_sigusr1_handler}, NULL) < 0) ERR("sigaction SIGUSR1");
    if (sigaction(SIGUSR2, &(struct sigaction){.sa_handler = child_sigusr2_handler}, NULL) < 0) ERR("sigaction SIGUSR2");
    if (sigaction(SIGINT, &(struct sigaction){.sa_handler = child_sigint_handler}, NULL) < 0) ERR("sigaction SIGINT");

    /* początkowy stan: niepracujący; będzie wznowiony przez rodzica */
    child_running = 0;
    child_terminate = 0;

    /* wydrukuj PID i index */
    printf("%d: %d\n", (int)pid, index);
    fflush(stdout);

    long counter = 0;

    /* maska sygnałów: chcemy móc czekać na SIGUSR1/SIGINT/SIGUSR2 */
    sigset_t mask_all, mask_wait;
    sigfillset(&mask_all);
    sigemptyset(&mask_wait);
    sigaddset(&mask_wait, SIGUSR1);
    sigaddset(&mask_wait, SIGUSR2);
    sigaddset(&mask_wait, SIGINT);

    /* Main loop */
    while (!child_terminate) {
        if (child_running) {
            /* robota: sleep 100..200 ms, ++counter, wypisz, powiadom rodzica */
            int delay = (rand() % 101) + 100; /* 100..200 */
            ms_sleep((unsigned)delay);

            counter++;
            printf("%d: %ld\n", (int)pid, counter);
            fflush(stdout);

            /* warn parent - finished one iteration */
            if (kill(ppid, SIGUSR1) < 0) {
                if (errno == ESRCH) break; /* parent died */
            }

            if (counter >= MAX_COUNT_PER_CHILD) {
                /* safety exit */
                write_counter_to_file(pid, counter);
                int exitcode = (counter > 255) ? 255 : (int)counter;
                _exit(exitcode);
            }

            /* after signaling parent, child expects parent to pause it by SIGUSR2 (unless parent passes token back) */
            /* busy-waiting here is bad; better to pause until paused (child_running==0) or terminate */
            while (child_running && !child_terminate) {
                /* wait for signals - avoid busy loop */
                pause();
            }
        } else {
            /* paused - wait for signals (SIGUSR1 to resume or SIGINT) */
            pause();
        }
    }

    /* kończymy: zapisz wynik i wyjdź */
    write_counter_to_file(pid, counter);
    int exitcode = (counter > 255) ? 255 : (int)counter;
    _exit(exitcode);
}

/* ---------- RODZIC ---------- */

void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s N\n", prog);
    fprintf(stderr, " N - number of worker children (1..30)\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2) usage(argv[0]);
    int N = atoi(argv[1]);
    if (N < 1 || N > 30) usage(argv[0]);

    /* zapewnij, że jesteśmy liderem grupy procesów */
    if (setpgid(0, 0) < 0) ERR("setpgid");

    g_children = calloc((size_t)N, sizeof(pid_t));
    if (!g_children) ERR("calloc");
    g_nchildren = N;
    g_current = 0;

    /* ustaw handler SIGUSR1 z SA_SIGINFO aby pozyskac si_pid nadawcy */
    struct sigaction sa_parent;
    memset(&sa_parent, 0, sizeof(sa_parent));
    sa_parent.sa_sigaction = parent_sigusr1_handler;
    sa_parent.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(SIGUSR1, &sa_parent, NULL)) ERR("sigaction parent SIGUSR1");

    /* handler SIGINT w rodzicu: ustaw flagę, a w pętli wyślemy SIGINT do dzieci */
    sethandler(parent_sigint_handler, SIGINT);

    /* fork dzieci */
    for (int i = 0; i < N; ++i) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");
        if (pid == 0) {
            /* child path */
            child_main_loop(i);
            _exit(EXIT_FAILURE);
        } else {
            g_children[i] = pid;
        }
    }

    /* daj dzieciom chwilę na ustawienie handlerów i wypisanie PID/index */
    ms_sleep(100);

    /* uruchom pierwszy child - wyślij SIGUSR1 */
    if (g_children[0] > 0) {
        if (kill(g_children[0], SIGUSR1) < 0) {
            if (errno != ESRCH) ERR("kill start SIGUSR1");
        }
    }

    /* teraz czekamy na sygnały od dzieci; główna pętla wykonuje przełączanie tokena
       gdy otrzyma flagę parent_got_sig ustawioną przez handler. */
    int rounds = 0;
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    /* blokujemy SIGUSR1 podczas sprawdzania flagi i wykonywania akcji, potem sigsuspend */
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask) < 0) ERR("sigprocmask");

    while (1) {
        /* jeśli dostalismy SIGINT (Ctrl-C) w rodzicu - przekaż do dzieci i zakończ */
        if (parent_got_sigint) {
            /* prześlij SIGINT do grupy procesów (dzieci) */
            TEMP_FAILURE_RETRY(killpg(getpgrp(), SIGINT));
            break;
        }

        /* jeśli handler ustawił parent_got_sig - obsłuż tę informację tutaj */
        if (parent_got_sig) {
            parent_got_sig = 0;
            pid_t sender = (pid_t)parent_sender_pid;
            parent_sender_pid = 0;

            /* zatrzymaj obecnego pracownika */
            pid_t curpid = g_children[g_current];
            if (curpid > 0) {
                TEMP_FAILURE_RETRY(kill(curpid, SIGUSR2));
            }

            /* wybierz następnego i wznow jego pracę */
            g_current = (g_current + 1) % g_nchildren;
            pid_t nextpid = g_children[g_current];
            if (nextpid > 0) {
                TEMP_FAILURE_RETRY(kill(nextpid, SIGUSR1));
            }

            rounds++;
            if (rounds >= MAX_ROUNDS) {
                /* safety stop: powiadom dzieci i zakończ */
                TEMP_FAILURE_RETRY(killpg(getpgrp(), SIGINT));
                break;
            }
        }

        /* teraz czekamy na SIGUSR1 lub SIGINT */
        /* odmaskuj SIGUSR1 i czekaj atomowo, potem wrócimy tu i obsłużymy flagę */
        sigsuspend(&oldmask);
        /* po powrocie z sigsuspend ponownie sprawdzimy flagę w topie pętli */
    }

    /* musimy zebrać wszystkie dzieci (blokująco) */
    int remaining = g_nchildren;
    while (remaining > 0) {
        int status;
        pid_t r = wait(&status);
        if (r == -1) {
            if (errno == EINTR) continue;
            if (errno == ECHILD) break;
            ERR("wait");
        }
        remaining--;
    }

    free(g_children);
    g_children = NULL;
    return 0;
}
