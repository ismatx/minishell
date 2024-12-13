/*
Fichier builtin.c : tout le builtin ici
Auteur : MADANI YOUCEF ISMAT ELLAH , SAIDI SOUAD
Dépendances : cmd.h, builtin.h, parser.h
L3 INFORMATIQUE 
m22412177
*/

#include <unistd.h>  // pour chdir
#include <stdlib.h>  // pour exit, setenv
#include <stdio.h>   // pour fprintf
#include <string.h>  // pour strcmp
#include "cmd.h"     // inclusion de cmd.h pour cmd_t
#include "builtin.h"
#include "parser.h"

/*
Fonction is_builtin
Paramètre cmd : chaîne de caractères représentant une commande
Retourne un entier, 1 si la commande est interne, 0 sinon
*/
int is_builtin(const char* cmd) {
    // Les commandes internes que l'on gère
    if (strcmp(cmd, "cd") == 0 || 
        strcmp(cmd, "export") == 0 || 
        strcmp(cmd, "exit") == 0 || 
        strcmp(cmd, "unset") == 0) {
        return 1;
    }
    return 0;
}

/*
Fonction builtin
Paramètre cmd : pointeur vers la structure cmd_t représentant une commande
Retourne un entier, 0 si la commande est exécutée avec succès, -1 sinon
*/
int builtin(cmd_t* cmd) {
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return -1; // Commande non valide
    }

    // Vérifie quelle commande interne est demandée
    if (strcmp(cmd->argv[0], "cd") == 0) {
        return cd(cmd->argv[1], STDERR_FILENO);  // Passe le chemin et descripteur d'erreur
    } else if (strcmp(cmd->argv[0], "export") == 0) {
    if (cmd->argv[1] == NULL || cmd ->argv[2] == NULL ){
    fprintf(stderr, "export: usage: EXPORT VAR VALUE\n");
    return -1;}
        return exportvar(cmd->argv[1], cmd->argv[2], STDERR_FILENO); // var et value
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        int ret = (cmd->argv[1] != NULL) ? atoi(cmd->argv[1]) : 0; // Si un code de sortie est passé
        return exit_shell(ret, STDERR_FILENO);
    } else if (strcmp(cmd->argv[0], "unset") == 0) { unset(cmd->argv[1]);}
    if (!is_builtin(cmd->argv[0]) && access(cmd->argv[0], X_OK) != 0) {
    fprintf(stderr, "minishell: command not found: %s\n", cmd->argv[0]);
    return -1;
}
    return -1; // Si la commande n'est pas reconnue
}

/*
Fonction cd
Paramètres :
    path : chaîne de caractères représentant le chemin du répertoire cible
    fderr : descripteur de sortie pour les messages d'erreur
Retourne un entier, 0 si le changement de répertoire réussit, -1 sinon
*/
int cd(const char* path, int fderr) {
    if (path == NULL) {
    fprintf(stderr, "cd : missing argument\n");
    return -1;
    }
    
    if (chdir(path) != 0) {
        // Affiche une erreur sur le descripteur de la sortie d'erreur
        perror("cd");
        return -1;
    }
    return 0; // Succès
}

/*
Fonction exportvar
Paramètres :
    var : chaîne de caractères représentant le nom de la variable
    value : chaîne de caractères représentant la valeur de la variable
    fderr : descripteur de sortie pour les messages d'erreur
Retourne un entier, 0 si la modification ou création de variable réussit, -1 sinon
*/
int exportvar(const char* var, const char* value, int fderr) {
    // Si la commande est 'export', on ajoute/modifie une variable d'environnement
   if (var == NULL || value == NULL) {
        dprintf(fderr, "export: usage: export VAR VALUE\n");
        return -1;
    }

    if (setenv(var, value, 1) != 0) {
        dprintf(fderr, "export: failed to set variable: %s\n", var);
        return -1;
    }

    return 0;  // Succès
}
/*
Fonction unset
Paramètre variable : chaîne de caractères représentant le nom de la variable à supprimer
Ne retourne rien
*/
void unset(const char *variable) {
    if (variable == NULL) {
        fprintf(stderr, "Usage: unset <variable>\n");
        return;
    }
    
    // Utiliser unsetenv pour supprimer la variable d'environnement
    if (unsetenv(variable) != 0) {
        perror("unsetenv");
    }
}
/*
Fonction exit_shell
Paramètres :
    ret : entier représentant le code de sortie
    fderr : descripteur de sortie pour les messages d'erreur
Retourne un entier, bien que cette fonction quitte le programme
*/
int exit_shell(int ret, int fderr) {
    // Ici, on peut éventuellement afficher un message avant de quitter
    fprintf(stderr, "Exiting shell with status %d\n", ret);
    exit(ret); // Quitte avec le code de retour passé en paramètre
    return 0; // Cette ligne ne sera jamais atteinte, mais c'est pour respecter la signature
}
