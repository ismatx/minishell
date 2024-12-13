/*
Fichier cmd.c : Gestion et exécution des commandes pour le shell
Auteur : MADANI YOUCEF ISMAT ELLAH , SAIDI SOUAD
Dépendances : cmd.h, builtin.h, parser.h
L3 INFORMATIQUE 
m22412177
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>  // pour les flags de redirection
#include <sys/wait.h>
#include "cmd.h"
#include "builtin.h"
#include "parser.h"

// Fonction pour initialiser une commande
int init_cmd(cmd_t* p) {
    if (p == NULL) return -1;    // Vérifie si le pointeur de commande est NULL

    // Initialise les champs de la structure cmd_t avec des valeurs par défaut
    p->pid = -1;         // PID initialisé à -1
    p->status = 0;      // Statut initialisé à 0    
   p->stdin = STDIN_FILENO;  // Entrée standard
    p->stdout = STDOUT_FILENO;// Sortie standard
    p->stderr = STDERR_FILENO;// Erreur standard
    p->wait = 1;              // Attente du processus enfant par défaut
    p->path = NULL;           // Chemin de commande initialisé à NULL
    // Initialise les tableaux argv et fdclose
    memset(p->argv, 0, sizeof(p->argv));
    memset(p->fdclose, -1, sizeof(p->fdclose));
     // Initialise les pointeurs vers les commandes suivantes (pipeline, success, failure)
    p->next = NULL;
    p->next_success = NULL;
    p->next_failure = NULL;

    return 0;
}

// Fonction pour exécuter une commande
int exec_cmd(cmd_t* p) {
     // Vérifie que la commande n'est pas NULL et que le premier argument n'est pas NULL
    if (p == NULL || p->argv[0] == NULL) {
        return -1;
    }

     // Vérifie si c'est une commande interne (builtin), comme "cd" ou "export"
    if (strcmp(p->argv[0], "cd") == 0) {
        return cd(p->argv[1], STDERR_FILENO);
    }
    else if (strcmp(p->argv[0], "export") == 0) {
        return exportvar(p->argv[1],p->argv[2], STDERR_FILENO);
    }

    // Création d'un nouveau processus
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {  // Processus enfant
        // Rediriger l'entrée et la sortie si nécessaire
        if (p->stdin != STDIN_FILENO) {
            dup2(p->stdin, STDIN_FILENO);
            close(p->stdin);
        }
        if (p->stdout != STDOUT_FILENO) {
            dup2(p->stdout, STDOUT_FILENO);
            if (p->stdout != p->stderr) {
            close(p->stdout);
            }
        }
        if (p->stderr != STDERR_FILENO) {
            dup2(p->stderr, STDERR_FILENO);
            if (p->stderr != p->stdout) {
                close(p->stderr);
            }
        }
        // Exécute la commande avec execvp
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
        // Vérifie les paramètres d'entrée
    if (tokens == NULL || cmds == NULL || max == 0) return -1;
    
    size_t cmd_count = 0;               // Compteur de commandes
    cmd_t* current_cmd = &cmds[cmd_count]; // Pointe sur la première commande
    init_cmd(current_cmd);             // Initialise la première commande
    int pipefds[2] = {-1, -1};         // Descripteurs de fichiers pour les pipes
    char* out_file = NULL;             // Fichier pour la redirection de sortie
    char* err_file = NULL;             // Fichier pour la redirection des erreurs
        // Parcourt les tokens pour analyser la commande
    for (size_t i = 0; tokens[i] != NULL; i++) {
                    // Gestion de la redirection de sortie
        if (strcmp(tokens[i], ">") == 0) {
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>'\n");
                return -1;
            }// Récupère le nom du fichier suivant '>' et l'ouvre en TRUNK
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (current_cmd->stdout == -1) {
                perror("open");
                return -1;
            }
        } else if (strcmp(tokens[i], ">>") == 0) {
             // Gestion de la redirection en mode ajout (append)
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: expected file after '>>'\n");
                return -1;
            }
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (current_cmd->stdout == -1) {
                perror("open");
                return -1;
            }
        } else if (strcmp(tokens[i], "2>") == 0) {  // Gestion de la redirection des erreurs
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: expected file after '2>'\n");
                return -1;
            }
            err_file = tokens[++i];
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (current_cmd->stderr == -1) {
                perror("open");
                return -1;
            }
        } else if (strcmp(tokens[i], "2>>") == 0) {  // Gestion de la redirection des erreurs en mode append
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: expected file after '2>>'\n");
                return -1;
            }
            err_file = tokens[++i];
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (current_cmd->stderr == -1) {
                perror("open");
                return -1;
            }}else if (strcmp(tokens[i], "<") == 0) { // Gestion de la redirection d'entrée
    if (tokens[i + 1] == NULL) {
        fprintf(stderr, "Syntax error: expected file after '<'\n");
        return -1;
    }
    char* in_file = tokens[++i];
    current_cmd->stdin = open(in_file, O_RDONLY);
    if (current_cmd->stdin == -1) {
        perror("open");
        return -1;
    }}
    else if (strcmp(tokens[i], ">&2") == 0) {
    current_cmd->stdout = STDERR_FILENO;
} else if (strcmp(tokens[i], "2>&1") == 0) {
    current_cmd->stderr = STDOUT_FILENO;
}else if (strcmp(tokens[i], "&>") == 0) {
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "Syntax error: expected file after '&>'\n");
                return -1;
            }
            out_file = tokens[++i];
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (current_cmd->stdout == -1) {
                perror("open");
                return -1;
            }
            // Redirect stderr to the same file as stdout
            current_cmd->stderr = current_cmd->stdout;
        } else if (strcmp(tokens[i], "||") == 0) {// passe a la prochaine commande si échoue
            if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;
            }

            // Si la commande réussit, ignorer les commandes jusqu'à la prochaine "&&" ou "||"
            if (WEXITSTATUS(current_cmd->status) == 0) {
                return 0;
            }
            if (cmd_count >= max) return -1;
            current_cmd->next_failure = &cmds[cmd_count];
            current_cmd = current_cmd->next_failure;
            init_cmd(current_cmd);
        } else if (strcmp(tokens[i], "|") == 0) {// gesrion des pipes
            if (pipe(pipefds) == -1) {
                perror("Error creating pipe");
                return -1;
            }
            current_cmd->stdout = pipefds[1];
            current_cmd->next_success = &cmds[++cmd_count];
            current_cmd = current_cmd->next_success;
            init_cmd(current_cmd);
            current_cmd->stdin = pipefds[0];
        } else if (strcmp(tokens[i], "&&") == 0) {
    if (exec_cmd(current_cmd) == -1) {
        fprintf(stderr, "Error executing command before &&\n");
        return -1;
    }
    if (WEXITSTATUS(current_cmd->status) == 0) {  // Passe à la prochaine si succès
        current_cmd->next_success = &cmds[++cmd_count];
        current_cmd = current_cmd->next_success;
        init_cmd(current_cmd);
    } else {  // Sinon, ignore la commande suivante
        break;
    }
} else if (strcmp(tokens[i], "&") == 0) {
            current_cmd->wait = 0;
            if (cmd_count >= max) return -1;
            current_cmd = &cmds[++cmd_count];
            init_cmd(current_cmd);
        }else if (strcmp(tokens[i], ";") == 0) {// !!!
            // Execute the current command
            /*if (exec_cmd(current_cmd) == -1) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande.\n");
                return -1;  // Return an error if execution fails
            }*/
            if (cmd_count >= max) return -1;
            current_cmd->next = &cmds[++cmd_count];
            current_cmd = current_cmd->next;
            init_cmd(current_cmd);
        } else {
            size_t j = 0;
            while (current_cmd->argv[j] != NULL) {
                j++;
            }
            current_cmd->argv[j] = tokens[i];
            current_cmd->argv[j + 1] = NULL;
        }
    }

    return cmd_count + 1;
}