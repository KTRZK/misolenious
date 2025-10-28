/* === sop-venv.c ===
 * Simple virtual environment manager (create, install, remove).
 * gcc -Wall -Wextra -std=c11 sop-venv.c -o sop-venv
 */
#include "util.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

void usage(const char *p) { fprintf(stderr, "Usage examples: %s -c -v env | %s -v env -i name==ver | %s -v env -r name\n", p,p,p); exit(EXIT_FAILURE); }

int env_exists(const char *env) {
    struct stat st; return (stat(env, &st) == 0 && S_ISDIR(st.st_mode));
}

int create_env(const char *env) {
    if (env_exists(env)) { fprintf(stderr, "Environment %s already exists\n", env); return -1; }
    if (mkdir(env, 0755) != 0) { perror("mkdir"); return -1; }
    char req[4096]; snprintf(req, sizeof(req), "%s/requirements", env);
    FILE *f = fopen(req, "w"); if (!f) { perror("fopen"); return -1; } fclose(f);
    return 0;
}

int append_line_if_missing(const char *path, const char *line) {
    FILE *f = fopen(path, "r+");
    if (!f) { if (errno == ENOENT) f = fopen(path, "w+"); else return -1; }
    char buf[512]; rewind(f);
    while (fgets(buf, sizeof(buf), f)) if (strncmp(buf, line, strlen(line)) == 0) { fclose(f); return 0; }
    fseek(f, 0, SEEK_END);
    fprintf(f, "%s\n", line);
    fclose(f);
    return 1;
}

int install_pkg(const char *env, const char *pkg_eq_ver) {
    if (!env_exists(env)) { fprintf(stderr, "the environment does not exist: %s\n", env); return -1; }
    char pkg[256], ver[256];
    char *eq = strstr(pkg_eq_ver, "==");
    if (!eq) { fprintf(stderr, "Bad package format\n"); return -1; }
    snprintf(pkg, eq - pkg_eq_ver + 1, "%s", pkg_eq_ver);
    strcpy(ver, eq + 2);
    char reqpath[4096]; snprintf(reqpath, sizeof(reqpath), "%s/requirements", env);
    if (append_line_if_missing(reqpath, (char*)pkg) < 0) return -1;
    char pkgpath[4096]; snprintf(pkgpath, sizeof(pkgpath), "%s/%s", env, pkg);
    FILE *pf = fopen(pkgpath, "w"); if (!pf) return -1;
    srand((unsigned)time(NULL));
    for (int i=0;i<20;i++) fputc('a' + rand()%26, pf);
    fclose(pf);
    chmod(pkgpath, 0444);
    return 0;
}

int remove_pkg(const char *env, const char *pkg) {
    if (!env_exists(env)) { fprintf(stderr, "the environment does not exist: %s\n", env); return -1; }
    char reqpath[4096]; snprintf(reqpath, sizeof(reqpath), "%s/requirements", env);
    char tmp[4096]; snprintf(tmp, sizeof(tmp), "%s.tmp", reqpath);
    FILE *f = fopen(reqpath, "r"); if (!f) return -1;
    FILE *t = fopen(tmp, "w"); if (!t) { fclose(f); return -1; }
    char name[256], ver[256]; int found = 0;
    while (fscanf(f, "%255s %255s", name, ver) == 2) {
        if (strcmp(name, pkg) == 0) { found=1; continue; }
        fprintf(t, "%s %s\n", name, ver);
    }
    fclose(f); fclose(t);
    if (!found) { remove(tmp); fprintf(stderr, "Package %s not installed\n", pkg); return -1; }
    if (rename(tmp, reqpath) != 0) { remove(tmp); return -1; }
    char pkgpath[4096]; snprintf(pkgpath, sizeof(pkgpath), "%s/%s", env, pkg);
    if (unlink(pkgpath) != 0 && errno != ENOENT) return -1;
    return 0;
}

int main(int argc, char **argv) {
    int opt; int do_create=0; char *envs[64]; int envc=0; char *install_spec=NULL; char *remove_pkg_name=NULL;
    srand((unsigned)time(NULL));
    while ((opt = getopt(argc, argv, "cv:i:r:")) != -1) {
        switch(opt) {
            case 'c': do_create=1; break;
            case 'v': envs[envc++]=optarg; break;
            case 'i': install_spec = optarg; break;
            case 'r': remove_pkg_name = optarg; break;
            default: usage(argv[0]);
        }
    }
    if (envc == 0) usage(argv[0]);
    if (do_create) {
        if (envc != 1) { fprintf(stderr, "Creation expects exactly one -v <env>\n"); usage(argv[0]); }
        return create_env(envs[0]) == 0 ? 0 : 1;
    }
    for (int i=0;i<envc;i++) {
        if (install_spec) if (install_pkg(envs[i], install_spec) != 0) return 1;
        if (remove_pkg_name) if (remove_pkg(envs[i], remove_pkg_name) != 0) return 1;
    }
    return 0;
}
