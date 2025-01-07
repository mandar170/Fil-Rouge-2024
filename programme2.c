#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Définition des structures
typedef struct {
    char* key;           // Clé dynamique
    char*** definitions; // Tableau dynamique de tableaux dynamiques pour les définitions
    int nbDefinitions;   // Nombre d'occurrences du mot
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
// Prototypes
char* readLine(FILE* file);
char* allocateField(const char* source);
t_hashtable* parseFileHash(const char* filename, t_metadata* metadata, int nbSlots, unsigned int (*hashFunc)(const char*, int));
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata, unsigned int (*hashFunc)(const char*, int));
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots, unsigned int (*hashFunc)(const char*, int));
unsigned int hashFunction(const char* key, int nbSlots);
unsigned int hashFunction1(const char* key, int nbSlots);
unsigned int hashFunction2(const char* key, int nbSlots);
void freeHashTable(t_hashtable* table, t_metadata* metadata);

// Fonction principale
int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <filename> <nbSlots> <hashFunctionChoice>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];
    int nbSlots = atoi(argv[2]);
    int hashFunctionChoice = atoi(argv[3]);
    if (nbSlots <= 0) {
        fprintf(stderr, "Erreur : nbSlots doit être supérieur à 0.\n");
        exit(EXIT_FAILURE);
    }

    unsigned int (*hashFunc)(const char*, int) = NULL;
    if (hashFunctionChoice == 1) {
        hashFunc = hashFunction1;
    } else if (hashFunctionChoice == 2) {
        hashFunc = hashFunction2;
    } else {
        fprintf(stderr, "Erreur : choix de fonction de hachage invalide (1 ou 2).\n");
        exit(EXIT_FAILURE);
    }

    t_metadata metadata;
    t_hashtable* table = parseFileHash(filename, &metadata, nbSlots, hashFunc);

    printf("%d mots indexés dans %d alvéoles\n", table->nbTuples, nbSlots);
    printf("Saisir les mots recherchés :\n\n");

    char key[1000];
    while (fgets(key, sizeof(key), stdin)) {
        size_t len = strlen(key);
        if (key[len - 1] == '\n') key[len - 1] = '\0';
        if (strlen(key) == 0) break;
        searchKeyHash(table, &metadata, key, nbSlots, hashFunc);
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
    assert(line != NULL); 
    strcpy(line, buffer);
    return line;
}

// Allocation dynamique d'un champ
char* allocateField(const char* source) {
    char* field = malloc(strlen(source) + 1);
    assert(field != NULL); 
    strcpy(field, source);
    return field;
}

// Fonction de hachage 1
unsigned int hashFunction1(const char* key, int nbSlots) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key++;
    }
    return hash % nbSlots;
}

// Fonction de hachage 2
unsigned int hashFunction2(const char* key, int nbSlots) {
    unsigned int hash = 5381;
    while (*key) {
        hash = ((hash << 5) + hash) + *key++;
    }
    return hash % nbSlots;
}

