#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Définition des structures
typedef struct {
    char* key;       // Clé dynamique
    char** value;    // Tableau dynamique de champs
} t_tuple;

typedef struct {
    t_tuple* tuples; // Tableau dynamique de tuples
    int sizeTab;     // Taille allouée
    int nbTuples;    // Nombre de tuples enregistrés
} t_tupletable;

typedef struct {
    char sep;        // Séparateur
    int nbFields;    // Nombre de champs
    char** fieldNames; // Tableau dynamique pour les noms de champs
} t_metadata;

// Fonction pour lire une ligne
char* readLine(FILE* file) {
    char buffer[1000];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        return NULL;
    }
    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    char* line = malloc(len + 1);
    assert(line!=NULL); 
    strcpy(line, buffer);
    return line;
}

// Fonction pour allouer dynamiquement un champ
char* allocateField(const char* source) {
    char* field = malloc(strlen(source) + 1);
    if (!field) {
        perror("Erreur d'allocation mémoire pour un champ");
        exit(EXIT_FAILURE);
    }
    strcpy(field, source);
    return field;
}

// Fonction pour analyser un fichier
t_tupletable* parseFile(const char* filename, t_metadata* metadata) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erreur d'ouverture de fichier");
        exit(EXIT_FAILURE);
    }

    t_tupletable* table = malloc(sizeof(t_tupletable));
    assert(table!=NULL); 
    table->tuples = malloc(10 * sizeof(t_tuple));
    assert(table->tuples!=NULL); 
    table->sizeTab = 10;
    table->nbTuples = 0;

    metadata->sep = '\0';
    metadata->nbFields = 0;
    metadata->fieldNames = NULL;

    char* line;
    while ((line = readLine(file)) != NULL) {
        // Ignore les commentaires
        if (line[0] == '#' && strlen(line) > 1) {
            free(line);
            continue;
        }

        // Définition du séparateur
        if (strlen(line) == 1 && metadata->sep == '\0') {
            metadata->sep = line[0];
            free(line);
            continue;
        }

        // Définition du nombre de champs
        if (metadata->nbFields == 0) {
            metadata->nbFields = atoi(line);
            metadata->fieldNames = malloc(metadata->nbFields * sizeof(char*));
            assert(metadata->fieldNames !=NULL); 
            free(line);
            continue;
        }

        // Définition des noms des champs
        if (metadata->fieldNames[0] == NULL) {
            char* token = strtok(line, &metadata->sep);
            for (int i = 0; i < metadata->nbFields; i++) {
                metadata->fieldNames[i] = allocateField(token ? token : "");
                token = strtok(NULL, &metadata->sep);
            }
            free(line);
            continue;
        }

        // Lecture des données
        if (table->nbTuples >= table->sizeTab) {
            table->sizeTab *= 2;
            table->tuples = realloc(table->tuples, table->sizeTab * sizeof(t_tuple));
        }

        t_tuple* tuple = &table->tuples[table->nbTuples];
        char* token = strtok(line, &metadata->sep);

        // Lecture de la clé
        tuple->key = allocateField(token);

        // Lecture des valeurs
        tuple->value = malloc((metadata->nbFields - 1) * sizeof(char*));
        assert(tuple->value !=NULL); 
        for (int i = 0; i < metadata->nbFields - 1; i++) {
            token = strtok(NULL, &metadata->sep);
            tuple->value[i] = allocateField(token ? token : "");
        }

        table->nbTuples++;
        free(line);
    }

    fclose(file);
    return table;
}

// Fonction pour rechercher une clé
void searchKey(t_tupletable* table, t_metadata* metadata, const char* key) {
    int comparisons = 0;
    for (int i = 0; i < table->nbTuples; i++) {
        comparisons++;
        if (strcmp(table->tuples[i].key, key) == 0) {
            printf("Recherche de %s : trouvé ! nb comparaisons : %d\n", key, comparisons);
            printf("mot : %s\n", table->tuples[i].key);
            for (int j = 0; j < metadata->nbFields - 1; j++) {
                printf("%s : %s\n", metadata->fieldNames[j + 1],
                       strlen(table->tuples[i].value[j]) > 0 ? table->tuples[i].value[j] : "X");
            }
            return;
        }
    }
    printf("Recherche de %s : échec ! nb comparaisons : %d\n", key, comparisons);
}

// Fonction principale
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];
    t_metadata metadata;
    t_tupletable* table = parseFile(filename, &metadata);

    printf("%d mots indexés\n", table->nbTuples);
    printf("Saisir les mots recherchés :\n");

    char key[1000];
    while (fgets(key, sizeof(key), stdin)) {
        size_t len = strlen(key);
        if (key[len - 1] == '\n') key[len - 1] = '\0';
        if (strlen(key) == 0) break;
        searchKey(table, &metadata, key);
    }

    // Libération de la mémoire
    for (int i = 0; i < table->nbTuples; i++) {
        free(table->tuples[i].key);
        for (int j = 0; j < metadata.nbFields - 1; j++) {
            free(table->tuples[i].value[j]);
        }
        free(table->tuples[i].value);
    }
    free(table->tuples);
    for (int i = 0; i < metadata.nbFields; i++) {
        free(metadata.fieldNames[i]);
    }
    free(metadata.fieldNames);
    free(table);

    return 0;
}
