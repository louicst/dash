#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// On inclut les prototypes nécessaires du shell
#define MAX_ARGS 100
#define MAX_PATHS 100
extern char *search_paths[MAX_PATHS];
extern int path_count;
char **parse_command(char *line);
void set_path(char **argv);

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

int main() {
    test_parse_command();
    test_set_path();
    return 0;
} 