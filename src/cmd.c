#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "cmd.h"
#include "builtin.h"
#include "parser.h"

// Fonction pour initialiser une commande
int init_cmd(cmd_t* p) {
    if (p == NULL) return -1;

    p->pid = -1;
    p->status = 0;
    p->stdin = STDIN_FILENO;
    p->stdout = STDOUT_FILENO;
    p->stderr = STDERR_FILENO;
    p->wait = 1;
    p->path = NULL;
    memset(p->argv, 0, sizeof(p->argv));
    p->next = NULL;

    return 0;
}

// Fonction pour exécuter une commande avec redirections
int exec_cmd(cmd_t* p) {
    if (p == NULL || p->argv[0] == NULL) return -1;

    // Commandes internes (ex: cd, export)
    if (strcmp(p->argv[0], "cd") == 0) {
        return cd(p->argv[1], STDERR_FILENO);
    } else if (strcmp(p->argv[0], "export") == 0) {
        return exportvar(p->argv[1], p->argv[2], STDERR_FILENO);
    }

    // Création d'un processus enfant pour exécuter la commande
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {  // Processus enfant
        // Gérer les redirections
        if (p->stdin != STDIN_FILENO) {
            dup2(p->stdin, STDIN_FILENO);
            close(p->stdin);
        }
        if (p->stdout != STDOUT_FILENO) {
            dup2(p->stdout, STDOUT_FILENO);
            close(p->stdout);
        }
        if (p->stderr != STDERR_FILENO) {
            dup2(p->stderr, STDERR_FILENO);
            close(p->stderr);
        }

        execvp(p->argv[0], p->argv);
        perror("execvp");  // Si exec échoue
        exit(EXIT_FAILURE);
    }

    // Processus parent
    p->pid = pid;
    if (p->wait) {
        if (waitpid(pid, &p->status, 0) < 0) {
            perror("waitpid");
            return -1;
        }
    }

    return 0;
}

// Fonction pour analyser les tokens et gérer les redirections
int parse_cmd(char* tokens[], cmd_t* cmds, size_t max) {
    if (tokens == NULL || cmds == NULL || max == 0) return -1;
    size_t cmd_count = 0;
    cmd_t* current_cmd = &cmds[cmd_count];
    init_cmd(current_cmd);
    int pipefds[2] = {-1, -1}; // Pipe file descriptors
    char* out_file = NULL;
    char* err_file = NULL;

    for (size_t i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            // Output redirection to a file
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else if (strcmp(tokens[i], ">>") == 0) {
            // Output redirection to a file in append mode
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else if (strcmp(tokens[i], "2>") == 0) {
            // Output redirection for stderr
            err_file = tokens[++i];
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else if (strcmp(tokens[i], "2>>") == 0) {
            // Output redirection for stderr in append mode
            err_file = tokens[++i];
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else if (strcmp(tokens[i], "&>") == 0) {
            // Redirect both stdout and stderr to the same file
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            current_cmd->stderr = current_cmd->stdout;
        } else if (strcmp(tokens[i], "|") == 0) {
            // Gestion des pipes
            if (pipe(pipefds) == -1) {
                perror("pipe");
                return -1;
            }
            current_cmd->stdout = pipefds[1];  // Redirige stdout vers l'écriture du pipe

            // Exécute la commande actuelle
            if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;
            }

            close(pipefds[1]);  // Ferme l'écriture du pipe dans le parent
            current_cmd->stdout = STDOUT_FILENO;  // Réinitialise stdout

            // Prépare la commande suivante
            cmd_count++;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);
            current_cmd->stdin = pipefds[0];  // Redirige stdin vers la lecture du pipe
        } 
          else if (strcmp(tokens[i], "&") == 0) {
            // Handle background execution
            current_cmd->wait = 0;  // Mark the current command as non-blocking
            pid_t pid = fork();
            if (pid == 0) {
                // Execute the command in the background
                execvp(current_cmd->argv[0], current_cmd->argv);
                perror("execvp failed");
                exit(EXIT_FAILURE);
            
            // Initialize the next command
            cmd_count++;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);}
        } else if (strcmp(tokens[i], ";") == 0) {
            // Exécute la commande actuelle
            if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;
            }

            // Prépare la commande suivante
            cmd_count++;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);
        } else if (strcmp(tokens[i], "&&") == 0) {
            // Exécute la commande actuelle
            if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;
            }

            // Si la commande échoue, ignorer les commandes jusqu'à la prochaine "&&" ou "||"
            if (WEXITSTATUS(current_cmd->status) != 0) {
                while (tokens[i + 1] != NULL && strcmp(tokens[i + 1], "&&") != 0 && strcmp(tokens[i + 1], "||") != 0) {
                    i++;
                }
            }

            cmd_count++;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);
        } else if (strcmp(tokens[i], "||") == 0) {
            // Exécute la commande actuelle
            if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;
            }

            // Si la commande réussit, ignorer les commandes jusqu'à la prochaine "&&" ou "||"
            if (WEXITSTATUS(current_cmd->status) == 0) {
                while (tokens[i + 1] != NULL && strcmp(tokens[i + 1], "&&") != 0 && strcmp(tokens[i + 1], "||") != 0) {
                    i++;
                }
            }

            cmd_count++;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);
        }else {
            // Ajouter un argument à la commande actuelle
            size_t arg_index = 0;
            while (current_cmd->argv[arg_index] != NULL) arg_index++;
            current_cmd->argv[arg_index] = tokens[i];
            current_cmd->argv[arg_index + 1] = NULL;  // Terminer par NULL
        }
    }

    // Exécute la dernière commande
    if (exec_cmd(current_cmd) == -1) {
        fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
        return -1;
    }

    return cmd_count + 1;  // Retourne le nombre de commandes traitées
}