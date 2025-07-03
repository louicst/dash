#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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
char *trim(char *str);

// === Main ===
/*
int main(int argc, char **argv) {
    //Pas besoin de argv ici, mais main ne prend que 0 ou 2 arguments, donc on fait ça...
    char **inutile = argv;
    inutile++;

    if(argc >1){
        //Interdit d'avoir d'autres arguments que ./dash
        print_error();
        return 0;
    }

    // Valeur initiale du path : /bin
    search_paths[0] = strdup("/bin");
    path_count = 1;

    shell_loop();
    return 0;
}
*/

// === Boucle principale ===
void shell_loop(void) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("dash> ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1){
            break;
        }

        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        if (handle_builtin(line)){
            break;
        }

        char **argv = parse_command(line);

        //On execute la commande sauf si c'était une commande path ou cd
        if ((argv[0] != NULL )&&!((argv[0][0] == 'p') && (argv[0][1] == 'a') && (argv[0][2] == 't') && (argv[0][3] == 'h'))&&!((argv[0][0] == 'c') && (argv[0][1] == 'd')) ) {
            execute_command(argv);
        }
        free(argv);
    }

    free(line);
}

// === Commandes intégrées : exit, path, cd ===
int handle_builtin(char *line) {
    char *line_copy = strdup(line);
    char **argv = parse_command(line_copy);
    int result = 0;

    if (argv[0] == NULL) {
        free(argv);
        free(line_copy);
        return 0;
    }

    if (strcmp(argv[0], "exit") == 0) {
        // exit ne prend pas d'argument
        if (argv[1] != NULL) {
            print_error();
            result = 0;
        } else {
            result = 1; // signaler la sortie du shell
        }
        free(argv);
        free(line_copy);
        return result;
    }

    if (strcmp(argv[0], "path") == 0) {
        // path prend 0 ou plusieurs arguments
        set_path(&argv[1]);
        free(argv);
        free(line_copy);
        return 0;
    }

    if (strcmp(argv[0], "cd") == 0) {
        // cd prend exactement un argument
        if (argv[1] == NULL || argv[2] != NULL) {
            print_error();
        } else {
            if (chdir(argv[1]) != 0) {
                print_error();
            } else {
                char buf[4096];
                if (getcwd(buf, sizeof(buf)) != NULL) {
                    printf("moving to %s\n", buf);
                }
            }
        }
        free(argv);
        free(line_copy);
        return 0;
    }

    free(argv);
    free(line_copy);
    return 0;
}

// === Mise à jour du path ===
void set_path(char **argv) {
    // Libère les anciens chemins
    int i;

    for (i= 0; i < path_count; i++) {
        free(search_paths[i]);
    }
    path_count = 0;

    // Enregistre les nouveaux chemins
    for (i = 0; argv[i] != NULL && i < MAX_PATHS; i++) {
        search_paths[path_count++] = strdup(argv[i]);
    }
}

// === Ajout d'une fonction utilitaire pour trim les espaces
char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) end--;
    *(end + 1) = 0;
    return str;
}

// === Parsing d'une ligne en argv ===
char **parse_command(char *line) {
    char **argv = malloc(sizeof(char *) * MAX_ARGS);
    if (!argv) {
        print_error();
        exit(1);
    }

    int argc = 0;
    char *token;

    while ((token = strsep(&line, " \t")) != NULL) {
        if (*token == '\0'){
            continue;
        }
        token = trim(token);
        if (*token == '\0') continue;
        argv[argc++] = token;
        if (argc >= MAX_ARGS - 1){
            break;
        }
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

    // Recherche d'une éventuelle redirection '<'
    int redirect_index = -1;
    for (int i = 0; argv[i] != NULL; i++) {
        // On ignore les espaces autour de '<'
        if (strcmp(argv[i], "<") == 0) {
            redirect_index = i;
            break;
        }
    }

    char *input_file = NULL;
    char *cmd_argv[MAX_ARGS];
    int i;

    if (redirect_index != -1) {
        // Vérifie la syntaxe : un seul '<', un seul fichier après, rien après le fichier
        if (argv[redirect_index + 1] == NULL || argv[redirect_index + 2] != NULL) {
            print_error();
            return;
        }
        input_file = argv[redirect_index + 1];
        // Copie la commande sans la redirection
        for (i = 0; i < redirect_index; i++) {
            cmd_argv[i] = argv[i];
        }
        cmd_argv[redirect_index] = NULL;
        if (cmd_argv[0] == NULL) {
            print_error();
            return;
        }
    } else {
        // Toujours couper à '<' s'il y en a une (sécurité)
        int cut = -1;
        for (i = 0; argv[i] != NULL && i < MAX_ARGS; i++) {
            if (strcmp(argv[i], "<") == 0) {
                cut = i;
                break;
            }
            cmd_argv[i] = argv[i];
        }
        if (cut != -1) {
            cmd_argv[cut] = NULL;
        } else {
            cmd_argv[i] = NULL;
        }
    }

    char full_path[512];
    int found = 0;

    for (int i = 0; i < path_count; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", search_paths[i], cmd_argv[0]);
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
        // Redirection si besoin
        if (redirect_index != -1) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                print_error();
                close(fd);
                exit(1);
            }
            close(fd);
        }
        execv(full_path, cmd_argv);
        print_error();
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }
}

// === Message d'erreur standard ===
void print_error(void) {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}