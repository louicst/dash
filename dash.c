#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 100
#define MAX_PATHS 100
#define ERROR_MESSAGE "An error has occurred\n"

// === Variables globales ===
char *search_paths[MAX_PATHS];
int path_count = 0;

// === Prototypes ===
void shell_loop(void);
int handle_builtin(char *line);
void set_path(char **argv);
char **parse_command(char *line);
void execute_command(char **argv);
void print_error(void);

// === Main ===
int main(void) {
    // Valeur initiale du path : /bin
    search_paths[0] = strdup("/bin");
    path_count = 1;

    shell_loop();
    return 0;
}

// === Boucle principale ===
void shell_loop(void) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("dash> ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) break;

        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        if (handle_builtin(line)) break;

        char **argv = parse_command(line);
        if (argv[0] != NULL) {
            execute_command(argv);
        }
        free(argv);
    }

    free(line);
}

// === Commandes intégrées : exit, path ===
int handle_builtin(char *line) {
    // Duplique la ligne avant parsing
    char *line_copy = strdup(line);
    char *ptr = line_copy;
    char *token = strsep(&ptr, " \t");

    if (token == NULL) {
        free(line_copy);
        return 0;
    }

    if (strcmp(token, "exit") == 0) {
        // Vérifie qu’il n’y a pas d’argument après
        token = strsep(&ptr, " \t");
        if (token != NULL && *token != '\0') {
            print_error();
            free(line_copy);
            return 0;
        }
        free(line_copy);
        return 1;
    }

    if (strcmp(token, "path") == 0) {
        // Récupère les arguments
        char *args_line = ptr;
        char **argv = parse_command(args_line);

        // Met à jour la liste des chemins
        set_path(argv);
        free(argv);
        free(line_copy);
        return 0;
    }

    free(line_copy);
    return 0;
}

// === Mise à jour du path ===
void set_path(char **argv) {
    // Libère les anciens chemins
    for (int i = 0; i < path_count; i++) {
        free(search_paths[i]);
    }
    path_count = 0;

    // Enregistre les nouveaux chemins
    for (int i = 0; argv[i] != NULL && i < MAX_PATHS; i++) {
        search_paths[path_count++] = strdup(argv[i]);
    }
}

// === Parsing d’une ligne en argv ===
char **parse_command(char *line) {
    char **argv = malloc(sizeof(char *) * MAX_ARGS);
    if (!argv) {
        print_error();
        exit(1);
    }

    int argc = 0;
    char *token;

    while ((token = strsep(&line, " \t")) != NULL) {
        if (*token == '\0') continue;
        argv[argc++] = token;
        if (argc >= MAX_ARGS - 1) break;
    }
    argv[argc] = NULL;
    return argv;
}

// === Exécution des commandes externes ===
void execute_command(char **argv) {
    if (path_count == 0) {
        print_error();
        return;
    }

    char full_path[512];
    int found = 0;

    for (int i = 0; i < path_count; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", search_paths[i], argv[0]);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        print_error();
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
    } else if (pid == 0) {
        execv(full_path, argv);
        print_error();
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }
}

// === Message d’erreur standard ===
void print_error(void) {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}