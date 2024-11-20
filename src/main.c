#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"
#include "cmd.h"
#include "builtin.h"
#include <sys/types.h>

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
        // Vérifier les commandes internes avant d'exécuter les commandes générales
        if (tok_count > 0 && strcmp(cmdtoks[0], "exit") == 0) {
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
        // Exécuter les commandes avec pipes
        for (int i = 0; i < num_cmds; i++) {
            if (exec_cmd(&cmds[i]) < 0) {
                fprintf(stderr, "Erreur lors de l'exécution de la commande %s.\n", *(cmdtoks + i));
            }
        }
    }
    fprintf(stderr, "Good bye!\n");
    return 0;
}