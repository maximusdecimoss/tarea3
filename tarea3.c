#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h> // para getch() en windows, lee teclas
#include "list.h"
#include "stack.h"
#include "map.h"
#include "queue.h"
#include "extra.h"

// esto es como una caja para guardar un objeto, como un cuchillo o una joya
typedef struct {
    char name[50]; // aqui va el nombre del objeto, maximo 50 letras
    float weight; // el peso, puede tener decimales
    int value; // los puntos que vale
} Item;

// esto es como una habitacion del laberinto, con sus detalles
typedef struct {
    int id; // un numero unico para la habitacion
    char name[100]; // el nombre, maximo 100 letras
    char description[300]; // una descripcion larga, maximo 300 letras
    int is_final; // 1 si es el final del juego, 0 si no
    int up, down, left, right; // numeros de las habitaciones vecinas, -1 si no hay camino
    List* items; // lista de objetos en la habitacion
} Scenario;

// esto es como la ficha de un jugador, guarda todo lo que lleva
typedef struct {
    int current_scenario; // en que habitacion esta
    List* inventory; // su mochila con objetos
    float total_weight; // cuanto pesan todos sus objetos
    int total_score; // sus puntos totales
    int time_left; // turnos que le quedan
    Stack* path; // una pila con el camino que ha seguido
} Player;

// prototipos, como una lista de funciones que usaremos mas adelante
int int_equal(void* key1, void* key2);
void print_items(List* items);
void wait_for_key(void);
int read_valid_option(int min, int max, const char* prompt, const char* error_msg);
int show_end_menu(void);
void load_graph_from_csv(Map* scenarios, int* scenario_count);
void initialize_player(Player* p);
void display_state(Map* scenarios, Player* p, int player_id);
void manage_items(Map* scenarios, Player* p, int collect);
void display_final_scores(Player* players, int num_players, int winner_id);
void move(Map* scenarios, Player* p, Player* players, int num_players, int player_id, Queue* turns, int* running);
void show_hints(Player* p);
void free_resources(Map* scenarios, Player* players, int num_players);

/*
 * oye, esta funcion la hicimos para comparar dos numeros y ver si son iguales
 * es como mirar dos tarjetas y checar si tienen el mismo numero
 */
int int_equal(void* key1, void* key2) {
    return *(int*)key1 == *(int*)key2; // comparamos los valores
}

/*
 * esta funcion la hicimos para mostrar los objetos que hay en una lista
 * imagina que abres una caja y lees lo que tiene dentro
 */
void print_items(List* items) {
    if (!list_size(items)) { // si no hay nada en la lista
        printf("Ninguno");
        return; // nos salimos
    }
    void* ptr = list_first(items); // tomamos el primer objeto
    while (ptr) { // mientras haya objetos
        Item* item = (Item*)ptr; // lo convertimos a objeto
        printf("%s (%.1f kg, %d pts)", item->name, item->weight, item->value);
        ptr = list_next(items); // pasamos al siguiente
        if (ptr) printf(", "); // ponemos una coma si hay mas
    }
}

/*
 * esta funcion hace que el juego espere a que presiones una tecla
 * es como pausar hasta que toques el teclado
 */
void wait_for_key() {
    printf("Presiona una tecla para continuar...\n");
    getch(); // esperamos una tecla
    while (kbhit()) getch(); // limpiamos teclas extras
}

/*
 * esta funcion la hicimos para pedir un numero y asegurarnos que sea valido
 * es como pedirle a un amigo que elija entre 1 y 4, y no aceptar cosas raras
 */
int read_valid_option(int min, int max, const char* prompt, const char* error_msg) {
    char buffer[100]; // espacio para lo que escribe el jugador
    int choice; // aqui guardamos el numero que elige
    while (1) { // repetimos hasta que sea correcto
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) { // leemos lo escrito
            printf("%s", error_msg);
            continue; // si falla, pedimos otra vez
        }
        buffer[strcspn(buffer, "\n")] = '\0'; // quitamos el salto de linea
        if (strlen(buffer) == 0) { // si no escribio nada
            printf("%s", error_msg);
            continue; // pedimos otra vez
        }
        if (sscanf(buffer, "%d", &choice) != 1) { // intentamos hacer numero
            printf("%s", error_msg);
            continue; // si no es numero, pedimos otra vez
        }
        if (choice < min || choice > max) { // si esta fuera del rango
            printf("%s", error_msg);
            continue; // pedimos otra vez
        }
        while (getchar() != '\n' && !feof(stdin)); // limpiamos el teclado
        return choice; // devolvemos el numero bueno
    }
}

