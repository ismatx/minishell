/*
Fichier parser.c : Gestion et exécution des commandes pour le shell
Auteur : MADANI YOUCEF ISMAT ELLAH , SAIDI SOUAD
Dépendances : cmd.h, builtin.h, parser.h
L3 INFORMATIQUE 
m22412177
*/

#include <stddef.h>  // pour size_t
#include <ctype.h>   // pour isspace
#include <string.h>  // pour strlen, strncpy
#include <stdlib.h>

/*
Fonction trim
Paramètre str : chaîne de caractères à traiter
Retourne un entier, 0 si l'opération réussit, -1 sinon
Supprime les espaces en début et fin de chaîne
*/
int trim(char* str) {
    if (str == NULL) return -1;

    char* start = str;
    char* end = str + strlen(str) - 1;

    // Supprimer les espaces au début
    while (isspace((unsigned char)*start)) start++;

    // Supprimer les espaces à la fin
    while (end > start && isspace((unsigned char)*end)) end--;

    // Calculer la nouvelle longueur de la chaîne
    size_t len = end - start + 1;

    // Copier la chaîne trimée au début de str
    memmove(str, start, len);
    str[len] = '\0';  // Ajouter le terminateur de fin

    return 0;
}

/*
Fonction clean
Paramètre str : chaîne de caractères à nettoyer
Retourne un entier, 0 si l'opération réussit, -1 sinon
Remplace les caractères non imprimables par des espaces
*/
int clean(char* str) {
    if (str == NULL) return -1;

    // Nettoyage simple: remplacement des caractères non imprimables par des espaces
    for (size_t i = 0; i < strlen(str); i++) {
        if (!isprint((unsigned char)str[i])) {
            str[i] = ' ';
        }
    }
    
    return 0;
}

/*
Fonction separate_s
Paramètres :
    str : chaîne de caractères à traiter
    s : chaîne contenant les séparateurs
    max : taille maximale de la chaîne traitée
Retourne un entier, 0 si l'opération réussit, -1 sinon
Ajoute des espaces avant et après chaque séparateur
*/
int separate_s(char* str, char* s, size_t max) {
   if (str == NULL || s == NULL) return -1;
    size_t len = strlen(str);
    size_t count = 0; // Compteur de caractères ajoutés
    char* result = (char*)malloc((max + 1) * sizeof(char)); // +1 pour le '\0'
    if (result == NULL) return -1; // Vérification de l'allocation
    char* ptr = result; // Pointeur pour construire la chaîne résultante
    // Parcourir chaque caractère de la chaîne d'entrée
    for (size_t i = 0; i < len && count < max; i++) {
        // Vérifier si le caractère courant est dans la chaîne s
        if (strchr(s, str[i]) != NULL) {
            // Ajouter un espace avant si ce n'est pas le début et le précédent n'est pas un espace
            // Special case: Handle the case for multi-character operators like '&&' or '||'
            if (i + 1 < len && str[i] == '&' && ((str[i + 1] == '&')|| (str[i + 1] == '>'))) {
                *ptr++ = str[i++];
                *ptr++ = str[i];
                count += 2;
                continue;
            } else if (i + 1 < len && str[i] == '|' && str[i + 1] == '|') {
                *ptr++ = str[i++];
                *ptr++ = str[i];
                count += 2;
                continue;
            }
            if (ptr != result && *(ptr - 1) != ' ') {
                *ptr++ = ' ';
                count++;
                if (count >= max) break; // Vérifier la capacité
            }
            // Ajouter le caractère spécial
            *ptr++ = str[i];
            count++;
            if (count >= max) break; // Vérifier la capacité
            // Ajouter un espace après si ce n'est pas la fin et le suivant n'est pas un espace
            if (i + 1 < len && str[i + 1] != ' ' && count < max) {
                *ptr++ = ' ';
                count++;
            }
        } else {
            // Ajouter le caractère courant
            *ptr++ = str[i];
            count++;
        }
        // Vérifier la capacité
        if (count >= max) break; 
    }
    // Terminer la chaîne résultante
    *ptr = '\0';
    // Copier le résultat dans la chaîne d'entrée
    strncpy(str, result, max); // S'assurer de ne pas dépasser max
    str[max] = '\0'; // S'assurer que str est bien terminée
    free(result); // Libérer la mémoire allouée
    return 0; // Opération réussie
}

/*
Fonction substenv
Paramètres :
    str : chaîne de caractères contenant des variables d'environnement à substituer
    max : taille maximale de la chaîne
Retourne un entier, 0 si l'opération réussit, -1 sinon
Substitue les variables d'environnement dans une chaîne
*/
int substenv(char* str, size_t max) {
    if (str == NULL) return -1;

    // Remplacer une variable d'environnement basique comme $HOME
    for (size_t i = 0; i < strlen(str); i++) { // home = $Home
        if (str[i] == '$') {
            char* env_var = getenv(&str[i + 1]);
            if (env_var != NULL) {
                strncpy(str, env_var, max);
            }
        }
    }

    return 0;
}

/*
Fonction strcut
Paramètres :
    str : chaîne de caractères à découper
    sep : caractère séparateur
    tokens : tableau pour stocker les tokens
    max : nombre maximum de tokens
Retourne un entier, nombre de tokens extraits, -1 en cas d'erreur
*/
int strcut(char* str, char sep, char** tokens, size_t max) {
    if (str == NULL || tokens == NULL) return -1;

    size_t count = 0;
    char* token;

    token = strtok(str, &sep);
    while (token != NULL && count < max) {
        tokens[count++] = token;
        token = strtok(NULL, &sep);
    }
    if (count < max) { tokens[count] = NULL;}

    return count;
}


