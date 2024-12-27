#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Définition des structures et constantes
#define MAXLEN 1000
#define MAXFIELDS 15

typedef char* t_field;
typedef t_field t_key;

typedef struct {
    t_field* fields;  // Tableau dynamique de champs
    int nbFields;     // Nombre de champs (y compris la clé)
} t_value;

typedef struct {
    t_key key;
    t_value value;
} t_tuple;

typedef struct node {
    t_tuple data;
    struct node* pNext;
} t_node;

typedef t_node* t_list;

typedef struct {
    char* hashFunction;  // Nom de la fonction de hachage
    int nbSlots;         // Nombre d'alvéoles
    t_list* slots;       // Tableau de listes chaînées
} t_hashtable;

// Fonctions utilitaires pour t_field, t_value et t_tuple
t_field create_field(const char* valeur) {
    t_field field = (t_field)malloc((strlen(valeur) + 1) * sizeof(char));
    assert(field != NULL);
    strcpy(field, valeur);
    return field;
}

void free_field(t_field field) {
    free(field);
}

t_value create_value(int nbFields, char* fields[]) {
    t_value value;
    value.nbFields = nbFields;
    value.fields = (t_field*)malloc(nbFields * sizeof(t_field));
    assert(value.fields != NULL);
    for (int i = 0; i < nbFields; i++) {
        value.fields[i] = create_field(fields[i]);
    }
    return value;
}

void free_value(t_value value) {
    for (int i = 0; i < value.nbFields; i++) {
        free_field(value.fields[i]);
    }
    free(value.fields);
}

// Fonction pour nettoyer une clé
void clean_key(char* key) {
    char* p = key;
    while (*p != '\0') {
        if (*p == '\n' || *p == '\r') {
            *p = '\0';  // Remplacer les caractères invisibles par le caractère nul
            break;
        }
        p++;
    }
}

// Fonctions pour la table de hachage
t_hashtable* create_hashtable(int nbSlots, const char* hashFunctionName) {
    t_hashtable* table = (t_hashtable*)malloc(sizeof(t_hashtable));
    assert(table != NULL);
    table->nbSlots = nbSlots;
    table->slots = (t_list*)calloc(nbSlots, sizeof(t_list));
    assert(table->slots != NULL);
    table->hashFunction = strdup(hashFunctionName);
    return table;
}

void free_hashtable(t_hashtable* table) {
    for (int i = 0; i < table->nbSlots; i++) {
        t_node* current = table->slots[i];
        while (current != NULL) {
            t_node* temp = current;
            free_field(temp->data.key);
            free_value(temp->data.value);
            current = current->pNext;
            free(temp);
        }
    }
    free(table->slots);
    free(table->hashFunction);
    free(table);
}

// Fonctions de hachage
int hash_function_sum(const char* key, int nbSlots) {
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash += key[i];
    }
    int index = hash % nbSlots;
    printf("Clé '%s' hachée à l'indice %d\n", key, index);
    return index;
}

// Fonction pour insérer un tuple dans la table de hachage
void insert_hashtable(t_hashtable* table, t_tuple tuple, int (*hash_function)(const char*, int)) {
    int index = hash_function(tuple.key, table->nbSlots);
    printf("Insertion de la clé '%s' à l'indice %d\n", tuple.key, index);

    // Créer un nouveau nœud
    t_node* new_node = (t_node*)malloc(sizeof(t_node));
    assert(new_node != NULL);
    new_node->data = tuple;
    new_node->pNext = table->slots[index];

    // Insérer en tête de liste
    table->slots[index] = new_node;

    // Afficher le contenu de l'alvéole
    printf("Contenu de l'alvéole %d après insertion :\n", index);
    t_node* current = table->slots[index];
    while (current != NULL) {
        printf("  - Clé : '%s'\n", current->data.key);
        current = current->pNext;
    }
}

