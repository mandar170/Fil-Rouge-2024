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

typedef unsigned int (*hashFunction)(const char* key, int nbSlots);

// Prototypes
char* readLine(FILE* file);
char* allocateField(const char* source);
t_hashtable* parseFileHash(FILE* inputFile, t_metadata* metadata, int nbSlots, hashFunction hashFunc);
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata, hashFunction hashFunc);
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots, hashFunction hashFunc);
unsigned int hashFunction1(const char* key, int nbSlots);
unsigned int hashFunction2(const char* key, int nbSlots);
void freeHashTable(t_hashtable* table, t_metadata* metadata);
void saveHashTableToFile(t_hashtable* table, FILE* output, t_metadata* metadata);
void afficherAide();

// Fonction principale
int main(int argc, char* argv[]) {
    if (argc == 2 && strcmp(argv[1], "-help") == 0) {
        afficherAide();
        return 0;
    }

    const char* inputFile = NULL;
    const char* outputFile = NULL;
    int nbSlots = -1;
    int hashFunctionChoice = -1;

    // Analyser les arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-i", 2) == 0) {
            inputFile = argv[i] + 2; // Nom du fichier d'entrée
        } else if (strncmp(argv[i], "-o", 2) == 0) {
            outputFile = argv[i] + 2; // Nom du fichier de sortie
        } else if (strncmp(argv[i], "-s", 2) == 0) {
            nbSlots = atoi(argv[i] + 2); // Nombre d'alvéoles
            if (nbSlots <= 0) {
                fprintf(stderr, "Erreur : nombre d'alvéoles invalide.\n");
                return EXIT_FAILURE;
            }
        } else if (strncmp(argv[i], "-h", 2) == 0) {
            hashFunctionChoice = atoi(argv[i] + 2); // Choix de la fonction de hachage
            if (hashFunctionChoice < 1) {
                fprintf(stderr, "Erreur : numéro de fonction de hachage invalide.\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-help") == 0) {
            afficherAide();
            return 0;
        } else {
            fprintf(stderr, "Erreur : argument inconnu %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    // Vérification des paramètres requis
    if (nbSlots == 0 || hashFunctionChoice == 0) {
        fprintf(stderr, "Erreur : les paramètres -s et -h sont obligatoires.\n");
        return EXIT_FAILURE;
    }

    // Définir la fonction de hachage choisie
    hashFunction hashFunc = NULL;
    if (hashFunctionChoice == 1) {
        hashFunc = hashFunction1;
    } else if (hashFunctionChoice == 2) {
        hashFunc = hashFunction2;
    } else {
        fprintf(stderr, "Erreur : numéro de fonction de hachage invalide (1 ou 2 uniquement).\n");
        return EXIT_FAILURE;
    }

    t_metadata metadata;
    FILE* input = inputFile ? fopen(inputFile, "r") : stdin;
    if (inputFile && !input) {
        perror("Erreur d'ouverture du fichier d'entrée");
        return EXIT_FAILURE;
    }

    // Construire la table de hachage
    t_hashtable* table = parseFileHash(input, &metadata, nbSlots, hashFunc);
    if (inputFile) fclose(input);

    // Définir la sortie
    FILE* output = outputFile ? fopen(outputFile, "w") : stdout;
    if (outputFile && !output) {
        perror("Erreur d'ouverture du fichier de sortie");
        freeHashTable(table, &metadata);
        return EXIT_FAILURE;
    }

    // Sauvegarde ou affichage de la table
    saveHashTableToFile(table, output, &metadata);

    // Si un fichier de sortie a été utilisé, le fermer
    if (outputFile) fclose(output);

    // Libération de la mémoire
    freeHashTable(table, &metadata);
    return EXIT_SUCCESS;
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


// Analyse du fichier et remplissage de la table de hachage
t_hashtable* parseFileHash(FILE* inputFile, t_metadata* metadata, int nbSlots, hashFunction hashFunc) {    
    FILE* file = inputFile;
    int isManualInput = (file == stdin); // Vérifier si l'entrée est manuelle

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

    while (1) {
        if (isManualInput) {
            if (step == 0) {
                printf("Entrez le séparateur à utiliser (un seul caractère) : ");
            } else if (step == 1) {
                printf("Entrez le nombre de champs dans chaque enregistrement : ");
            } else if (step == 2) {
                printf("Entrez les noms des champs, séparés par le séparateur : ");
            } else {
                printf("Entrez une nouvelle ligne de données (clé et champs séparés par le séparateur), ou une ligne vide pour terminer : ");
            }
        }

        line = readLine(file);
        if (!line) break; // Fin du fichier ou ligne vide

        // Ignore les commentaires
        if (line[0] == '#' && strlen(line) > 1) {
            free(line);
            continue;
        }

        if (step == 0) {
            if (strlen(line) == 1) {
                metadata->sep = line[0];
                printf("Séparateur détecté : '%c'\n", metadata->sep);
                free(line);
                step++;
                continue;
            } else {
                fprintf(stderr, "Erreur : séparateur invalide (doit être un caractère unique).\n");
                free(line);
                if (isManualInput) continue;
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }

        if (step == 1) {
            metadata->nbFields = atoi(line);
            if (metadata->nbFields <= 0) {
                fprintf(stderr, "Erreur : nombre de champs invalide : %s\n", line);
                free(line);
                if (isManualInput) continue;
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

        if (step >= 3) {
            if (strlen(line) == 0) { // Ligne vide pour terminer l'entrée
                free(line);
                break;
            }

            t_tuple tuple;
            char* token = strtok(line, &metadata->sep);

            if (token == NULL) {
                fprintf(stderr, "Erreur : ligne mal formatée, clé manquante.\n");
                free(line);
                continue;
            }
            tuple.key = allocateField(token);

            tuple.definitions = malloc(sizeof(char**));
            assert(tuple.definitions != NULL);
            tuple.definitions[0] = malloc((metadata->nbFields - 1) * sizeof(char*));
            assert(tuple.definitions[0] != NULL);
            for (int i = 0; i < metadata->nbFields - 1; i++) {
                token = strtok(NULL, &metadata->sep);
                tuple.definitions[0][i] = allocateField(token ? token : "");
            }
            tuple.nbDefinitions = 1;

            insertTupleHash(table, &tuple, nbSlots, metadata, hashFunc);
            free(line);
        }
    }

    if (isManualInput) {
        printf("Fin de l'entrée manuelle.\n");
    }

    return table;
}

// Insertion d'un tuple dans la table de hachage
void insertTupleHash(t_hashtable* table, const t_tuple* tuple, int nbSlots, t_metadata* metadata, hashFunction hashFunc) {
    unsigned int index = hashFunc(tuple->key, nbSlots);  // Correction ici
    t_node* current = table->slots[index];

    // Vérifier si la clé existe déjà
    while (current) {
        if (strcmp(current->data.key, tuple->key) == 0) {
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
void searchKeyHash(t_hashtable* table, t_metadata* metadata, const char* key, int nbSlots, hashFunction hashFunc) {
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

void saveHashTableToFile(t_hashtable* table, FILE* output, t_metadata* metadata) {
    // Sauvegarder le séparateur
    fprintf(output, "%c\n", metadata->sep);

    // Sauvegarder le nombre de champs
    fprintf(output, "%d\n", metadata->nbFields);

    // Sauvegarder les noms de champs
    for (int i = 0; i < metadata->nbFields; i++) {
        fprintf(output, "%s%c", metadata->fieldNames[i], (i == metadata->nbFields - 1) ? '\n' : metadata->sep);
    }

    // Sauvegarder les tuples
    for (int i = 0; i < table->nbSlots; i++) {
        t_node* current = table->slots[i];
        while (current) {
            fprintf(output, "%s", current->data.key);
            for (int d = 0; d < current->data.nbDefinitions; d++) {
                for (int j = 0; j < metadata->nbFields - 1; j++) {
                    fprintf(output, "%c%s", metadata->sep, current->data.definitions[d][j]);
                }
                fprintf(output, "\n");
            }
            current = current->next;
        }
    }
}

void afficherAide() {
    printf("Usage : ./prog3 -h<nom ou numéro de la fonction de hachage> -s<nombre d'alvéoles> -i<fichier entrée> -o<fichier sortie>\n");
    printf("Options :\n");
    printf("  -h<nom/numéro>    Fonction de hachage (1 pour une fonction de hachage simple, etc.)\n");
    printf("  -s<nombre>        Nombre d'alvéoles pour la table de hachage\n");
    printf("  -i<fichier>       Fichier d'entrée contenant les données à indexer\n");
    printf("  -o<fichier>       Fichier de sortie pour enregistrer la table de hachage\n");
    printf("  -help             Afficher ce message d'aide\n");
}