/*
 * esta funcion hace que el jugador vea el menu final y elija
 * es como mostrarle dos opciones al terminar el juego
 */
int show_end_menu() {
    return read_valid_option(1, 2, 
        "\n=== Fin del Juego ===\n1. Volver al menu principal\n2. Salir\nOpcion: ",
        "Opcion invalida. Por favor, ingrese 1 o 2.\n");
}

/*
 * oye, esta funcion la hicimos para leer el laberinto desde un archivo
 * es como abrir un cuaderno con instrucciones y guardar cada habitacion
 */
void load_graph_from_csv(Map* scenarios, int* scenario_count) {
    FILE* archivo = fopen("graphquest.csv", "r"); // abrimos el archivo
    if (!archivo) { // si no se pudo abrir
        printf("ERROR: No se pudo abrir el archivo graphquest.csv\n");
        return; // nos salimos
    }

    char line[500]; // espacio para leer una linea
    if (!fgets(line, sizeof(line), archivo)) { // saltamos la primera linea
        fclose(archivo); // cerramos el archivo
        return; // nos salimos
    }

    char** campos; // para guardar las partes de cada linea
    while ((campos = leer_linea_csv(archivo, ',')) != NULL) { // leemos cada linea
        Scenario* escenario = (Scenario*)malloc(sizeof(Scenario)); // creamos una habitacion
        escenario->id = atoi(campos[0]); // guardamos su numero
        strncpy(escenario->name, campos[1], 99); // copiamos el nombre, maximo 99 letras
        escenario->name[99] = '\0'; // ponemos un final al nombre
        strncpy(escenario->description, campos[2], 299); // copiamos la descripcion
        escenario->description[299] = '\0'; // ponemos un final a la descripcion
        escenario->is_final = atoi(campos[8]); // 1 si es el final, 0 si no
        escenario->items = list_create(); // creamos una lista para objetos
        escenario->up = atoi(campos[4]); // habitacion arriba
        escenario->down = atoi(campos[5]); // habitacion abajo
        escenario->left = atoi(campos[6]); // habitacion izquierda
        escenario->right = atoi(campos[7]); // habitacion derecha

        List* items_csv = split_string(campos[3], ";"); // separamos los objetos

        for (char* item_str = list_first(items_csv); item_str != NULL; item_str = list_next(items_csv)) { // para cada objeto
            List* valores = split_string(item_str, ","); // separamos nombre, valor, peso
            char* nombre = list_first(valores); // tomamos el nombre
            int valor = atoi(list_next(valores)); // tomamos el valor
            float peso = atof(list_next(valores)); // tomamos el peso, con decimales

            Item* item = (Item*)malloc(sizeof(Item)); // creamos un objeto
            strncpy(item->name, nombre, 49); // copiamos el nombre, maximo 49 letras
            item->name[49] = '\0'; // ponemos un final al nombre
            item->value = valor; // guardamos el valor
            item->weight = peso; // guardamos el peso

            list_pushBack(escenario->items, item); // ponemos el objeto en la habitacion

            list_clean(valores); // limpiamos la lista de valores
            free(valores); // liberamos memoria
        }

        list_clean(items_csv); // limpiamos la lista de objetos
        free(items_csv); // liberamos memoria

        int* key = malloc(sizeof(int)); // creamos un numero para la habitacion
        *key = escenario->id; // guardamos el numero
        map_insert(scenarios, key, escenario); // guardamos la habitacion
        (*scenario_count)++; // contamos una habitacion mas

        printf("Escenario ID %d cargado: %s\n", escenario->id, escenario->name);
    }

    fclose(archivo); // cerramos el archivo
}

/*
 * esta funcion la hicimos para preparar a un jugador nuevo
 * es como darle una mochila vacia y ponerlo en la primera habitacion
 */
void initialize_player(Player* p) {
    p->current_scenario = 1; // empezamos en la habitacion 1
    p->inventory = list_create(); // creamos una mochila vacia
    p->total_weight = 0.0; // la mochila no pesa nada
    p->total_score = 0; // tiene 0 puntos
    p->time_left = 10; // le damos 10 turnos
    p->path = stack_create(NULL); // creamos una pila para su camino
    int* id = malloc(sizeof(int)); // creamos un numero
    *id = 1; // ponemos la habitacion 1
    stack_push(p->path, id); // guardamos la habitacion
}

