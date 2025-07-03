#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

// On inclut les prototypes nécessaires du shell
#define MAX_ARGS 100
#define MAX_PATHS 100
extern char *search_paths[MAX_PATHS];
extern int path_count;
char **parse_command(char *line);
void set_path(char **argv);
int handle_builtin(char *line);

// Pour les tests, on utilise les variables globales du shell (pas de redéfinition ici)

void free_paths() {
    for (int i = 0; i < path_count; i++) {
        free(search_paths[i]);
    }
    path_count = 0;
}

void test_parse_command() {
    printf("Test parse_command... ");
    char input[] = "ls -l /tmp";
    char **argv = parse_command(input);
    int ok = 1;
    if (!argv[0] || strcmp(argv[0], "ls") != 0) ok = 0;
    if (!argv[1] || strcmp(argv[1], "-l") != 0) ok = 0;
    if (!argv[2] || strcmp(argv[2], "/tmp") != 0) ok = 0;
    if (argv[3] != NULL) ok = 0;
    if (ok) printf("OK\n"); else printf("FAIL\n");
    free(argv);
}

void test_set_path() {
    printf("Test set_path... ");
    free_paths();
    char *args[] = {"/bin", "/usr/bin", NULL};
    set_path(args);
    int ok = 1;
    if (path_count != 2) ok = 0;
    if (!search_paths[0] || strcmp(search_paths[0], "/bin") != 0) ok = 0;
    if (!search_paths[1] || strcmp(search_paths[1], "/usr/bin") != 0) ok = 0;
    if (ok) printf("OK\n"); else printf("FAIL\n");
    free_paths();
}

void test_cd() {
    printf("Test cd... ");
    char cwd[PATH_MAX];
    char tmpdir[] = "./cd_test_dir";
    int ok = 1;
    if (mkdir(tmpdir, 0700) != 0) {
        perror("mkdir");
        ok = 0;
    }
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        ok = 0;
    }
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "cd %s", tmpdir);
    printf("\n  Avant cd: %s\n", cwd);
    handle_builtin(cmd);
    char newcwd[PATH_MAX];
    if (getcwd(newcwd, sizeof(newcwd)) == NULL) {
        perror("getcwd");
        ok = 0;
    }
    printf("  Après cd: %s\n", newcwd);
    // Vérifie que le chemin courant se termine bien par cd_test_dir
    const char *suffix = "/cd_test_dir";
    size_t suffix_len = strlen(suffix);
    size_t len = strlen(newcwd);
    int ok_cd = (len >= suffix_len && strcmp(newcwd + len - suffix_len, suffix) == 0);
    printf("    cd normal: %s\n", ok_cd ? "OK" : "FAIL");
    // Test cd sans argument
    int ok_empty = (handle_builtin("cd") == 0);
    printf("    cd sans argument: %s\n", ok_empty ? "OK" : "FAIL");
    // Test cd avec trop d'arguments
    int ok_too_many = (handle_builtin("cd dir1 dir2") == 0);
    printf("    cd trop d'arguments: %s\n", ok_too_many ? "OK" : "FAIL");
    // Test cd vers dossier inexistant
    int ok_inex = (handle_builtin("cd /doesnotexist12345") == 0);
    printf("    cd inexistant: %s\n", ok_inex ? "OK" : "FAIL");
    // Retour au dossier initial
    chdir(cwd);
    rmdir(tmpdir);
    if (ok_cd && ok_empty && ok_too_many && ok_inex && ok) printf("Test cd... OK\n"); else printf("Test cd... FAIL\n");
}

void test_parse_redirection() {
    printf("Test parse redirection... ");
    char input[] = "ls -l < dash.c";
    char **argv = parse_command(input);
    int ok = 1;
    int found = 0;
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "<") == 0) found = 1;
    }
    if (!found) ok = 0;
    if (strcmp(argv[0], "ls") != 0) ok = 0;
    if (strcmp(argv[1], "-l") != 0) ok = 0;
    if (strcmp(argv[2], "<") != 0) ok = 0;
    if (strcmp(argv[3], "dash.c") != 0) ok = 0;
    if (ok) printf("OK\n"); else printf("FAIL\n");
    free(argv);
}

void test_cd_errors() {
    printf("Test cd errors... ");
    int ok = 1;
    // cd sans argument
    if (handle_builtin("cd") != 0) ok = 0;
    // cd trop d'arguments
    if (handle_builtin("cd dir1 dir2") != 0) ok = 0;
    // cd dossier inexistant
    if (handle_builtin("cd /doesnotexist12345") != 0) ok = 0;
    if (ok) printf("OK\n"); else printf("FAIL\n");
}

void test_path_empty() {
    printf("Test path empty... ");
    // Vide le path
    char *args[] = {NULL};
    set_path(args);
    // On ne peut pas tester execute_command directement car elle fork, mais on peut vérifier path_count
    int ok = (path_count == 0);
    if (ok) printf("OK\n"); else printf("FAIL\n");
}

int main() {
    test_parse_command();
    test_set_path();
    test_cd();
    test_parse_redirection();
    test_cd_errors();
    test_path_empty();
    return 0;
} 