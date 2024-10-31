/*
  Projet minishell - Licence 3 Info - PSI 2024
 
  Nom :
  Prénom :
  Num. étudiant :
  Groupe de projet :
  Date :
 
  Parsing de la ligne de commandes utilisateur (implémentation).
 
 */

#include <stddef.h>  // pour size_t
#include <ctype.h>   // pour isspace
#include <string.h>  // pour strlen, strncpy
#include <stdlib.h>

// Fonction pour supprimer les espaces en début et fin de chaîne
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

// Fonction pour nettoyer une chaîne (à définir selon ton besoin exact)
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

// Fonction pour séparer une chaîne sur un séparateur spécifique
int separate_s(char* str, char* s, size_t max) {
    if (str == NULL || s == NULL) return -1;

    char* token;
    size_t count = 0;

    token = strtok(str, s);
    while (token != NULL && count < max) {
        token = strtok(NULL, s);
        count++;
    }

    return count;
}

// Fonction pour substituer les variables d'environnement dans une chaîne
int substenv(char* str, size_t max) {
    if (str == NULL) return -1;

    // Remplacer une variable d'environnement basique comme $HOME
    for (size_t i = 0; i < strlen(str); i++) {
        if (str[i] == '$') {
            char* env_var = getenv(&str[i + 1]);
            if (env_var != NULL) {
                strncpy(str, env_var, max);
            }
        }
    }

    return 0;
}

// Fonction pour découper une chaîne en tokens
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

// Fonction optionnelle pour découper une chaîne en tokens (version sécurisée)
int strcut_s(char* str, char sep, char** tokens, size_t max) {
    return strcut(str, sep, tokens, max);  // Utilisation de la même logique
}