/*
 * esta funcion hace que el jugador vea donde esta y que lleva
 * es como mostrarle un mapa de la habitacion y su mochila
 */
void display_state(Map* scenarios, Player* p, int player_id) {
    limpiarPantalla(); // limpiamos la pantalla
    MapPair* pair = map_search(scenarios, &p->current_scenario); // buscamos la habitacion
    if (!pair) { // si no la encontramos
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return; // nos salimos
    }
    Scenario* s = (Scenario*)pair->value; // tomamos la habitacion
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

/*
 * oye, esta funcion la hicimos para que el jugador recoja o tire objetos
 * es como abrir la mochila y decidir que guardar o que dejar
 */
void manage_items(Map* scenarios, Player* p, int collect) {
    MapPair* pair = map_search(scenarios, &p->current_scenario); // buscamos la habitacion
    if (!pair) return; // si no la encontramos, salimos
    Scenario* s = (Scenario*)pair->value; // tomamos la habitacion
    List* source = collect ? s->items : p->inventory; // elegimos de donde tomar objetos
    if (!list_size(source)) { // si no hay objetos
        printf("%s vacio.\n", collect ? "Escenario" : "Inventario");
        wait_for_key();
        return; // nos salimos
    }
    printf("%s:\n", collect ? "Items disponibles" : "Inventario");
    void* ptr = list_first(source); // tomamos el primer objeto
    int i = 1; // contador para numerar
    List* indices = list_create(); // lista para los numeros
    while (ptr) { // mientras haya objetos
        Item* item = (Item*)ptr; // lo convertimos a objeto
        printf("%d. %s (%.1f kg, %d pts)\n", i, item->name, item->weight, item->value);
        int* index = malloc(sizeof(int)); // creamos un numero
        *index = i++; // guardamos el numero
        list_pushBack(indices, index); // lo ponemos en la lista
        ptr = list_next(source); // pasamos al siguiente
    }
    int choice; // para la eleccion del jugador
    do {
        choice = read_valid_option(0, list_size(indices), 
            "Seleccione item (0 para terminar): ",
            "Seleccion invalida. Por favor, ingrese un numero valido.\n");
        if (choice == 0) break; // si elige 0, terminamos
        ptr = list_first(source); // volvemos al primer objeto
        for (i = 1; i < choice && ptr; i++) ptr = list_next(source); // buscamos el objeto
        if (ptr) { // si lo encontramos
            Item* item = (Item*)ptr; // lo convertimos a objeto
            if (collect) { // si recogemos
                Item* new_item = (Item*)malloc(sizeof(Item)); // creamos un objeto nuevo
                strncpy(new_item->name, item->name, 49); // copiamos el nombre
                new_item->name[49] = '\0'; // ponemos un final
                new_item->weight = item->weight; // copiamos el peso
                new_item->value = item->value; // copiamos el valor
                list_pushBack(p->inventory, new_item); // ponemos en la mochila
                list_popCurrent(source); // quitamos del suelo
                free(item); // liberamos el objeto viejo
                p->total_weight += new_item->weight; // sumamos peso
                p->total_score += new_item->value; // sumamos puntos
            } else { // si tiramos
                list_popCurrent(source); // quitamos de la mochila
                p->total_weight -= item->weight; // restamos peso
                p->total_score -= item->value; // restamos puntos
                free(item); // liberamos el objeto
            }
        }
    } while (choice > 0); // repetimos hasta que elija 0
    list_clean(indices); // limpiamos los numeros
    free(indices); // liberamos memoria
    p->time_left--; // quitamos un turno
    wait_for_key();
}

/*
 * esta funcion hace que veamos los puntajes al final
 * es como mostrar una tabla con los resultados del juego
 */
void display_final_scores(Player* players, int num_players, int winner_id) {
    printf("\n¡Luz al final del túnel! Pero cuidado... hay salidas que son más trampas que finales felices.\n");
    printf("+---------+---------+--------+\n");
    printf("| Jugador | Puntaje | Nivel  |\n");
    printf("+---------+---------+--------+\n");
    for (int i = 0; i < num_players; i++) { // para cada jugador
        const char* level; // para el nivel (oro, plata, bronce)
        if (players[i].total_score > 70) level = "Oro"; // mas de 70 es oro
        else if (players[i].total_score >= 30) level = "Plata"; // 30 o mas es plata
        else level = "Bronce"; // menos de 30 es bronce
        printf("| %d       | %d      | %s    |\n", i + 1, players[i].total_score, level);
    }
    printf("+---------+---------+--------+\n");
    if (winner_id != 0) { // si alguien gano
        printf("Jugador %d ha ganado!\n", winner_id);
    } else { // si nadie gano
        printf("Tiempo agotado. Nadie ha ganado.\n");
    }
}

/*
 * oye, esta funcion la hicimos para mover al jugador por el laberinto
 * es como dejarlo caminar usando las teclas w, s, a, d
 */
void move(Map* scenarios, Player* p, Player* players, int num_players, int player_id, Queue* turns, int* running) {
    MapPair* pair = map_search(scenarios, &p->current_scenario); // buscamos donde esta
    if (!pair) { // si no lo encontramos
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return; // nos salimos
    }
    Scenario* s = (Scenario*)pair->value; // tomamos la habitacion
    int valid_move = 0; // para saber si el movimiento es bueno
    do {
        printf("Presione: w (Arriba), s (Abajo), a (Izquierda), d (Derecha), cualquier otra tecla para cancelar\n");
        char key = getch(); // leemos la tecla
        int new_scenario = -1; // para la nueva habitacion
        char* direction = NULL; // para la direccion
        switch (key) { // segun la tecla
            case 'w': case 'W':
                new_scenario = s->up; // arriba
                direction = "Arriba";
                break;
            case 's': case 'S':
                new_scenario = s->down; // abajo
                direction = "Abajo";
                break;
            case 'a': case 'A':
                new_scenario = s->left; // izquierda
                direction = "Izquierda";
                break;
            case 'd': case 'D':
                new_scenario = s->right; // derecha
                direction = "Derecha";
                break;
            default:
                printf("\nMovimiento cancelado.\n");
                wait_for_key();
                return; // salimos si no es una tecla valida
        }
        if (new_scenario != -1) { // si hay camino
            int time_cost = (int)ceil((p->total_weight + 1) / 10.0); // calculamos turnos que cuesta
            if (p->time_left <= time_cost) { // si no queda tiempo
                printf("\nTiempo agotado! Has perdido.\n");
                display_final_scores(players, num_players, 0);
                wait_for_key();
                int choice = show_end_menu(); // mostramos el menu final
                if (choice == 1) { // si quiere volver al menu
                    free_resources(NULL, players, num_players); // limpiamos jugadores
                    queue_clean(turns); // limpiamos turnos
                    free(turns); // liberamos turnos
                    *running = 1; // decimos que siga el juego
                    return; // volvemos al menu
                }
                *running = 0; // decimos que el juego termino
                return; // salimos
            }
            MapPair* new_pair = map_search(scenarios, &new_scenario); // buscamos la nueva habitacion
            if (!new_pair) { // si no existe
                printf("Error: Nuevo escenario ID %d no encontrado.\n", new_scenario);
                return; // salimos
            }
            p->current_scenario = new_scenario; // movemos al jugador
            p->time_left -= time_cost; // quitamos turnos
            int* id = malloc(sizeof(int)); // creamos un numero
            *id = new_scenario; // guardamos la habitacion
            stack_push(p->path, id); // ponemos en el camino
            Scenario* new_s = (Scenario*)new_pair->value; // tomamos la nueva habitacion
            valid_move = 1; // marcamos que el movimiento es bueno
            if (new_scenario == 16 || new_s->is_final) { // si llegamos al final
                printf("\n+----------------------------------+\n");
                printf("| ¡HAZ GANADO!                     |\n");
                printf("| Eres un explorador indomable     |\n");
                printf("+----------------------------------+\n");
                printf("Presiona una tecla para ver los resultados...\n");
                wait_for_key();
                display_final_scores(players, num_players, player_id);
                printf("Inventario: ");
                print_items(p->inventory);
                printf("\nPuntaje: %d\n", p->total_score);
                wait_for_key();
                int choice = show_end_menu(); // mostramos el menu final
                if (choice == 1) { // si quiere volver al menu
                    free_resources(NULL, players, num_players); // limpiamos jugadores
                    queue_clean(turns); // limpiamos turnos
                    free(turns); // liberamos turnos
                    *running = 1; // decimos que siga el juego
                    return; // volvemos al menu
                }
                *running = 0; // decimos que el juego termino
                return; // salimos
            }
        } else { // si no hay camino
            printf("\nNo hay camino hacia %s. Intenta de nuevo.\n", direction);
            wait_for_key();
        }
    } while (!valid_move); // repetimos hasta movernos
}

/*
 * esta funcion la hicimos para darle pistas al jugador
 * es como darle un libro con trucos para ganar
 */
void show_hints(Player* p) {
    limpiarPantalla();
    printf("\n=== Pistas para la Gloria ===\n");
    printf("Resuelve el puntaje de cada nivel para desbloquear su camino.\n");
    printf("Cada intento consume 1 unidad de tiempo. Tiempo actual: %d\n\n", p->time_left);

    int choice; // para elegir el nivel
    do {
        choice = read_valid_option(0, 3, 
            "Seleccione nivel (1: Oro, 2: Plata, 3: Bronce, 0: Salir): ",
            "Opcion invalida. Selecciona 1, 2, 3 o 0.\n");
        if (choice == 0) { // si quiere salir
            wait_for_key();
            return; // nos salimos
        }

        if (p->time_left <= 0) { // si no queda tiempo
            printf("\nTiempo agotado! Has perdido.\n");
            wait_for_key();
            return; // salimos
        }

        switch (choice) { // segun el nivel
            case 1: // oro
                printf("\nNivel Oro:\n");
                printf("Un tesoro crece con el cuadrado de tu camino, cuatro veces y media su peso, desde el inicio hasta cuatro pasos. Halla su valor.\n");
                printf("(Resuelve: integral de 0 a 4 de (9/2)x^2 dx)\n");
                break;
            case 2: // plata
                printf("\nNivel Plata:\n");
                printf("Un camino plateado guarda un secreto cuadrado: su valor menos diez veces él, más veinticinco, es nada. Toma el número mayor y multiplícalo por siete.\n");
                printf("(Resuelve: x^2 - 10x + 25 = 0, toma la raíz positiva, multiplica por 7)\n");
                break;
            case 3: // bronce
                printf("\nNivel Bronce:\n");
                printf("El bronce mide el cambio de un peso que crece como once y medio por el cuadrado de un paso. Encuentra su ritmo en el primer paso.\n");
                printf("(Deriva f(x) = 11.5x^2, evalúa en x = 1)\n");
                break;
        }

        int score = read_valid_option(INT_MIN, INT_MAX, 
            "Ingresa el puntaje: ",
            "Entrada invalida. Por favor, ingrese un numero valido.\n");
        
        switch (choice) { // segun el nivel
            case 1: // oro
                if (score == 96) { // si acierta
                    printf("\n¡Correcto! El camino al Oro:\n");
                    printf("Hacia el sur da un paso, sigue la corriente.\n");
                    printf("Donde el sol sale, dos pasos has de dar,\n");
                    printf("Al sur otra vez, no dejes de avanzar.\n");
                    printf("Al poniente un paso, donde el sol se va,\n");
                    printf("Sur de nuevo, y al final, la gloria hallarás.\n");
                    printf("Dos pasos al oriente, el camino cerrarás.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n");
                }
                break;
            case 2: // plata
                if (score == 35) { // si acierta
                    printf("\n¡Correcto! El camino a la Plata:\n");
                    printf("Por la senda plateada, tres pasos al sur,\n");
                    printf("Donde las sombras caen, sin temor ni dolor.\n");
                    printf("Luego al oriente, tres pasos contarás,\n");
                    printf("La plata te espera, si el rumbo no errarás.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n");
                }
                break;
            case 3: // bronce
                if (score == 23) { // si acierta
                    printf("\n¡Correcto! El camino al Bronce:\n");
                    printf("Por el bronce comienza, un paso al sur irás,\n");
                    printf("Al oriente un paso, donde el alba verás.\n");
                    printf("Dos pasos al sur, en la sombra avanzar,\n");
                    printf("Dos al oriente, el bronce a reclamar.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n");
                }
                break;
        }
        p->time_left--; // quitamos un turno
        printf("\nTiempo restante: %d\n", p->time_left);
        wait_for_key();
    } while (choice != 0); // repetimos hasta que salga
}

/*
 * esta funcion la hicimos para limpiar todo lo que usamos
 * es como recoger las cosas y tirarlas cuando terminamos
 */
void free_resources(Map* scenarios, Player* players, int num_players) {
    if (scenarios) { // si hay un mapa
        MapPair* pair = map_first(scenarios); // tomamos la primera habitacion
        while (pair) { // mientras haya habitaciones
            Scenario* s = (Scenario*)pair->value; // tomamos la habitacion
            if (s->items) { // si tiene objetos
                void* item = list_first(s->items); // tomamos el primero
                while (item) { // mientras haya objetos
                    free(item); // lo tiramos
                    item = list_next(s->items); // pasamos al siguiente
                }
                list_clean(s->items); // limpiamos la lista
                free(s->items); // liberamos la lista
            }
            free(pair->key); // tiramos el numero
            free(s); // tiramos la habitacion
            pair = map_next(scenarios); // siguiente habitacion
        }
        map_clean(scenarios); // limpiamos el mapa
    }
    if (players && num_players > 0) { // si hay jugadores
        for (int i = 0; i < num_players; i++) { // para cada jugador
            if (players[i].inventory) { // si tiene mochila
                void* item = list_first(players[i].inventory); // tomamos el primer objeto
                while (item) { // mientras haya objetos
                    free(item); // lo tiramos
                    item = list_next(players[i].inventory); // siguiente objeto
                }
                list_clean(players[i].inventory); // limpiamos la mochila
                free(players[i].inventory); // liberamos la mochila
                players[i].inventory = NULL; // marcamos como vacia
            }
            if (players[i].path) { // si tiene camino
                void* id = stack_pop(players[i].path); // tomamos el primer numero
                while (id) { // mientras haya numeros
                    free(id); // lo tiramos
                    id = stack_pop(players[i].path); // siguiente numero
                }
                stack_clean(players[i].path); // limpiamos el camino
                free(players[i].path); // liberamos el camino
                players[i].path = NULL; // marcamos como vacio
            }
        }
    }
}

/*
 * oye, esta funcion la hicimos para controlar todo el juego
 * es como el cerebro que dice que hacer en cada momento
 */
int main() {
    Map* scenarios = map_create(int_equal); // creamos un mapa para habitaciones
    int scenario_count = 0; // contamos las habitaciones
    int running = 1; // para saber si el juego sigue

    load_graph_from_csv(scenarios, &scenario_count); // cargamos el laberinto
    if (!scenario_count) { // si no se cargo
        printf("No se cargo el laberinto. Verifique el archivo CSV.\n");
        free_resources(scenarios, NULL, 0); // limpiamos
        free(scenarios); // liberamos el mapa
        return 1; // salimos con error
    }

    while (running) { // mientras el juego siga
        int choice = read_valid_option(1, 4, 
            "=== GraphQuest ===\n1. Cargar Laberinto\n2. Iniciar Partida\n3. Salir\n4. Pistas\nOpcion: ",
            "Entrada invalida. Por favor, ingrese un numero entre 1 y 4.\n");
        
        if (choice == 1) { // si quiere cargar laberinto
            free_resources(scenarios, NULL, 0); // limpiamos el mapa viejo
            map_clean(scenarios); // limpiamos el mapa
            scenarios = map_create(int_equal); // creamos uno nuevo
            scenario_count = 0; // reiniciamos el contador
            load_graph_from_csv(scenarios, &scenario_count); // cargamos de nuevo
            if (!scenario_count) { // si no se cargo
                printf("No se cargo el laberinto. Verifique el archivo CSV.\n");
            } else {
                printf("Laberinto cargado.\n");
            }
            wait_for_key();
        } else if (choice == 2) { // si quiere jugar
            int num_players = read_valid_option(1, 2, 
                "Cuantos jugadores? (1-2): ",
                "Entrada invalida. Por favor, ingrese 1 o 2.\n");
            
            Player players[2] = {0}; // espacio para dos jugadores
            Queue* turns = queue_create(NULL); // cola para turnos
            for (int i = 0; i < num_players; i++) { // para cada jugador
                initialize_player(&players[i]); // lo preparamos
                queue_insert(turns, &players[i]); // lo ponemos en la cola
            }
            int winner = 0; // para el ganador
            while (running && !winner) { // mientras no haya ganador
                Player* current_player = (Player*)queue_remove(turns); // tomamos el siguiente
                if (!current_player) break; // si no hay, salimos
                int player_id = (current_player == &players[0]) ? 1 : 2; // sabemos quien es
                display_state(scenarios, current_player, player_id); // mostramos su estado

                if (current_player->time_left <= 0) { // si se acabo el tiempo
                    printf("\nTiempo agotado! Has perdido.\n");
                    display_final_scores(players, num_players, 0);
                    wait_for_key();
                    int choice = show_end_menu(); // mostramos el menu final
                    free_resources(NULL, players, num_players); // limpiamos jugadores
                    queue_clean(turns); // limpiamos turnos
                    free(turns); // liberamos turnos
                    if (choice == 1) { // si quiere volver al menu
                        running = 1; // seguimos
                        break; // volvemos al menu
                    }
                    running = 0; // terminamos
                    break;
                }

                int game_choice = read_valid_option(1, 6, 
                    "1. Recoger item\n2. Descartar item\n3. Moverse\n4. Volver al menu principal\n5. Reiniciar partida\n6. Pistas\nOpcion: ",
                    "Entrada invalida. Por favor, ingrese un numero entre 1 y 6.\n");
                
                switch (game_choice) { // segun la eleccion
                    case 1:
                        manage_items(scenarios, current_player, 1); // recoger objetos
                        break;
                    case 2:
                        manage_items(scenarios, current_player, 0); // tirar objetos
                        break;
                    case 3:
                        move(scenarios, current_player, players, num_players, player_id, turns, &running); // moverse
                        if (num_players == 2 && current_player->time_left > 0 && running) { // si son dos jugadores
                            MapPair* pair = map_search(scenarios, &current_player->current_scenario); // buscamos la habitacion
                            if (pair && (((Scenario*)pair->value)->id == 16 || ((Scenario*)pair->value)->is_final)) { // si es el final
                                winner = player_id; // marcamos ganador
                                display_final_scores(players, num_players, player_id);
                                printf("Inventario: ");
                                print_items(current_player->inventory);
                                printf("\nPuntaje: %d\n", current_player->total_score);
                                wait_for_key();
                                int choice = show_end_menu(); // mostramos el menu final
                                free_resources(NULL, players, num_players); // limpiamos jugadores
                                queue_clean(turns); // limpiamos turnos
                                free(turns); // liberamos turnos
                                if (choice == 1) { // si quiere volver al menu
                                    running = 1; // seguimos
                                    break; // volvemos al menu
                                }
                                running = 0; // terminamos
                                break;
                            }
                        }
                        break;
                    case 4: // volver al menu
                        free_resources(NULL, players, num_players); // limpiamos jugadores
                        queue_clean(turns); // limpiamos turnos
                        free(turns); // liberamos turnos
                        printf("Volviendo al menu principal...\n");
                        wait_for_key();
                        running = 1; // seguimos
                        break;
                    case 5: // reiniciar partida
                        free_resources(NULL, players, num_players); // limpiamos jugadores
                        queue_clean(turns); // limpiamos turnos
                        free(turns); // liberamos turnos
                        printf("Volviendo al menu principal...\n");
                        wait_for_key();
                        running = 1; // seguimos
                        break;
                    case 6:
                        show_hints(current_player); // mostramos pistas
                        break;
                }
                if (winner || !running) break; // si hay ganador o terminamos, salimos
                if (current_player->time_left > 0) queue_insert(turns, current_player); // ponemos el jugador en la cola
            }
            free_resources(NULL, players, num_players); // limpiamos jugadores
            if (turns) { // si queda la cola
                queue_clean(turns); // limpiamos
                free(turns); // liberamos
            }
        } else if (choice == 3) { // si quiere salir
            free_resources(scenarios, NULL, 0); // limpiamos el mapa
            free(scenarios); // liberamos el mapa
            running = 0; // terminamos
        } else if (choice == 4) { // si quiere pistas
            limpiarPantalla();
            printf("Inicie una partida para ver las pistas.\n");
            wait_for_key();
        }
    }
    free_resources(scenarios, NULL, 0); // limpiamos el mapa
    free(scenarios); // liberamos el mapa
    return 0; // terminamos todo
}