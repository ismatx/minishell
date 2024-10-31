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
    if (p == NULL) return -1;

    p->pid = -1;
    p->status = 0;
    p->stdin = STDIN_FILENO;
    p->stdout = STDOUT_FILENO;
    p->stderr = STDERR_FILENO;
    p->wait = 1;  // Attendre par défaut
    p->path = NULL;
    memset(p->argv, 0, sizeof(p->argv));
    memset(p->fdclose, -1, sizeof(p->fdclose));
    p->next = NULL;
    p->next_success = NULL;
    p->next_failure = NULL;

    return 0;
}

// Fonction pour exécuter une commande
int exec_cmd(cmd_t* p) {
    if (p == NULL || p->argv[0] == NULL) {
        return -1;
    }

    // Vérifier si c'est une commande interne (ex: cd)
    if (strcmp(p->argv[0], "cd") == 0) {
        return cd(p->argv[1], STDERR_FILENO);
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

    char* out_file = NULL;
    char* err_file = NULL;

    for (size_t i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            out_file = tokens[++i];  // Fichier de redirection stdout
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else if (strcmp(tokens[i], ">>") == 0) {
            out_file = tokens[++i];  // Fichier de redirection stdout en mode ajout
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else if (strcmp(tokens[i], "2>") == 0) {
            err_file = tokens[++i];  // Fichier de redirection stderr
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        } else if (strcmp(tokens[i], "2>>") == 0) {
            err_file = tokens[++i];  // Fichier de redirection stderr en mode ajout
            current_cmd->stderr = open(err_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else if (strcmp(tokens[i], "&>") == 0) {
            out_file = tokens[++i];  // Redirige stdout et stderr vers le même fichier
            current_cmd->stdout = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            current_cmd->stderr = current_cmd->stdout;  // Stderr redirigé vers stdout
        } else if (strcmp(tokens[i], "|") == 0) {
            // Sauvegarder la commande actuelle
            current_cmd->argv[i] = NULL;  // Terminer l'argv actuel

            // Passer à la commande suivante
            cmd_count++;
            if (cmd_count >= max) return -1; // Vérifier le nombre maximum de commandes
            current_cmd = &cmds[cmd_count];
            init_cmd(current_cmd);  // Initialiser la nouvelle commande
        } else {
            current_cmd->argv[i] = tokens[i];  // Ajouter à la commande
        }

        if (current_cmd->stdout < 0 || current_cmd->stderr < 0) {
            perror("open");
            return -1;
        }
    }

    return cmd_count + 1; // Retourne le nombre de commandes analysées
}
