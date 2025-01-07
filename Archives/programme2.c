#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Définition des structures
typedef struct {
    char* key;       // Clé dynamique
    char** value;    // Tableau dynamique de champs
} t_tuple;

typedef struct node {
    t_tuple data;
    struct node* next;
} t_node;

typedef struct {
    char sep;        // Séparateur
    int nbFields;    // Nombre de champs
    char** fieldNames; // Tableau dynamique pour les noms de champs
} t_metadata;

typedef struct {
    t_node** slots;  // Table de hachage (tableau de listes chaînées)
    int nbSlots;     // Nombre d'alvéoles
    int nbTuples;    // Nombre total de tuples insérés
} t_hashtable;

// Prototypes
char* readLine(FILE* file);
char* allocateField(const char* source);
t_hashtable* parseFileHash(const char* filename, t_metadata* metadata, int nbSlots);
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata);
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots);
unsigned int hashFunction(const char* key, int nbSlots);
void freeHashTable(t_hashtable* table, t_metadata* metadata);

// Fonction principale
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <filename> <nbSlots>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];
    int nbSlots = atoi(argv[2]);
    if (nbSlots <= 0) {
        fprintf(stderr, "Erreur : nbSlots doit être supérieur à 0.\n");
        exit(EXIT_FAILURE);
    }

    t_metadata metadata;
    t_hashtable* table = parseFileHash(filename, &metadata, nbSlots);

    printf("%d mots indexés dans %d alvéoles\n", table->nbTuples, nbSlots);
    printf("Saisir les mots recherchés :\n");

    char key[1000];
    while (fgets(key, sizeof(key), stdin)) {
        size_t len = strlen(key);
        if (key[len - 1] == '\n') key[len - 1] = '\0';
        if (strlen(key) == 0) break;
        searchKeyHash(table, &metadata, key, nbSlots);
    }

    freeHashTable(table, &metadata);
    return 0;
}

// Lecture d'une ligne
char* readLine(FILE* file) {
    char buffer[1000];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return NULL;
    }
    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    char* line = malloc(len + 1);
    strcpy(line, buffer);
    return line;
}

// Allocation dynamique d'un champ
char* allocateField(const char* source) {
    char* field = malloc(strlen(source) + 1);
    if (!field) {
        perror("Erreur d'allocation mémoire pour un champ");
        exit(EXIT_FAILURE);
    }
    strcpy(field, source);
    return field;
}

// Fonction de hachage simple
unsigned int hashFunction(const char* key, int nbSlots) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31 + *key++) % nbSlots;
    }
    return hash;
}

// Analyse du fichier et remplissage de la table de hachage
t_hashtable* parseFileHash(const char* filename, t_metadata* metadata, int nbSlots) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erreur d'ouverture de fichier");
        exit(EXIT_FAILURE);
    }

    t_hashtable* table = malloc(sizeof(t_hashtable));
    table->slots = calloc(nbSlots, sizeof(t_node*));
    table->nbSlots = nbSlots;
    table->nbTuples = 0;

    metadata->sep = '\0';
    metadata->nbFields = 0;
    metadata->fieldNames = NULL;

    char* line;
    while ((line = readLine(file)) != NULL) {
        if (line[0] == '#' && strlen(line) > 1) {
            free(line);
            continue;
        }

        if (strlen(line) == 1 && metadata->sep == '\0') {
            metadata->sep = line[0];
            free(line);
            continue;
        }

        if (metadata->nbFields == 0) {
            metadata->nbFields = atoi(line);
            metadata->fieldNames = malloc(metadata->nbFields * sizeof(char*));
            for (int i = 0; i < metadata->nbFields; i++) {
                metadata->fieldNames[i] = NULL;
            }
            free(line);
            continue;
        }

        if (metadata->fieldNames[0] == NULL) {
            char* token = strtok(line, &metadata->sep);
            for (int i = 0; i < metadata->nbFields; i++) {
                metadata->fieldNames[i] = allocateField(token ? token : "");
                token = strtok(NULL, &metadata->sep);
            }
            free(line);
            continue;
        }

        t_tuple tuple;
        char* token = strtok(line, &metadata->sep);
        tuple.key = allocateField(token);
        tuple.value = malloc((metadata->nbFields - 1) * sizeof(char*));
        for (int i = 0; i < metadata->nbFields - 1; i++) {
            token = strtok(NULL, &metadata->sep);
            tuple.value[i] = allocateField(token ? token : "");
        }

        insertTupleHash(table, &tuple, nbSlots, metadata);
        free(line);
    }

    fclose(file);
    return table;
}

// Insertion d'un tuple dans la table de hachage
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata) {
    unsigned int index = hashFunction(tuple->key, nbSlots);

    t_node* newNode = malloc(sizeof(t_node));
    newNode->data.key = allocateField(tuple->key);
    newNode->data.value = malloc((metadata->nbFields - 1) * sizeof(char*));
    for (int i = 0; i < metadata->nbFields - 1; i++) {
        newNode->data.value[i] = allocateField(tuple->value[i]);
    }

    newNode->next = table->slots[index];
    table->slots[index] = newNode;
    table->nbTuples++;
}

// Recherche d'une clé dans la table de hachage
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots) {
    unsigned int index = hashFunction(key, nbSlots);
    t_node* current = table->slots[index];
    int comparisons = 0;

    while (current) {
        comparisons++;
        if (strcmp(current->data.key, key) == 0) {
            printf("Recherche de %s : trouvé ! nb comparaisons : %d\n", key, comparisons);
            printf("mot : %s\n", current->data.key);
            for (int j = 0; j < metadata->nbFields - 1; j++) {
                printf("%s : %s\n", metadata->fieldNames[j + 1],
                    strlen(current->data.value[j]) > 0 ? current->data.value[j] : "X");
            }
            return;
        }
        current = current->next;
    }
    printf("Recherche de %s : échec ! nb comparaisons : %d\n", key, comparisons);
}

// Libération de la mémoire de la table de hachage
void freeHashTable(t_hashtable* table, t_metadata* metadata) {
    for (int i = 0; i < table->nbSlots; i++) {
        t_node* current = table->slots[i];
        while (current) {
            t_node* temp = current;
            free(current->data.key);
            for (int j = 0; j < metadata->nbFields - 1; j++) {
                free(current->data.value[j]);
            }
            free(current->data.value);
            current = current->next;
            free(temp);
        }
    }
    free(table->slots);
    for (int i = 0; i < metadata->nbFields; i++) {
        free(metadata->fieldNames[i]);
    }
    free(metadata->fieldNames);
    free(table);
}