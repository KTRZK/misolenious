#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

/* pomocnicze */
static void ms_sleep(unsigned int milli)
{
    time_t sec = (int)(milli / 1000);
    milli = milli - (sec * 1000);
    struct timespec ts = { .tv_sec = sec, .tv_nsec = milli * 1000000L };
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
}

/* --- GLOBAL dla rodzica --- */
static volatile sig_atomic_t simulation_ended = 0;
static pid_t *children = NULL;
static int nchildren = 0;
static pid_t my_pgid = 0;

/* Parent: handler SIGALRM - oznacz kończenie i wyślij SIGTERM do grupy */
/* Używamy kill z negative pid (-(pgid)) - async-signal-safe. */
static void parent_sigalrm_handler(int signo)
{
    (void)signo;
    simulation_ended = 1;
    if (my_pgid > 0) {
        /* send SIGTERM to group (negative pgid) */
        TEMP_FAILURE_RETRY(kill(-my_pgid, SIGTERM));
    }
}

/* Parent: ignore SIGUSR1 so that group signals don't disturb parent printing. */
 
/* --- HANDLERY w dzieciach --- */
/* Dzieci używają tych flag i wartości aby obsłużyć sygnały poza handlerami */
static volatile sig_atomic_t got_cough_signal = 0;
static volatile sig_atomic_t last_sender_pid = 0; /* zapis z siginfo_t */
static volatile sig_atomic_t picked_up_flag = 0;  /* set by SIGALRM when rescue arrives */
static volatile sig_atomic_t term_flag = 0;       /* set by SIGTERM */

/* SIGUSR1 handler w dziecku (SA_SIGINFO) — lekkie: zapisujemy PID nadawcy i flagę */
static void child_sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig; (void)ucontext;
    if (info && info->si_pid > 0) {
        last_sender_pid = (sig_atomic_t)info->si_pid;
    } else {
        last_sender_pid = 0;
    }
    got_cough_signal = 1;
}

/* SIGALRM w dziecku — oznacza że przyszli rodzice i odbierają chore dziecko (picked up) */
static void child_sigalrm_handler(int signo)
{
    (void)signo;
    picked_up_flag = 1;
}

/* SIGTERM w dziecku — koniec symulacji */
static void child_sigterm_handler(int signo)
{
    (void)signo;
    term_flag = 1;
}

/* utility: losowy int w [a,b] */
static int rand_range(int a, int b)
{
    return a + rand() % (b - a + 1);
}

