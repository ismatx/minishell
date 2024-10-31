#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"
#include "cmd.h"
#include "builtin.h"

#define MAX_LINE_SIZE 1024  // Taille maximale pour une ligne de commande

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

        // Traiter la ligne de commande
        //   - supprimer les espaces en début et en fin de ligne
        trim(cmdline);
        clean(cmdline);

        // Vérifier les commandes internes avant d'analyser les commandes générales
        if (strcmp(cmdline, "exit") == 0) {
            break; // Quitter la boucle
        } else if (!strcmp(cmdline, "uid")) {
            uid_t uid = getuid();
            gid_t gid = getgid();
            printf("UID: %d, GID: %d\n", uid, gid);
            continue;
        } else if (!strcmp(cmdline, "what is today?")) {
            printf("Today is your birthday, HAPPY 22ND BIRTHDAY!\n");
            continue;
        } else if (strcmp(cmdline, "export") == 0) {
            // Utiliser une fonction `export` pour définir ou modifier une variable d'environnement
            if (cmdtoks[1] != NULL && cmdtoks[2] != NULL) {
                exportvar(cmdtoks[1], cmdtoks[2], STDERR_FILENO);
            } else {
                fprintf(stderr, "Usage: export <variable> <value>\n");
            }
            continue;
        } else if (strcmp(cmdline, "unset") == 0) {
            // Vérifier que la variable à supprimer est fournie
            if (cmdtoks[1] != NULL) {
                unset(cmdtoks[1]);  // Appeler la fonction unset
            } else {
                fprintf(stderr, "Usage: unset <variable>\n");
            }
            continue;
        } else if (strcmp(cmdline, "printvar") == 0) {
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

        // Découper la ligne de commande en fonction des pipes
        char* pipe_tok = strtok(cmdline, "|");
        int cmd_index = 0;

        while (pipe_tok != NULL && cmd_index < MAX_CMD_SIZE) {
            // Traiter chaque commande séparée par un pipe
            trim(pipe_tok); // Nettoyer les espaces autour de la commande
            strcut(pipe_tok, ' ', cmdtoks, MAX_CMD_SIZE);

            // Traduire la ligne en structures cmd_t dans cmds
            num_cmds = parse_cmd(cmdtoks, &cmds[cmd_index], MAX_CMD_SIZE);
            if (num_cmds < 0) {
                fprintf(stderr, "Error parsing command line.\n");
                break;
            }

            cmd_index++;
            pipe_tok = strtok(NULL, "|"); // Passer à la commande suivante
        }

        // Exécuter les commandes avec pipes
        for (int i = 0; i < cmd_index; i++) {
            if (exec_cmd(&cmds[i]) < 0) {
                fprintf(stderr, "Error executing command.\n");
            }
        }
    }

    fprintf(stderr, "Good bye!\n");
    return 0;
}