// Analyse du fichier et remplissage de la table de hachage
t_hashtable* parseFileHash(const char* filename, t_metadata* metadata, int nbSlots, unsigned int (*hashFunc)(const char*, int)) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erreur d'ouverture de fichier");
        exit(EXIT_FAILURE);
    }

    t_hashtable* table = malloc(sizeof(t_hashtable));
    assert(table != NULL);
    table->slots = calloc(nbSlots, sizeof(t_node*));
    assert(table->slots != NULL);
    table->nbSlots = nbSlots;
    table->nbTuples = 0;

    metadata->sep = '\0';           // Séparateur non défini au départ
    metadata->nbFields = 0;        // Nombre de champs non encore connu
    metadata->fieldNames = NULL;   // Pas encore de noms de champs

    char* line;
    int step = 0; // Étape de traitement (0 : séparateur, 1 : nbFields, 2 : noms des champs, 3+ : données)

    while ((line = readLine(file)) != NULL) {
        // Ignorer les lignes de commentaires (commençant par # mais contenant plus que le séparateur)
        if (line[0] == '#' && strlen(line) > 1) {
            free(line);
            continue;
        }

        // Étape 0 : Détection du séparateur
        if (step == 0) {
            if (strlen(line) == 1) { // Une ligne avec un seul caractère
                metadata->sep = line[0];
                printf("Séparateur détecté : '%c'\n", metadata->sep);
                free(line);
                step++;
                continue;
            } else {
                fprintf(stderr, "Erreur : séparateur invalide (doit être un caractère unique).\n");
                free(line);
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }

        // Étape 1 : Lecture du nombre de champs
        if (step == 1) {
            metadata->nbFields = atoi(line);
            if (metadata->nbFields <= 0) {
                fprintf(stderr, "Erreur : nombre de champs invalide : %s\n", line);
                free(line);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            metadata->fieldNames = malloc(metadata->nbFields * sizeof(char*));
            assert(metadata->fieldNames != NULL);
            for (int i = 0; i < metadata->nbFields; i++) {
                metadata->fieldNames[i] = NULL;
            }
            printf("%d champs détectés.\n", metadata->nbFields);
            free(line);
            step++;
            continue;
        }

        // Étape 2 : Lecture des noms des champs
        if (step == 2) {
            char* token = strtok(line, &metadata->sep);
            for (int i = 0; i < metadata->nbFields; i++) {
                if (token != NULL) {
                    metadata->fieldNames[i] = allocateField(token);
                    token = strtok(NULL, &metadata->sep);
                } else {
                    metadata->fieldNames[i] = allocateField("");
                }
            }
            printf("Noms des champs : ");
            for (int i = 0; i < metadata->nbFields; i++) {
                printf("%s%s", metadata->fieldNames[i], (i == metadata->nbFields - 1) ? "\n" : ", ");
            }
            free(line);
            step++;
            continue;
        }

        // Étape 3+ : Lecture et insertion des données
        if (step >= 3) {
            t_tuple tuple;
            char* token = strtok(line, &metadata->sep);

            // Lire la clé
            if (token == NULL) {
                fprintf(stderr, "Erreur : ligne mal formatée, clé manquante.\n");
                free(line);
                continue;
            }
            tuple.key = allocateField(token);

            // Lire les valeurs associées
            tuple.definitions = malloc(sizeof(char**));
            assert(tuple.definitions != NULL);
            tuple.definitions[0] = malloc((metadata->nbFields - 1) * sizeof(char*));
            assert(tuple.definitions[0] != NULL);
            for (int i = 0; i < metadata->nbFields - 1; i++) {
                token = strtok(NULL, &metadata->sep);
                tuple.definitions[0][i] = allocateField(token ? token : "");
            }
            tuple.nbDefinitions = 1;

            // Insérer le tuple dans la table
            insertTupleHash(table, &tuple, nbSlots, metadata, hashFunc);
            free(line);
        }
    }

    fclose(file);
    return table;
}

// Insertion d'un tuple dans la table de hachage
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata, unsigned int (*hashFunc)(const char*, int)) {
    unsigned int index = hashFunc(tuple->key, nbSlots);
    t_node* current = table->slots[index];

    while (current) {
        if (strcmp(current->data.key, tuple->key) == 0) {
            // Ajouter une nouvelle occurrence pour cette clé
            current->data.nbDefinitions++;
            current->data.definitions = realloc(current->data.definitions, current->data.nbDefinitions * sizeof(char**));
            assert(current->data.definitions != NULL);
            current->data.definitions[current->data.nbDefinitions - 1] = malloc((metadata->nbFields - 1) * sizeof(char*));
            assert(current->data.definitions[current->data.nbDefinitions - 1] != NULL);
            for (int i = 0; i < metadata->nbFields - 1; i++) {
                current->data.definitions[current->data.nbDefinitions - 1][i] = allocateField(tuple->definitions[0][i]);
            }
            return;
        }
        current = current->next;
    }

    // Si la clé n'existe pas encore, créer un nouveau nœud
    t_node* newNode = malloc(sizeof(t_node));
    assert(newNode != NULL);
    newNode->data.key = allocateField(tuple->key);
    newNode->data.definitions = malloc(sizeof(char**));
    assert(newNode->data.definitions != NULL);
    newNode->data.definitions[0] = malloc((metadata->nbFields - 1) * sizeof(char*));
    assert(newNode->data.definitions[0] != NULL);
    for (int i = 0; i < metadata->nbFields - 1; i++) {
        newNode->data.definitions[0][i] = allocateField(tuple->definitions[0][i]);
    }
    newNode->data.nbDefinitions = 1;
    newNode->next = table->slots[index];
    table->slots[index] = newNode;
    table->nbTuples++;
}

// Recherche d'une clé dans la table de hachage
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots, unsigned int (*hashFunc)(const char*, int)) {
    unsigned int index = hashFunc(key, nbSlots);
    t_node* current = table->slots[index];
    int comparisons = 0;
    int found = 0; // Indicateur si au moins une clé est trouvée

    while (current) {
        comparisons++;
        if (strcmp(current->data.key, key) == 0) {
            if (!found) {
                printf("Recherche de %s : trouvé ! nb comparaisons : %d\n", key, comparisons);
                found = 1;
            }
            printf("mot : %s\n", current->data.key);
            for (int d = 0; d < current->data.nbDefinitions; d++) {
                printf("Définition %d :\n", d + 1);
                for (int j = 0; j < metadata->nbFields - 1; j++) {
                    printf("  %s : %s\n", metadata->fieldNames[j + 1],
                        strlen(current->data.definitions[d][j]) > 0 ? current->data.definitions[d][j] : "X");
                }
                printf("\n");
            }
        }
        current = current->next;
    }
    if (!found) {
        printf("Recherche de %s : échec ! nb comparaisons : %d\n", key, comparisons);
    }
}

// Libération de la mémoire de la table de hachage
void freeHashTable(t_hashtable* table, t_metadata* metadata) {
    for (int i = 0; i < table->nbSlots; i++) {
        t_node* current = table->slots[i];
        while (current) {
            t_node* temp = current;
            free(current->data.key);
            for (int d = 0; d < current->data.nbDefinitions; d++) {
                for (int j = 0; j < metadata->nbFields - 1; j++) {
                    free(current->data.definitions[d][j]);
                }
                free(current->data.definitions[d]);
            }
            free(current->data.definitions);
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