// Fonction pour rechercher un tuple par clé
t_value* search_hashtable(t_hashtable* table, const char* key, int (*hash_function)(const char*, int), int* comparisons) {
    int index = hash_function(key, table->nbSlots);
    printf("Recherche de la clé '%s' à l'indice %d\n", key, index);

    t_node* current = table->slots[index];
    *comparisons = 0;

    while (current != NULL) {
        (*comparisons)++;
        printf("Comparaison avec la clé : '%s'\n", current->data.key);
        if (strcmp(current->data.key, key) == 0) {
            printf("Clé trouvée : %s\n", current->data.key);
            return &current->data.value;
        }
        current = current->pNext;
    }
    printf("Clé non trouvée.\n");
    return NULL;
}


// Fonction pour analyser la répartition des données dans les alvéoles
void analyze_hashtable(t_hashtable* table) {
    printf("\nRépartition des données dans les alvéoles :\n");
    for (int i = 0; i < table->nbSlots; i++) {
        int count = 0;
        t_node* current = table->slots[i];
        while (current != NULL) {
            count++;
            current = current->pNext;
        }
        printf("Alvéole %d : %d éléments\n", i, count);
    }
}

// Fonction pour lire un fichier et remplir la table
void read_file_to_hashtable(const char* filename, t_hashtable* table, int (*hash_function)(const char*, int)) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char line[MAXLEN];
    char* fields[MAXFIELDS];
    int nbFields = 0;
    char separateur = '\0';

    // Trouver le séparateur
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) == 2 && line[1] == '\n') { 
            separateur = line[0];  // Le premier caractère est le séparateur
            printf("Séparateur trouvé : '%c'\n", separateur);
            break;
        }
    }
    if (separateur == '\0') {
        fprintf(stderr, "Erreur : Aucun séparateur valide trouvé dans le fichier.\n");
        exit(EXIT_FAILURE);
    }

    // Lire le nombre de champs
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || strlen(line) <= 1) continue; // Ignorer commentaires et lignes vides
        if (sscanf(line, "%d", &nbFields) == 1) {
            printf("Nombre de champs trouvé : %d\n", nbFields);
            break; 
        }
    }

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || strlen(line) <= 1) continue; // Ignorer commentaires et lignes vides

        // Découper la ligne en champs
        int fieldCount = 0;
        char* token = strtok(line, &separateur);
        while (token != NULL && fieldCount < MAXFIELDS) {
            fields[fieldCount++] = strdup(token);
            token = strtok(NULL, &separateur);
        }

        // Nettoyer la clé
        clean_key(fields[0]);
        printf("Clé lue : '%s'\n", fields[0]); // Débogage : afficher la clé lue

        // Créer le tuple et l'insérer dans la table
        t_value value = create_value(fieldCount - 1, fields + 1);
        t_tuple tuple = {create_field(fields[0]), value};
        insert_hashtable(table, tuple, hash_function);

        // Libérer les champs alloués temporairement
        for (int i = 0; i < fieldCount; i++) {
            free(fields[i]);
        }
    }
    fclose(file);
}

// Programme principal
int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <fichier> <taille_table>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Paramètres
    const char* filename = argv[1];
    int nbSlots = atoi(argv[2]);

    // Créer la table de hachage
    t_hashtable* table = create_hashtable(nbSlots, "sum");

    // Remplir la table depuis le fichier
    read_file_to_hashtable(filename, table, hash_function_sum);

    // Afficher l'état initial de la table
    analyze_hashtable(table);

    // Recherche interactive
    char key[MAXLEN];
    printf("\nSaisir les clés à rechercher (Ctrl+D pour quitter) :\n");
    while (fgets(key, sizeof(key), stdin)) {
        clean_key(key);
        int comparisons = 0;
        t_value* result = search_hashtable(table, key, hash_function_sum, &comparisons);
        if (result) {
            printf("Clé trouvée : %s (comparaisons : %d)\n", key, comparisons);
            for (int i = 0; i < result->nbFields; i++) {
                printf("  Champ %d : %s\n", i + 1, result->fields[i]);
            }
        } else {
            printf("Clé non trouvée (comparaisons : %d).\n", comparisons);
        }
    }

    // Libérer la mémoire
    free_hashtable(table);

    return EXIT_SUCCESS;
}
