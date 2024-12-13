/*
Fichier main.c : fichier principal du main
Auteur : MADANI YOUCEF ISMAT ELLAH , SAIDI SOUAD
Dépendances : cmd.h, builtin.h, parser.h
L3 INFORMATIQUE 
m22412177
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"
#include "cmd.h"
#include "builtin.h"
#include <sys/types.h>
//#include <readline/readline.h> libraries a ajouter pour pouvoir lire historique avec les fleches mais bon trop compliqué faut telecharger la biblio et tout
//#include <readline/history.h>
#define MAX_LINE_SIZE 1024  // Taille maximale pour une ligne de commande
#define MAX_HISTORY_SIZE 100  // Nombre maximal de commandes dans l'historique

// Structure pour stocker l'historique
char* history[MAX_HISTORY_SIZE];
int history_count = 0;
// Fonction pour ajouter une commande à l'historique
void add_to_history(const char* cmdline) {
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count++] = strdup(cmdline);
    } else {
        // Si l'historique est plein, supprimer la plus ancienne commande
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY_SIZE - 1) * sizeof(char*));
        history[MAX_HISTORY_SIZE - 1] = strdup(cmdline);
    }
}

// Fonction pour afficher l'historique
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

// Libérer la mémoire allouée pour l'historique
void free_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
}
void execute_all_cmds(cmd_t* cmds) {
    cmd_t* current_cmd = cmds;
    while (current_cmd != NULL) {
        int result = exec_cmd(current_cmd);
        if (current_cmd->stdin != STDIN_FILENO) {
            close(current_cmd->stdin);
        }
        if (current_cmd->stdout != STDOUT_FILENO) {
            close(current_cmd->stdout);
        }
         if (current_cmd->stderr != STDERR_FILENO) {
            close(current_cmd->stderr);
        }
        if (result == 0 && current_cmd->next_success != NULL) {
            current_cmd = current_cmd->next_success;
        } else if (result != 0 && current_cmd->next_failure != NULL) {
            current_cmd = current_cmd->next_failure;
        } else {
            current_cmd = current_cmd->next;
        }
        
    }
}
int main(int argc, char* argv[]) {
    char cmdline[MAX_LINE_SIZE];  // buffer pour la ligne de commande
    char* cmdtoks[MAX_CMD_SIZE];  // "mots" de la ligne de commande
    cmd_t cmds[MAX_CMD_SIZE];     // tableau de commandes
    int num_cmds;                 // nombre de commandes analysées

    while (1) {
        // Effacer les contenus de cmdline, cmdtoks et cmds
        memset(cmdline, 0, sizeof(cmdline));
        memset(cmdtoks, 0, sizeof(cmdtoks));
        for (int i = 0; i < MAX_CMD_SIZE; i++) {
            init_cmd(&cmds[i]);  // Réinitialiser les commandes
        }

        // Afficher un prompt
        printf("MINISHELLISMAT> ");
        
        // Lire une ligne dans cmdline - fgets enregistre le \n final
        if (fgets(cmdline, MAX_LINE_SIZE, stdin) == NULL) break;  // EOF (Ctrl+D)
        cmdline[strlen(cmdline) - 1] = '\0';  // Retirer le \n final
        if (strlen(cmdline) > 0) {
            add_to_history(cmdline);
        }
        // Traiter la ligne de commande
        separate_s(cmdline, ";|&!", MAX_LINE_SIZE);
        trim(cmdline);
        clean(cmdline);
        // Découper la ligne en tokens dans cmdtoks
        int tok_count = strcut(cmdline, ' ', cmdtoks, MAX_CMD_SIZE);
        // Appel de parse_cmd pour analyser les tokens et remplir cmds
        num_cmds = parse_cmd(cmdtoks, cmds, MAX_CMD_SIZE);
        if (num_cmds < 0) {
            fprintf(stderr, "Erreur lors de l'analyse de la commande.\n");
            continue;
        }
        if (tok_count > 0 && strcmp(cmdtoks[0], "history") == 0) {
            show_history();
            continue;}
        // Vérifier les commandes internes avant d'exécuter les commandes générales
        else if (tok_count > 0 && strcmp(cmdtoks[0], "echo") == 0) {
                if (cmdtoks[1][0] == '$') {
                    substenv(cmdtoks[1], MAX_CMD_SIZE);
                    printf("%s", cmdtoks[1]);
                     printf("\n");
                    continue;
                }
        }
        else if (tok_count > 0 && strcmp(cmdtoks[0], "exit") == 0) {
            break; // Quitter la boucle
        } else if (tok_count > 0 && strcmp(cmdtoks[0], "uid") == 0) {
            uid_t uid = getuid();
            gid_t gid = getgid();
            printf("UID: %d, GID: %d\n", uid, gid);
            continue;
        } else if (tok_count > 1 && strcmp(cmdtoks[0], "export") == 0) {
            if (cmdtoks[1] != NULL && cmdtoks[2] != NULL) {
                exportvar(cmdtoks[1], cmdtoks[2], STDERR_FILENO);
            } else {
                fprintf(stderr, "Usage: export <variable> <value>\n");
            }
            continue;
        } else if (tok_count > 1 && strcmp(cmdtoks[0], "unset") == 0) {
            if (cmdtoks[1] != NULL) {
                unset(cmdtoks[1]);
            } else {
                fprintf(stderr, "Usage: unset <variable>\n");
            }
            continue;
        } else if (tok_count > 1 && strcmp(cmdtoks[0], "printvar") == 0) {
            if (cmdtoks[1] != NULL) {
                char* value = getenv(cmdtoks[1]);
                if (value != NULL) {
                    printf("%s=%s\n", cmdtoks[1], value);
                } else {
                    printf("%s: variable not found\n", cmdtoks[1]);
                }
            } else {
                fprintf(stderr, "Usage: printvar <variable>\n");
            }
            continue;
        }
       execute_all_cmds(cmds);
    }

    fprintf(stderr, "Good bye!\n");
    return 0;
}