/* --- LOGIKA DZIECKA --- */
static void child_process(int k_seconds, int p_percent, int is_initially_sick)
{
    pid_t pid = getpid();

    /* ustawienie handlera SIGUSR1 z SA_SIGINFO */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = child_sigusr1_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) ERR("sigaction SIGUSR1");

    /* SIGALRM (for being picked up) */
    if (signal(SIGALRM, child_sigalrm_handler) == SIG_ERR) ERR("signal SIGALRM");
    /* SIGTERM */
    if (signal(SIGTERM, child_sigterm_handler) == SIG_ERR) ERR("signal SIGTERM");

    /* losujemy początkowy sleep 300..1000 ms (etap 1) */
    srand((unsigned)(time(NULL) ^ (unsigned)pid));
    int initial_sleep = rand_range(300, 1000);
    printf("Child[%d] starts day in the kindergarten, ill: %d\n", (int)pid, is_initially_sick ? 1 : 0);
    fflush(stdout);
    ms_sleep((unsigned)initial_sleep);

    int sick = is_initially_sick ? 1 : 0;
    int cough_count = 0;

    /* jeśli na starcie jest chory — ustaw alarm k sekund na odbiór */
    if (sick) {
        alarm((unsigned)k_seconds);
    }

    /* pętla główna potomka */
    while (1) {
        /* najpierw obsłuż sygnały typu przychodzące (got_cough_signal) */
        if (got_cough_signal) {
            int sender = (int)last_sender_pid;
            got_cough_signal = 0;
            last_sender_pid = 0;
            /* ignoruj własne sygnały (nadawca == ja) */
            if (sender != (int)pid && sender != 0) {
                printf("Child[%d]: %d has coughed at me!\n", (int)pid, sender);
                fflush(stdout);
                /* zdrowe dziecko może się zarazić z prawdopodobieństwem p_percent */
                if (!sick) {
                    int roll = rand_range(1, 100);
                    if (roll <= p_percent) {
                        sick = 1;
                        printf("Child[%d] get sick!\n", (int)pid);
                        fflush(stdout);
                        /* ustaw alarm k sekund po zachorowaniu */
                        alarm((unsigned)k_seconds);
                    }
                }
            }
        }

        /* jeśli odebrano przez rodziców (picked_up_flag) - zakończ zwracając cough_count */
        if (picked_up_flag) {
            /* informacja o zakończeniu powinna być wypisana przez dziecko przed exit */
            int exit_code = (cough_count > 255) ? 255 : cough_count;
            printf("Child[%d] exits with %d\n", (int)pid, exit_code);
            fflush(stdout);
            _exit(exit_code);
        }

        /* jeśli dostaliśmy SIGTERM (koniec symulacji), zakończ zwracając licznik */
        if (term_flag) {
            int exit_code = (cough_count > 255) ? 255 : cough_count;
            printf("Child[%d] exits with %d\n", (int)pid, exit_code);
            fflush(stdout);
            _exit(exit_code);
        }

        /* jeśli jesteśmy chorzy -> kaszlemy co 50..200 ms i wysyłamy SIGUSR1 do grupy */
        if (sick) {
            int delay = rand_range(50, 200);
            ms_sleep((unsigned)delay);

            cough_count++;
            printf("Child[%d] is coughing (%d)\n", (int)pid, cough_count);
            fflush(stdout);

            /* wyślij sygnał do wszystkich w grupie procesów (kill with negative pgid):
               parent may receive it, so parent should ignore SIGUSR1. */
            /* use kill with -(pgid) */
            if (kill(- (int)getpgrp(), SIGUSR1) < 0) {
                if (errno != ESRCH) {
                    /* ignore if group gone, otherwise print error (not fatal) */
                    perror("kill group");
                }
            }
            /* po wysłaniu sygnału wrócimy do obsługi przychodzących sygnałów i ewentualnego picked_up/term */
            continue;
        } else {
            /* niechorzy: czekaj na sygnały - użyj pause() (przerwie gdy dostaniemy SIGUSR1/SIGALRM/SIGTERM) */
            pause();
            /* po wznowieniu pętla wróci i obsłuży flagi */
        }
    }
}

