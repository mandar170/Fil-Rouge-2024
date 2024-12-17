#include <stdio.h> 
#include <assert.h>
#include <stdlib.h>
#include <string.h>

//CHAMP
#define MAXLEN 1000 

//typedef char t_field[1000]; moins efficace qu'avec un malloc 

//BONUS 1 
typedef char* t_field;  //Pointeur vers une chaîne dynamique

//Initialiser un champ avec malloc
t_field create_field(const char* valeur) {
    size_t len = strlen(valeur);
    if (len > MAXLEN) {
        fprintf(stderr, "Erreur : la taille du champ dépasse la limite de %d caractères.\n", MAXLEN);
        return NULL;
    }
    t_field field = (t_field)malloc((len + 1) * sizeof(char)); // Allocation dynamique, len+1 pour le caractère '\0'
    if (field == NULL) {
        printf("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    strcpy(field, valeur); // Copier la chaîne dans le champ alloué
    return field;
}

//Libérer la mémoire
void free_field(t_field field) {
    free(field); 
}

//CLE
//BONUS 2 
#define MAXFIELDS 15
typedef t_field t_key;

// typedef t_field t_value[MAXFIELDS]; moins bien qu'avec l'allocation dynamique 

typedef t_field* t_value;  // Une valeur est un pointeur vers un tableau de champs

// Fonction pour créer une valeur (tableau dynamique de champs)
t_value create_value(size_t field_count, const char* fields[]) {
    if (field_count == 0) return NULL;
    // Allouer de la mémoire pour les pointeurs vers les champs
    t_value value = (t_value)malloc(field_count * sizeof(t_field));
    if (value == NULL) {
        printf("Erreur d'allocation mémoire pour la valeur");
        exit(EXIT_FAILURE);
    }
    // Allouer chaque champ individuellement
    for (size_t i = 0; i < field_count; i++) {
        value[i] = create_field(fields[i]);
    }
    return value;
}

// Fonction pour libérer une valeur (tableau de champs)
void free_value(t_value value, size_t field_count) {
    for (size_t i = 0; i < field_count; i++) {
        free_field(value[i]);  // Libérer chaque champ
    }
    free(value);  // Libérer le tableau de pointeurs
}

//TUPLE
typedef struct {
  t_key key;
  t_value value;
} t_tuple;

//METADONNEES
/* Structure sans malloc
typedef struct {
  char sep;
  int nbFields;
  t_field fieldNames[MAXFIELDS];
} t_metadata;
*/

//BONUS 3 
typedef struct {
    char sep;        // Caractère séparateur
    int nbFields;    // Nombre de champs
    t_field* fieldNames;  // Pointeur vers un tableau dynamique de champs
} t_metadata;

// Fonction pour initialiser les métadonnées avec allocation dynamique
t_metadata* create_metadata(char sep, int nbFields, const char* fieldNames[]) {
    if (nbFields <= 0) return NULL;
    t_metadata* metadata = (t_metadata*)malloc(sizeof(t_metadata));
    if (metadata == NULL) {
        printf("Erreur d'allocation mémoire pour les métadonnées");
        exit(EXIT_FAILURE);
    }
    metadata->sep = sep;
    metadata->nbFields = nbFields;
    // Allouer dynamiquement le tableau fieldNames
    metadata->fieldNames = (t_field*)malloc(nbFields * sizeof(t_field));
    if (metadata->fieldNames == NULL) {
        printf("Erreur d'allocation mémoire pour fieldNames");
        free(metadata);
        exit(EXIT_FAILURE);
    }
    // Allouer et copier chaque champ du tableau fieldNames
    for (int i = 0; i < nbFields; i++) {
        metadata->fieldNames[i] = create_field(fieldNames[i]);
    }

    return metadata;
}

// Fonction pour libérer les métadonnées
void free_metadata(t_metadata* metadata) {
    if (metadata == NULL) return;
    for (int i = 0; i < metadata->nbFields; i++) {
        free_field(metadata->fieldNames[i]);  // Libérer chaque champ
    }
    free(metadata->fieldNames);  // Libérer le tableau de pointeurs
    free(metadata);  // Libérer la structure des métadonnées
}

