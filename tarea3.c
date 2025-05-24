#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h> // Para getch() en Windows
#include "list.h"
#include "stack.h"
#include "map.h"
#include "queue.h"
#include "extra.h"

typedef struct {
    char name[50];
    float weight;
    int value;
} Item;

typedef struct {
    int id;
    char name[100];
    char description[300];
    int is_final;
    int up, down, left, right;
    List* items;
} Scenario;

typedef struct {
    int current_scenario;
    List* inventory;
    float total_weight;
    int total_score;
    int time_left;
    Stack* path;
} Player;

int int_equal(void* key1, void* key2) {
    return *(int*)key1 == *(int*)key2;
}

void print_items(List* items) {
    if (!list_size(items)) {
        printf("Ninguno");
        return;
    }
    void* ptr = list_first(items);
    while (ptr) {
        Item* item = (Item*)ptr;
        printf("%s (%.1f kg)", item->name, item->weight);
        ptr = list_next(items);
        if (ptr) printf(", ");
    }
}

void load_graph_from_csv(Map* scenarios, int* scenario_count) {
    FILE* archivo = fopen("graphquest.csv", "r");
    if (!archivo) {
        printf("ERROR: No se pudo abrir el archivo graphquest.csv\n");
        return;
    }

    char** campos;
    fgets(malloc(500), 500, archivo); // Saltar encabezado CSV

    while ((campos = leer_linea_csv(archivo, ',')) != NULL) {
        Scenario* escenario = (Scenario*)malloc(sizeof(Scenario));
        escenario->id = atoi(campos[0]);
        strcpy(escenario->name, campos[1]);
        strcpy(escenario->description, campos[2]);
        escenario->is_final = atoi(campos[8]);
        escenario->items = list_create();
        escenario->up = atoi(campos[4]);
        escenario->down = atoi(campos[5]);
        escenario->left = atoi(campos[6]);
        escenario->right = atoi(campos[7]);

        List* items_csv = split_string(campos[3], ";");

        for (char* item_str = list_first(items_csv); item_str != NULL; item_str = list_next(items_csv)) {
            List* valores = split_string(item_str, ",");
            char* nombre = list_first(valores);
            int valor = atoi(list_next(valores));
            int peso = atoi(list_next(valores));

            Item* item = (Item*)malloc(sizeof(Item));
            strcpy(item->name, nombre);
            item->value = valor;
            item->weight = peso;

            list_pushBack(escenario->items, item);

            list_clean(valores);
            free(valores);
        }

        list_clean(items_csv);
        free(items_csv);

        map_insert(scenarios, &escenario->id, escenario);
        (*scenario_count)++;

        printf("Escenario ID %d cargado: %s\n", escenario->id, escenario->name);
    }

    fclose(archivo);
}

void initialize_player(Player* p) {
    p->current_scenario = 1;
    p->inventory = list_create();
    p->total_weight = 0.0;
    p->total_score = 0;
    p->time_left = 10;
    p->path = stack_create(NULL);
    int* id = malloc(sizeof(int));
    *id = 1;
    stack_push(p->path, id);
}

void display_state(Map* scenarios, Player* p, int player_id) {
    limpiarPantalla();
    MapPair* pair = map_search(scenarios, &p->current_scenario);
    if (!pair) {
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return;
    }
    Scenario* s = (Scenario*)pair->value;
    printf("\n=== Jugador %d ===\n", player_id);
    printf("Escenario: %s\n", s->description);
    printf("Elementos: ");
    print_items(s->items);
    printf("\nTiempo restante: %d\n", p->time_left);
    printf("Inventario: ");
    print_items(p->inventory);
    printf("\nPeso total: %.1f kg, Puntaje: %d\n", p->total_weight, p->total_score);
    printf("Direcciones: ");
    if (s->up != -1) printf("Arriba ");
    if (s->down != -1) printf("Abajo ");
    if (s->left != -1) printf("Izquierda ");
    if (s->right != -1) printf("Derecha ");
    printf("\n");
}

void manage_items(Map* scenarios, Player* p, int collect) {
    MapPair* pair = map_search(scenarios, &p->current_scenario);
    if (!pair) return;
    Scenario* s = (Scenario*)pair->value;
    List* source = collect ? s->items : p->inventory;
    if (!list_size(source)) {
        printf("%s vacio.\n", collect ? "Escenario" : "Inventario");
        presioneTeclaParaContinuar();
        return;
    }
    printf("%s:\n", collect ? "Items disponibles" : "Inventario");
    void* ptr = list_first(source);
    int i = 1;
    List* indices = list_create();
    while (ptr) {
        Item* item = (Item*)ptr;
        printf("%d. %s (%.1f kg, %d pts)\n", i, item->name, item->weight, item->value);
        int* index = malloc(sizeof(int));
        *index = i++;
        list_pushBack(indices, index);
        ptr = list_next(source);
    }
    printf("Seleccione item (0 para terminar): ");
    int choice;
    scanf("%d", &choice);
    while (choice > 0) {
        int valid = 0;
        void* idx_ptr = list_first(indices);
        while (idx_ptr) {
            if (*(int*)idx_ptr == choice) {
                valid = 1;
                break;
            }
            idx_ptr = list_next(indices);
        }
        if (!valid) {
            printf("Seleccion invalida. Intente de nuevo: ");
            scanf("%d", &choice);
            continue;
        }
        ptr = list_first(source);
        for (i = 1; i < choice && ptr; i++) ptr = list_next(source);
        if (ptr) {
            Item* item = (Item*)ptr;
            if (collect) {
                list_pushBack(p->inventory, item);
                list_popCurrent(source);
                p->total_weight += item->weight;
                p->total_score += item->value;
            } else {
                list_popCurrent(source);
                p->total_weight -= item->weight;
                p->total_score -= item->value;
                free(item);
            }
        }
        printf("Seleccione otro item (0 para terminar): ");
        scanf("%d", &choice);
    }
    list_clean(indices);
    free(indices);
    p->time_left--;
    presioneTeclaParaContinuar();
}