/* --- RODZIC: main i zbieranie wyników --- */
static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s t k n p\n", prog);
    fprintf(stderr, " t - simulation time seconds (1..100)\n");
    fprintf(stderr, " k - time until parents pick sick child (1..100)\n");
    fprintf(stderr, " n - number children (1..30)\n");
    fprintf(stderr, " p - infection probability percent (1..100)\n");
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

    /* ustaw grupę procesów - rodzic lideruje grupie */
    if (setpgid(0, 0) < 0) ERR("setpgid");
    my_pgid = getpgrp();

    /* ignoruj SIGUSR1 w rodzicu żeby nie reagował na kaszel dzieci */
    struct sigaction ign;
    memset(&ign, 0, sizeof(ign));
    ign.sa_handler = SIG_IGN;
    if (sigaction(SIGUSR1, &ign, NULL) < 0) ERR("sigaction ignore SIGUSR1");

    /* ustaw handler SIGALRM w rodzicu (oznaczenie zakończenia symulacji) */
    struct sigaction salrm;
    memset(&salrm, 0, sizeof(salrm));
    salrm.sa_handler = parent_sigalrm_handler;
    if (sigaction(SIGALRM, &salrm, NULL) < 0) ERR("sigaction SIGALRM");

    children = calloc((size_t)n, sizeof(pid_t));
    if (children == NULL) ERR("calloc children");
    nchildren = n;

    /* forkuj dzieci */
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) ERR("fork");
        if (pid == 0) {
            /* child: first child (i==0) initially sick */
            child_process(k, p, i == 0 ? 1 : 0);
            _exit(EXIT_FAILURE); /* never returns */
        } else {
            children[i] = pid;
        }
    }

    /* wypisz informację i ustaw alarm (symulacja t sekund) */
    printf("KG[%d]: Alarm has been set for %d sec\n", (int)getpid(), t);
    fflush(stdout);
    alarm((unsigned)t);

    /* Rodzic: przed alarmem reapsuje dzieci z WNOHANG aby wykryć odebrane dzieci.
       Po ustawieniu simulation_ended handler SIGALRM już wysłał SIGTERM do grupy. */
    int remaining = n;
    int *reaped = calloc((size_t)n, sizeof(int));
    int *picked_up = calloc((size_t)n, sizeof(int)); /* 1 jeśli child ended before simulation end */
    int *exit_codes = calloc((size_t)n, sizeof(int)); /* stored exit status (0..255) */
    if (!reaped || !picked_up || !exit_codes) ERR("calloc aux");

    for (int i = 0; i < n; ++i) { reaped[i] = 0; picked_up[i] = 0; exit_codes[i] = -1; }

    /* pętla: dopóki nie ustawiono flagi simulation_ended, co pewien czas sprawdzaj WNOHANG */
    while (!simulation_ended && remaining > 0) {
        for (int i = 0; i < n; ++i) {
            if (reaped[i]) continue;
            int status;
            pid_t r = waitpid(children[i], &status, WNOHANG);
            if (r == 0) continue; /* nadal żyje */
            if (r == -1) {
                if (errno == ECHILD) {
                    reaped[i] = 1;
                    remaining--;
                    picked_up[i] = 1; /* assume picked up if exited before alarm */
                    continue;
                } else {
                    ERR("waitpid WNOHANG");
                }
            }
            /* child zakończył przed simulation end */
            reaped[i] = 1;
            remaining--;
            picked_up[i] = 1;
            if (WIFEXITED(status)) {
                exit_codes[i] = WEXITSTATUS(status);
            } else {
                exit_codes[i] = -1;
            }
        }
        ms_sleep(50);
    }

    /* jeśli alarm się uruchomił, handler wysłał SIGTERM do grupy, wypisz informację końcową */
    if (simulation_ended) {
        printf("KG[%d]: Simulation has ended!\n", (int)getpid());
        fflush(stdout);
    }

    /* teraz zbierz pozostałe dzieci blokująco */
    for (int i = 0; i < n; ++i) {
        if (reaped[i]) continue;
        int status;
        pid_t r = waitpid(children[i], &status, 0);
        if (r < 0) ERR("waitpid blocking");
        reaped[i] = 1;
        if (WIFEXITED(status)) {
            exit_codes[i] = WEXITSTATUS(status);
        } else {
            exit_codes[i] = -1;
        }
        /* jeśli reaped after simulation_ended -> not picked up (they left at sim end) */
        if (!picked_up[i]) {
            picked_up[i] = 0;
        }
    }

    /* wypisz raport */
    printf("No. | Child ID | Status\n");
    int stayed = 0;
    for (int i = 0; i < n; ++i) {
        int code = exit_codes[i];
        pid_t cid = children[i];
        if (picked_up[i]) {
            /* child ended before simulation end -> parents picked them up */
            int coughs = (code >= 0) ? code : 0;
            printf("%3d | %8d | Coughed %d times and parents picked them up!\n", i+1, (int)cid, coughs);
        } else {
            int coughs = (code >= 0) ? code : 0;
            printf("%3d | %8d | Coughed %d times and is still in the kindergarten!\n", i+1, (int)cid, coughs);
            stayed++;
        }
    }
    printf("%d out of %d children stayed at in the kindergarten!\n", stayed, n);

    free(children);
    free(reaped);
    free(picked_up);
    free(exit_codes);

    return 0;
}
