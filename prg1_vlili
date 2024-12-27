#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 1000
#define MAXFIELDS 15

typedef char* t_field;

typedef struct {
    t_field *fields;  // Tableau dynamique de champs
    int nbFields;     // Nombre de champs (dont la clé)
} t_value;

typedef char* t_key;  // Clé : mot

typedef struct {
    t_key key;
    t_value value;
} t_tuple;

typedef struct {
    t_tuple *tuples;   // Tableau des tuples
    int sizeTab;       // Taille de la table (espace disponible)
    int nbTuples;      // Nombre de mots insérés
} t_tupletable;

t_field create_field(const char* value) {
    size_t len = strlen(value);
    t_field field = (t_field)malloc((len + 1) * sizeof(char));
    assert(field!=NULL); 
    strcpy(field, value);
    return field;
}

void free_field(t_field field) {
    if (field != NULL) {
        free(field);
    }
}

t_value create_value(int nbFields, char *fields[]) {
    t_value value;
    value.fields = (t_field*)malloc(nbFields * sizeof(t_field));
    assert(value.fields!=NULL); 

    value.nbFields = nbFields;
    for (int i = 0; i < nbFields; i++) {
        value.fields[i] = create_field(fields[i]);
    }
    return value;
}

void free_value(t_value value) {
    if (value.fields != NULL) {
        for (int i = 0; i < value.nbFields; i++) {
            free_field(value.fields[i]);
        }
        free(value.fields);
    }
}

// Fonction d'insertion modifiée pour gérer les clés identiques
void inserer_tuple(t_tupletable *table, t_key cle, t_value valeur) {
    // Vérifier si la clé existe déjà
    for (int i = 0; i < table->nbTuples; i++) {
        if (strcmp(table->tuples[i].key, cle) == 0) {
            // Ajouter les nouveaux champs à l'entrée existante
            table->tuples[i].value.fields = realloc(table->tuples[i].value.fields,(table->tuples[i].value.nbFields + valeur.nbFields) * sizeof(t_field));
            assert(table->tuples[i].value.fields != NULL);
            for (int j = 0; j < valeur.nbFields; j++) {
                table->tuples[i].value.fields[table->tuples[i].value.nbFields + j] = create_field(valeur.fields[j]);
            }
            table->tuples[i].value.nbFields += valeur.nbFields;
            return;
        }
    }

    // Sinon, insérer normalement
    if (table->nbTuples >= table->sizeTab) {
        table->sizeTab *= 2;
        table->tuples = realloc(table->tuples, table->sizeTab * sizeof(t_tuple));
        assert(table->tuples != NULL);
    }
    table->tuples[table->nbTuples].key = create_field(cle);
    table->tuples[table->nbTuples].value = valeur;
    table->nbTuples++;
}

void free_table(t_tupletable *table) {
    if (table->tuples != NULL) {
        for (int i = 0; i < table->nbTuples; i++) {
            free_field(table->tuples[i].key);
            free_value(table->tuples[i].value);
        }
        free(table->tuples);
    }
}

void rech_tuple(t_tupletable *table, t_key cle) {
    int comparaisons = 0;
    for (int i = 0; i < table->nbTuples; i++) {
        comparaisons++;
        if (strcmp(table->tuples[i].key, cle) == 0) {
            printf("Recherche de '%s' : trouvé ! nb comparaisons : %d\n", cle, comparaisons);
            for (int j = 0; j < table->tuples[i].value.nbFields; j++) {
                printf("Version %d: %s\n", j + 1, table->tuples[i].value.fields[j]);
            }
            return;
        }
    }
    printf("Recherche de '%s' : échec ! nb comparaisons : %d\n\n", cle, comparaisons);
}

void print_table(t_tupletable *table) {
    printf("%d mots indexés\n", table->nbTuples);
}

void lire_fichier(FILE *file, t_tupletable *table) {
    char line[MAXLEN];
    char *token;
    char *fields[MAXFIELDS];
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
        } else {
            fprintf(stderr, "Erreur : impossible de lire le nombre de champs.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Lire les autres lignes avec les données
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || strlen(line) <= 1) continue; // Ignorer commentaires et lignes vides

        // Traitement des lignes de données
        token = strtok(line, &separateur);
        if (token != NULL) {
            char *key = strdup(token);  // Clé : premier mot
            assert(key!=NULL); 
            int i = 0;
            while ((token = strtok(NULL, &separateur)) != NULL && i < MAXFIELDS - 1) {
                fields[i] = strdup(token);
                assert(fields[i]!=NULL);
                i++; 
            }

            // Insérer dans la table
            t_value value = create_value(i, fields);
            inserer_tuple(table, key, value);

            // Libérer les champs alloués pour cette ligne
            free(key);
            for (int j = 0; j < i; j++) {
                free(fields[j]);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage : %s <fichier>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Ouverture du fichier
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le fichier %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Initialisation de la table
    t_tupletable table = {NULL, 100, 0};
    table.tuples = (t_tuple*)malloc(table.sizeTab * sizeof(t_tuple));
    assert(table.tuples != NULL);

    // Lire le fichier et remplir la table
    lire_fichier(file, &table);
    fclose(file);

    // Afficher le nombre de mots indexés
    print_table(&table);

    // Recherche des mots
    char search_key[MAXLEN];
    printf("Saisir les mots recherchés (Ctrl+D pour quitter) :\n\n");
    while (fgets(search_key, sizeof(search_key), stdin)) {
        search_key[strcspn(search_key, "\n")] = '\0'; // Supprimer le '\n'
        rech_tuple(&table, search_key);
    }

    // Libérer la mémoire
    free_table(&table);

    return EXIT_SUCCESS;
}