void move(Map* scenarios, Player* p) {
    MapPair* pair = map_search(scenarios, &p->current_scenario);
    if (!pair) {
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return;
    }
    Scenario* s = (Scenario*)pair->value;
    printf("Presione: w (Arriba), s (Abajo), a (Izquierda), d (Derecha), cualquier otra tecla para cancelar\n");
    char key = getch(); // Leer tecla sin esperar Enter
    int new_scenario = -1;
    char* direction = NULL;
    switch (key) {
        case 'w': case 'W':
            new_scenario = s->up;
            direction = "Arriba";
            break;
        case 's': case 'S':
            new_scenario = s->down;
            direction = "Abajo";
            break;
        case 'a': case 'A':
            new_scenario = s->left;
            direction = "Izquierda";
            break;
        case 'd': case 'D':
            new_scenario = s->right;
            direction = "Derecha";
            break;
        default: 
            printf("\nMovimiento cancelado.\n");
            presioneTeclaParaContinuar();
            return;
    }
    if (new_scenario != -1) {
        int time_cost = (int)ceil((p->total_weight + 1) / 10.0);
        if (p->time_left <= time_cost) {
            printf("\nTiempo agotado! Has perdido.\n");
            presioneTeclaParaContinuar();
            exit(0);
        }
        p->current_scenario = new_scenario;
        p->time_left -= time_cost;
        int* id = malloc(sizeof(int));
        *id = new_scenario;
        stack_push(p->path, id);
        MapPair* new_pair = map_search(scenarios, &new_scenario);
        if (!new_pair) {
            printf("Error: Nuevo escenario ID %d no encontrado.\n", new_scenario);
            return;
        }
        Scenario* new_s = (Scenario*)new_pair->value;
        if (new_s->is_final) {
            printf("\nHas llegado a la salida!\n");
            printf("Inventario: ");
            print_items(p->inventory);
            printf("\nPuntaje: %d\n", p->total_score);
            presioneTeclaParaContinuar();
            exit(0);
        }
    } else {
        printf("\nNo hay camino hacia %s.\n", direction);
    }
    presioneTeclaParaContinuar();
}

void free_resources(Map* scenarios, Player* players, int num_players) {
    MapPair* pair = map_first(scenarios);
    while (pair) {
        Scenario* s = (Scenario*)pair->value;
        void* item = list_first(s->items);
        while (item) {
            free(item);
            item = list_next(s->items);
        }
        list_clean(s->items);
        free(s->items);
        free(pair->key);
        free(s);
        pair = map_next(scenarios);
    }
    map_clean(scenarios);
    if (players == NULL) return;
    for (int i = 0; i < num_players; i++) {
        void* item = list_first(players[i].inventory);
        while (item) {
            free(item);
            item = list_next(players[i].inventory);
        }
        list_clean(players[i].inventory);
        free(players[i].inventory);
        void* id = stack_pop(players[i].path);
        while (id) {
            free(id);
            id = stack_pop(players[i].path);
        }
        stack_clean(players[i].path);
        free(players[i].path);
    }
}

int main() {
    Map* scenarios = map_create(int_equal);
    int scenario_count = 0;
    int running = 1;

    while (running) {
        load_graph_from_csv(scenarios, &scenario_count);
        if (!scenario_count) {
            printf("No se cargo el laberinto. Verifique el archivo CSV.\n");
            free_resources(scenarios, NULL, 0);
            free(scenarios);
            return 1;
        }

        printf("=== GraphQuest ===\n");
        printf("1. Cargar Laberinto\n2. Iniciar Partida\n3. Salir\nOpcion: ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            printf("Laberinto cargado.\n");
            presioneTeclaParaContinuar();
        } else if (choice == 2) {
            int num_players;
            printf("Cuantos jugadores? (1-2): ");
            scanf("%d", &num_players);
            if (num_players < 1 || num_players > 2) num_players = 1;
            Player players[2];
            Queue* turns = queue_create(NULL);
            for (int i = 0; i < num_players; i++) {
                initialize_player(&players[i]);
                queue_insert(turns, &players[i]);
            }
            while (1) {
                Player* current_player = (Player*)queue_remove(turns);
                if (!current_player) break;
                display_state(scenarios, current_player, current_player == &players[0] ? 1 : 2);
                printf("1. Recoger item\n2. Descartar item\n3. Moverse\n4. Salir\nOpcion: ");
                scanf("%d", &choice);
                switch (choice) {
                    case 1: manage_items(scenarios, current_player, 1); break;
                    case 2: manage_items(scenarios, current_player, 0); break;
                    case 3: move(scenarios, current_player); break;
                    case 4:
                        free_resources(scenarios, players, num_players);
                        free(scenarios);
                        queue_clean(turns);
                        free(turns);
                        running = 0;
                        break;
                }
                if (current_player->time_left > 0) queue_insert(turns, current_player);
            }
        } else if (choice == 3) {
            free_resources(scenarios, NULL, 0);
            free(scenarios);
            running = 0;
        }
    }
    return 0;
}