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

/*
Comenzaremos con definir nuestras estructuras de datos para el juego , de manera que tengamos 

nuestras habitaciones, objetos y jugadores listos para interactuar y de forma bien organizada
*/

// La primera estructura es para los objetos que el jugador puede recoger.
typedef struct {
    char name[50]; // aqui va el nombre del objeto, maximo 50 letras
    float weight; // el peso, puede tener decimales
    int value; // los puntos que vale
} Item;

// La siguiente estructura representará la habitación del laberinto, con sus detalles
typedef struct {
    int id; // un numero unico para la habitacion
    char name[100]; // el nombre
    char description[300]; // una descripcion larga, 
    int is_final; // 1 si es el final del juego, 0 si no
    int up, down, left, right; // numeros de las habitaciones vecinas, -1 si no hay camino
    List* items; // lista de objetos en la habitacion declarada anteriormente
} Scenario;

// y por ultimo una estructura para los jugadores, la cual guardara todo lo que llevan
typedef struct {
    int current_scenario; // en que habitacion esta
    List* inventory; // su mochila con objetos
    float total_weight; // cuanto pesan todos sus objetos
    int total_score; // sus puntos totales
    int time_left; // turnos que le quedan
    Stack* path; // una pila con el camino que ha seguido
} Player;

// Aqui procederemos a declarar todos los prototipos de las funciones que usaremos mas adelante
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
void move(Map* scenarios, Player* p, Player* players, int num_players, int player_id);
void show_hints(Player* p);
void free_resources(Map* scenarios, Player* players, int num_players);

/*
La siguiente funcion se encargara de comparar dos numeros y ver si son iguales
 */
int int_equal(void* key1, void* key2) {
    return *(int*)key1 == *(int*)key2;
}

/*
 * esta funcion la hicimos para mostrar los objetos que hay en una lista, en este caso seria los items que se encuentran en el suelo
 */
void print_items(List* items) {
    if (!list_size(items)) { // si no hay nada en la lista
        printf("Ninguno");
        return; // nos salimos
    }
    void* ptr = list_first(items); //  luego tomamos el primer objeto
    while (ptr) { // y mientras haya items en el suelo
        Item* item = (Item*)ptr; // lo convertimos a objeto
        printf("%s (%.1f kg, %d pts)", item->name, item->weight, item->value);
        ptr = list_next(items); // pasamos al siguiente
        if (ptr) printf(", "); // ponemos una coma si hay mas para mostrarlos de forma ordenada.
    }
}

/*
esta funcion hace que el juego espere a que el jugador presione una tecla, se crea para simular como una especie de pausa hasta que se presione el teclado
 */
void wait_for_key() {
    printf("Presiona una tecla para continuar...\n");
    getch(); // Consumir una tecla
    while (kbhit()) getch(); // Limpiar cualquier tecla adicional en el buffer // limpiamos teclas extras
}

/*
la siguiente funcion se hizo con la finalidad de pedir un numero y asegurarnos que sea valido, esto para que el jugador no ingrese un numero fuera de rango o algo que no sea un numero
 */
int read_valid_option(int min, int max, const char* prompt, const char* error_msg) {
    char buffer[100]; // primero creamos un buffer (lo que llamamos espacio de memoria) para lo que escribe el jugador
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
ahora esta funcion se encargara de que el jugador vea el menu final y elija entre salir del juego o volver al menu principal
 */
int show_end_menu() {
    return read_valid_option(1, 2, 
        "\n=== Fin del Juego ===\n1. Volver al menu principal\n2. Salir\nOpcion: ",
        "Opcion invalida. Por favor, ingrese 1 o 2.\n");
}

/*
ya teniendo las funciones mas generales, creamos la funcion encargadda de la lectura del archivo , el cual es en principio un data frame que contiene la estrcutura del laberitno que usaremos para el juego
 */
void load_graph_from_csv(Map* scenarios, int* scenario_count) {
    FILE* archivo = fopen("graphquest.csv", "r"); // aqui primero abrimos el archivo
    if (!archivo) { // si no se pudo abrir
        printf("ERROR: No se pudo abrir el archivo graphquest.csv\n");
        return; // nos salimos
    }

    char** campos;
    fgets(malloc(500), 500, archivo); // Saltar encabezado CSV // luego creamos un espacio para leer una linea // si no se pudo leer la primera linea 

    while ((campos = leer_linea_csv(archivo, ',')) != NULL) { // leemos cada linea
        Scenario* escenario = (Scenario*)malloc(sizeof(Scenario)); // creamos una habitacion
        escenario->id = atoi(campos[0]); // guardamos su numero
        strcpy(escenario->name, campos[1]); // copiamos el nombre, maximo con un maximo de 99 letras(esto se puede adaptar a gusto del que lo quiera modificar)
        strcpy(escenario->description, campos[2]); // copiamos la descripcion
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
            int peso = atoi(list_next(valores)); // tomamos el peso, con decimales

            Item* item = (Item*)malloc(sizeof(Item)); // creamos un objeto
            strcpy(item->name, nombre); // copiamos el nombre, maximo 49 letras
            item->value = valor; // guardamos el valor
            item->weight = peso; // guardamos el peso

            list_pushBack(escenario->items, item); // ponemos el objeto en la habitacion

            list_clean(valores); // limpiamos la lista de valores
            free(valores); // liberamos la memoria
        }

        list_clean(items_csv); // limpiamos la lista de objetos
        free(items_csv); // liberamos memoria

        map_insert(scenarios, &escenario->id, escenario); // luego guardamos la habitacion en el mapa
        (*scenario_count)++; // contamos una habitacion mas si se cargo bien y printeamos un mensaje que indica que se cargo

        printf("Escenario ID %d cargado: %s\n", escenario->id, escenario->name);
    }

    fclose(archivo); // cerramos el archivo
}

/*
esta funcion se encargara de preparar a un nuevo jugador, es decir, se
inicializa su mochila, su puntaje, el tiempo que le queda y la habitacion en la que empieza
 */
void initialize_player(Player* p) {
    p->current_scenario = 1; // empezamos en la habitacion 1
    p->inventory = list_create(); // creamos una mochila vacia
    p->total_weight = 0.0; // la mochila no pesa nada
    p->total_score = 0; // tiene 0 puntos
    p->time_left = 10; // le damos 10 turnos
    p->path = stack_create(NULL); // creamos una pila para su camino ya que el jugador puede volver a habitaciones anteriores
    int* id = malloc(sizeof(int)); // creamos un numero ya que la pila guarda numeros en vez de objetos siendo es un poco mas eficiente en este caso
    *id = 1; // ponemos la habitacion 1
    stack_push(p->path, id); // guardamos la habitacion
}

/*
la siguiente funcion hace que el jugador vea donde esta y que lleva
es como mostrarle un mapa de la habitacion y su mochila
 */
void display_state(Map* scenarios, Player* p, int player_id) {
    limpiarPantalla(); //primero limpiamos la pantalla
    MapPair* pair = map_search(scenarios, &p->current_scenario); //aqui  buscamos el escenario actual del jugador en el mapa de escenarios
    if (!pair) { // si no la encontramos
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return; // nos salimos
    }
    Scenario* s = (Scenario*)pair->value; //Obtenemos el puntero al escenario actual desde el MapPair

    printf("\n=== Jugador %d ===\n", player_id); // Muestra el número de jugador
    printf("Escenario: %s\n", s->description); // Muestra la descripción del escenario actual

    printf("Elementos: "); // Encabezado para los elementos del escenario
    print_items(s->items); // Imprime los ítems disponibles en el escenario

    printf("\nTiempo restante: %d\n", p->time_left); // Muestra el tiempo restante del jugador

    printf("Inventario: "); // Encabezado del inventario del jugador
    print_items(p->inventory); // Muestra los ítems en el inventario del jugador

    printf("\nPeso total: %.1f kg, Puntaje: %d\n", p->total_weight, p->total_score); // Muestra el peso acumulado y el puntaje del jugador

    printf("Direcciones: "); // Encabezado para las direcciones disponibles
    if (s->up != -1) printf("Arriba "); // Muestra dirección arriba si es válida
    if (s->down != -1) printf("Abajo "); // Muestra dirección abajo si es válida
    if (s->left != -1) printf("Izquierda "); // Muestra dirección izquierda si es válida
    if (s->right != -1) printf("Derecha "); // Muestra dirección derecha si es válida
    printf("\n"); // Salto de línea final
}

/*
esta funcionse encargara de mostrar los objetos que hay en el escenario y en la mochila , dando opcon al jugador de recoger o tirar objetos
 */
void manage_items(Map* scenarios, Player* p, int collect) {
    // primero buscamos el escenario actual del jugador en el mapa
    MapPair* pair = map_search(scenarios, &p->current_scenario); 
    if (!pair) return; // si no lo encontramos, no seguimos porque no hay donde interactuar

    // sacamos el escenario desde el MapPair y lo casteamos (aqui cabe destacar que el value viene como void*)
    Scenario* s = (Scenario*)pair->value; 

    // si collect es 1 vamos a tomar cosas del escenario, si es 0 soltamos cosas al escenario
    List* source = collect ? s->items : p->inventory; 

    // si la lista está vacía (no hay nada que recoger o soltar), se lo avisamos al jugador
    if (!list_size(source)) {
        printf("%s vacio.\n", collect ? "Escenario" : "Inventario");
        wait_for_key(); // esperamos que presione una tecla para que vea el mensaje
        return;
    }

    // mostramos los items que puede elegir
    printf("%s:\n", collect ? "Items disponibles" : "Inventario");

    // empezamos desde el primer item de la lista (list_first devuelve un void*, así que necesitamos castearlo después)
    void* ptr = list_first(source);
    int i = 1; // numerador para mostrar opciones tipo 1, 2, 3...
    List* indices = list_create(); // esta de aqui es una lista auxiliar donde guardamos los números que mostramos, para validar opciones

    while (ptr) {
        Item* item = (Item*)ptr; // casteamos el void* a Item* para poder acceder a nombre, peso y valor
        printf("%d. %s (%.1f kg, %d pts)\n", i, item->name, item->weight, item->value); // mostramos info del item

        int* index = malloc(sizeof(int)); // reservamos memoria para guardar ese número
        *index = i++; // le damos valor y lo incrementamos para el próximo
        list_pushBack(indices, index); // lo metemos en la lista de indices

        ptr = list_next(source); // pasamos al siguiente ítem
    }

    int choice; // aqui vamos a guardar la opción del jugador

    do {
        // pedimos al jugador que elija un item (0 para salir). Validamos que elija bien.
        choice = read_valid_option(0, list_size(indices), 
            "Seleccione item (0 para terminar): ",
            "Seleccion invalida. Por favor, ingrese un numero valido.\n");

        if (choice == 0) break; // si elige 0, termina la acción

        ptr = list_first(source); // volvemos al inicio de la lista
        for (i = 1; i < choice && ptr; i++) ptr = list_next(source); // avanzamos hasta el item elegido

        if (ptr) {
            Item* item = (Item*)ptr; // casteamos el puntero al tipo correcto

            if (collect) {
                // si estamos recogiendo un item, lo duplicamos porque el original se va a eliminar
                Item* new_item = (Item*)malloc(sizeof(Item)); // pedimos espacio para el nuevo item

                // copiamos los datos manualmente. No usamos el mismo puntero porque lo vamos a liberar más abajo
                strcpy(new_item->name, item->name); 
                new_item->weight = item->weight;
                new_item->value = item->value;

                list_pushBack(p->inventory, new_item); // lo metemos en la mochila
                list_popCurrent(source); // lo sacamos del escenario
                free(item); // liberamos la memoria del item original

                // actualizamos peso y puntaje del jugador
                p->total_weight += new_item->weight;
                p->total_score += new_item->value;
            } else {
                // si estamos soltando un item, simplemente lo sacamos del inventario
                list_popCurrent(source); 
                p->total_weight -= item->weight;
                p->total_score -= item->value;
                free(item); // liberamos el item porque ya no lo necesitamos
            }
        }

    } while (choice > 0); // seguimos preguntando hasta que el jugador diga que no quiere más

    // al final liberamos la lista de índices auxiliares
    list_clean(indices);
    free(indices);

    p->time_left--; // cada acción (recoger o soltar) consume 1 unidad de tiempo
    wait_for_key(); // pausa para que el jugador pueda ver lo que pasó antes de continuar
}

/*
esta funcion hace que veamos los puntajes al final , es una tabla que mostrara de forma ordenada  los resultados del juego, aginando nombres, puntajes y niveles.
 */
void display_final_scores(Player* players, int num_players, int winner_id) {
    printf("\n¡Luz al final del túnel! Pero cuidado... hay salidas que son más trampas que finales felices.\n"); // mensaje final extraido del data frame.
    printf("+---------+---------+--------+\n"); // parte de arriba de la tabla
    printf("| Jugador | Puntaje | Nivel  |\n"); // encabezados
    printf("+---------+---------+--------+\n");

    for (int i = 0; i < num_players; i++) { // recorremos cada jugador
        const char* level; // para guardar el nivel tipo "Oro", "Plata", "Bronce"

        if (players[i].total_score > 70) level = "Oro"; // si hizo más de 70 pts, es oro
        else if (players[i].total_score >= 30) level = "Plata"; // si hizo entre 30 y 70, plata
        else level = "Bronce"; // menos de 30, bronce
        printf("| %d       | %d      | %s    |\n", i + 1, players[i].total_score, level); // mostramos todo bien ordenadito al final.
    }

    printf("+---------+---------+--------+\n"); 

    if (winner_id != 0) { // si hay un ID de ganador distinto de 0
        printf("Jugador %d ha ganado!\n", winner_id); // lo celebramos
    } else { // si no hay ganador (por tiempo o lo que sea)
        printf("Tiempo agotado. Nadie ha ganado.\n"); // imprimimos un menasje que lo indique
    }
}

/*
la sigueinte funcion la hicimos para mover al jugador por el laberinto
aginadno las teclas w, s, a, d para arriba, abajo, izquierda y derecha respectivamente, de forma que sea mas intuitivo para el jugador.
 */
void move(Map* scenarios, Player* p, Player* players, int num_players, int player_id) {
    // primero buscamos el escenario actual del jugador en el mapa (usamos un puntero a MapPair porque map_search devuelve eso)
    MapPair* pair = map_search(scenarios, &p->current_scenario); 
    if (!pair) { // si no encontramos el escenario actual, algo anda mal
        printf("Error: Escenario ID %d no encontrado.\n", p->current_scenario);
        return; //entonces no hacemos nada
    }

    // accedemos a la habitación desde el par (value es un void*, así que lo casteamos a Scenario*)
    Scenario* s = (Scenario*)pair->value;
    int valid_move = 0; // usamos esta bandera para saber si el movimiento fue válido

    do {
        // mostramos las opciones de movimiento al jugador
        printf("Presione: w (Arriba), s (Abajo), a (Izquierda), d (Derecha), cualquier otra tecla para cancelar\n");
        char key = getch(); // leemos la tecla
        int new_scenario = -1; // inicializamos el nuevo escenario a -1 (no hay camino)
        char* direction = NULL; //creamos esta variable para mostrar un mensaje si no hay camino

        // determinamos la dirección según la tecla presionada
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
                // si presiona otra cosa, se cancela el movimiento
                printf("\nMovimiento cancelado.\n");
                wait_for_key();
                return;
        }

        if (new_scenario != -1) {
            // calculamos cuántos turnos cuesta moverse, dependiendo del peso total
            int time_cost = (int)ceil((p->total_weight + 1) / 10.0);
            
            if (p->time_left <= time_cost) {
                // si no tiene tiempo suficiente, se acaba el juego para él
                printf("\nTiempo agotado! Has perdido.\n");
                display_final_scores(players, num_players, 0); // esto indica que no hay ganador
                wait_for_key(); // pausa para que el jugador pueda ver el mensaje

                // mostramos el menú final y terminamos el programa
                int choice = show_end_menu();
                if (choice == 1) exit(0);
                return;
            }

            // buscamos la nueva habitación en el mapa (otro puntero a MapPair)
            MapPair* new_pair = map_search(scenarios, &new_scenario);
            if (!new_pair) {
                printf("Error: Nuevo escenario ID %d no encontrado.\n", new_scenario);
                return;
            }

            // movemos al jugador al nuevo escenario
            p->current_scenario = new_scenario;
            p->time_left -= time_cost;

            // aquí usamos malloc para guardar el camino recorrido (memoria dinámica para un entero)
            int* id = malloc(sizeof(int)); 
            *id = new_scenario;
            stack_push(p->path, id); // agregamos al stack el nuevo escenario visitado

            // accedemos a los datos del nuevo escenario
            Scenario* new_s = (Scenario*)new_pair->value;
            valid_move = 1;

            // si llegamos al escenario final (por ID o bandera), el jugador gana
            if (new_scenario == 16 || new_s->is_final) {
                printf("\n+----------------------------------+\n");
                printf("| ¡HAZ GANADO!                     |\n");
                printf("| Eres un explorador indomable     |\n");
                printf("+----------------------------------+\n");
                printf("Presiona una tecla para ver los resultados...\n");
                wait_for_key();

                // mostramos los puntajes finales e inventario
                display_final_scores(players, num_players, player_id);
                printf("Inventario: ");
                print_items(p->inventory);
                printf("\nPuntaje: %d\n", p->total_score);
                wait_for_key();

                // menú final otra vez
                int choice = show_end_menu();
                if (choice == 1) exit(0);
                return;
            }
        } else {
            // si no hay camino en esa dirección, avisamos
            printf("\nNo hay camino hacia %s. Intenta de nuevo.\n", direction);
            wait_for_key();
        }
    } while (!valid_move); // aqui esta condicion nos permitira repetir la funcion hasta lograr un movimiento válido
}

/*
Esta función proporciona pistas al jugador para ayudarlo a resolver el laberinto, ofreciendo acertijos matemáticos que desbloquean caminos específicos si se resuelven correctamente.
 */
void show_hints(Player* p) {
    limpiarPantalla(); // Limpiamos la pantalla para mostrar solo la información relevante
    printf("\n=== Pistas para la Gloria ===\n"); // Título de la sección de pistas
    printf("Resuelve el puntaje de cada nivel para desbloquear su camino.\n"); // Explicamos al jugador que debe resolver acertijos
    printf("Cada intento consume 1 unidad de tiempo. Tiempo actual: %d\n\n", p->time_left); // Mostramos el tiempo restante

    int choice; // Variable para almacenar la opción elegida por el jugador
    do {
        // Pedimos al jugador que seleccione un nivel de dificultad (Oro, Plata, Bronce) o salir
        choice = read_valid_option(0, 3, 
            "Seleccione nivel (1: Oro, 2: Plata, 3: Bronce, 0: Salir): ",
            "Opcion invalida. Selecciona 1, 2, 3 o 0.\n");

        if (choice == 0) { // Si elige 0, salimos de la función
            wait_for_key(); // Esperamos una tecla antes de volver
            return; // Terminamos la función
        }

        // Verificamos si el jugador tiene tiempo suficiente
        if (p->time_left <= 0) { // Si no le queda tiempo
            printf("\n¡Tiempo agotado! Has perdido.\n"); // Avisamos que perdió
            wait_for_key(); // Pausamos
            exit(0); // Terminamos el programa
        }

        // Según el nivel elegido, mostramos el acertijo correspondiente
        switch (choice) {
            case 1: // Nivel Oro
                printf("\nNivel Oro:\n");
                printf("Un tesoro crece con el cuadrado de tu camino, cuatro veces y media su peso, desde el inicio hasta cuatro pasos. Halla su valor.\n");
                printf("(Resuelve: integral de 0 a 4 de (9/2)x^2 dx)\n"); // Acertijo matemático
                break;
            case 2: // Nivel Plata
                printf("\nNivel Plata:\n");
                printf("Un camino plateado guarda un secreto cuadrado: su valor menos diez veces él, más veinticinco, es nada. Toma el número mayor y multiplícalo por siete.\n");
                printf("(Resuelve: x^2 - 10x + 25 = 0, toma la raíz positiva, multiplica por 7)\n"); // Acertijo cuadrático
                break;
            case 3: // Nivel Bronce
                printf("\nNivel Bronce:\n");
                printf("El bronce mide el cambio de un peso que crece como once y medio por el cuadrado de un paso. Encuentra su ritmo en el primer paso.\n");
                printf("(Deriva f(x) = 11.5x^2, evalúa en x = 1)\n"); // Acertijo de derivada
                break;
        }

        // Pedimos al jugador que ingrese la solución al acertijo
        int score = read_valid_option(INT_MIN, INT_MAX, 
            "Ingresa el puntaje: ",
            "Entrada invalida. Por favor, ingrese un numero valido.\n");

        // Verificamos si la solución es correcta según el nivel
        switch (choice) {
            case 1: // Nivel Oro
                if (score == 96) { // Solución correcta: integral = 96
                    printf("\n¡Correcto! El camino al Oro:\n"); // Mostramos la pista
                    printf("Hacia el sur da un paso, sigue la corriente.\n");
                    printf("Donde el sol sale, dos pasos has de dar,\n");
                    printf("Al sur otra vez, no dejes de avanzar.\n");
                    printf("Al poniente un paso, donde el sol se va,\n");
                    printf("Sur de nuevo, y al final, la gloria hallarás.\n");
                    printf("Dos pasos al oriente, el camino cerrarás.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n"); // Solución incorrecta
                }
                break;
            case 2: // Nivel Plata
                if (score == 35) { // Solución correcta: x = 5, 5 * 7 = 35
                    printf("\n¡Correcto! El camino a la Plata:\n"); // Mostramos la ruta
                    printf("Por la senda plateada, tres pasos al sur,\n");
                    printf("Donde las sombras caen, sin temor ni dolor.\n");
                    printf("Luego al oriente, tres pasos contarás,\n");
                    printf("La plata te espera, si el rumbo no errarás.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n"); // Solución incorrecta
                }
                break;
            case 3: // Nivel Bronce
                if (score == 23) { // Solución correcta: f'(x) = 23x, f'(1) = 23
                    printf("\n¡Correcto! El camino al Bronce:\n"); // Mostramos la ruta
                    printf("Por el bronce comienza, un paso al sur irás,\n");
                    printf("Al oriente un paso, donde el alba verás.\n");
                    printf("Dos pasos al sur, en la sombra avanzar,\n");
                    printf("Dos al oriente, el bronce a reclamar.\n");
                } else {
                    printf("Puntaje incorrecto. Intenta de nuevo o selecciona otro nivel.\n"); // Solución incorrecta
                }
                break;
        }

        p->time_left--; // Cada intento consume 1 unidad de tiempo
        printf("\nTiempo restante: %d\n", p->time_left); // Mostramos el tiempo actualizado
        wait_for_key(); // Pausamos antes de continuar
    } while (choice != 0); // Repetimos hasta que el jugador elija salir
}

/*
Esta función se encarga de liberar toda la memoria dinámica utilizada durante el juego, asegurando que no queden fugas de memoria al finalizar.
 */
void free_resources(Map* scenarios, Player* players, int num_players) {
    // Liberamos los escenarios del mapa si existe
    if (scenarios) {
        MapPair* pair = map_first(scenarios); // Obtenemos el primer par clave-valor
        while (pair) { // Recorremos cada escenario en el mapa
            Scenario* s = (Scenario*)pair->value; // Extraemos el escenario

            // Liberamos los ítems del escenario
            if (s->items) {
                void* item = list_first(s->items); // Tomamos el primer ítem
                while (item) { // Mientras haya ítems
                    free(item); // Liberamos la memoria del ítem
                    item = list_next(s->items); // Pasamos al siguiente
                }
                list_clean(s->items); // Vaciamos la lista
                free(s->items); // Liberamos la lista
            }

            free(pair->key); // Liberamos la clave del escenario (ID)
            free(s); // Liberamos la estructura del escenario
            pair = map_next(scenarios); // Avanzamos al siguiente par
        }
        map_clean(scenarios); // Limpiamos el mapa completo
    }

    // Liberamos los recursos de los jugadores si existen
    if (players && num_players > 0) { // Verificamos que hay jugadores
        for (int i = 0; i < num_players; i++) { // Recorremos cada jugador
            // Liberamos los ítems del inventario
            if (players[i].inventory) { // Si tiene inventario
                void* item = list_first(players[i].inventory); // Tomamos el primer ítem
                while (item) { // Mientras haya ítems
                    free(item); // Liberamos el ítem
                    item = list_next(players[i].inventory); // Avanzamos
                }
                list_clean(players[i].inventory); // Vaciamos la lista
                free(players[i].inventory); // Liberamos la lista
                players[i].inventory = NULL; // Marcamos como NULL para evitar accesos inválidos
            }

            // Liberamos el camino (pila de escenarios visitados)
            if (players[i].path) { // Si tiene un camino
                void* id = stack_pop(players[i].path); // Sacamos el primer ID
                while (id) { // Mientras haya IDs
                    free(id); // Liberamos el ID
                    id = stack_pop(players[i].path); // Sacamos el siguiente
                }
                stack_clean(players[i].path); // Vaciamos la pila
                free(players[i].path); // Liberamos la pila
                players[i].path = NULL; // Marcamos como NULL
            }
        }
    }
}

/*
La función principal es el corazón del juego, encargada de inicializar el laberinto, gestionar el menú principal y coordinar las acciones del jugador.
 */
int main() {
    // Creamos un mapa para almacenar los escenarios del laberinto
    Map* scenarios = map_create(int_equal); // Usamos la función de comparación int_equal
    int scenario_count = 0; // Contador de escenarios cargados
    int running = 1; // Bandera para controlar el ciclo principal del juego (1 = activo, 0 = terminado)

    // Cargamos el laberinto desde el archivo CSV
    load_graph_from_csv(scenarios, &scenario_count);
    if (!scenario_count) { // Si no se cargaron escenarios
        printf("No se cargó el laberinto. Verifique el archivo CSV.\n"); // Avisamos al jugador
        free_resources(scenarios, NULL, 0); // Liberamos el mapa
        free(scenarios); // Liberamos la estructura del mapa
        return 1; // Terminamos con error
    }

    // Ciclo principal del juego
    while (running) {
        // Mostramos el menú principal y obtenemos la opción del jugador
        int choice = read_valid_option(1, 4, 
            "=== GraphQuest ===\n1. Cargar Laberinto\n2. Iniciar Partida\n3. Salir\n4. Pistas\nOpcion: ",
            "Entrada invalida. Por favor, ingrese un numero entre 1 y 4.\n");

        if (choice == 1) { // Opción 1: Cargar un nuevo laberinto
            printf("Laberinto cargado.\n"); // Confirmamos que el laberinto ya está cargado
            wait_for_key(); // Pausamos para que el jugador vea el mensaje
        } else if (choice == 2) { // Opción 2: Iniciar una partida
            // Preguntamos cuántos jugadores participarán (1 o 2)
            int num_players = read_valid_option(1, 2, 
                "Cuantos jugadores? (1-2): ",
                "Entrada invalida. Por favor, ingrese 1 o 2.\n");

            // Inicializamos los jugadores (máximo 2)
            Player players[2] = {0}; // Arreglo de jugadores, inicializado en 0
            Queue* turns = queue_create(NULL); // Creamos una cola para gestionar turnos
            for (int i = 0; i < num_players; i++) { // Para cada jugador
                initialize_player(&players[i]); // Configuramos su estado inicial
                queue_insert(turns, &players[i]); // Lo añadimos a la cola de turnos
            }

            int winner = 0; // Bandera para indicar si hay un ganador
            // Ciclo de juego para los turnos de los jugadores
            while (1) {
                Player* current_player = (Player*)queue_remove(turns); // Sacamos el jugador actual
                if (!current_player) break; // Si no hay jugador, salimos
                int player_id = (current_player == &players[0]) ? 1 : 2; // Determinamos su ID (1 o 2)

                // Mostramos el estado actual del jugador
                display_state(scenarios, current_player, player_id);

                // Mostramos las opciones de juego
                int game_choice = read_valid_option(1, 6, 
                    "1. Recoger item\n2. Descartar item\n3. Moverse\n4. Volver al menu principal\n5. Reiniciar partida\n6. Pistas\nOpcion: ",
                    "Entrada invalida. Por favor, ingrese un numero entre 1 y 6.\n");

                // Ejecutamos la acción elegida
                switch (game_choice) {
                    case 1: // Recoger un ítem
                        manage_items(scenarios, current_player, 1);
                        break;
                    case 2: // Descartar un ítem
                        manage_items(scenarios, current_player, 0);
                        break;
                    case 3: // Moverse a otra habitación
                        move(scenarios, current_player, players, num_players, player_id);
                        // Verificamos si el jugador llegó al final (solo en modo multijugador)
                        if (num_players == 2 && current_player->time_left > 0) {
                            MapPair* pair = map_search(scenarios, &current_player->current_scenario);
                            if (pair && (((Scenario*)pair->value)->id == 16 || ((Scenario*)pair->value)->is_final)) {
                                winner = player_id; // Marcamos al ganador
                                display_final_scores(players, num_players, player_id); // Mostramos puntajes
                                printf("Inventario: ");
                                print_items(current_player->inventory); // Mostramos su inventario
                                printf("\nPuntaje: %d\n", current_player->total_score); // Mostramos su puntaje
                                wait_for_key(); // Pausamos
                                int choice = show_end_menu(); // Mostramos menú final
                                if (choice == 1) { // Si elige volver al menú principal
                                    queue_clean(turns);
                                    free(turns); // Liberamos la cola
                                    free_resources(NULL, players, num_players); // Liberamos jugadores
                                    continue; // Volvemos al menú principal
                                }
                                queue_clean(turns);
                                free(turns); // Liberamos la cola
                                free_resources(NULL, players, num_players); // Liberamos jugadores
                                free_resources(scenarios, NULL, 0); // Liberamos escenarios
                                free(scenarios); // Liberamos el mapa
                                running = 0; // Terminamos el juego
                                break;
                            }
                        }
                        break;
                    case 4: // Volver al menú principal
                        free_resources(NULL, players, num_players); // Liberamos jugadores
                        queue_clean(turns);
                        free(turns); // Liberamos la cola
                        winner = 0; // Reseteamos el ganador
                        printf("Volviendo al menu principal...\n"); // Avisamos
                        wait_for_key(); // Pausamos
                        goto restart_game; // Saltamos al inicio del ciclo principal
                    case 5: // Reiniciar partida
                        free_resources(NULL, players, num_players); // Liberamos jugadores
                        queue_clean(turns);
                        free(turns); // Liberamos la cola
                        winner = 0; // Reseteamos el ganador
                        printf("Volviendo al menu principal...\n"); // Avisamos
                        wait_for_key(); // Pausamos
                        goto restart_game; // Saltamos al inicio del ciclo principal
                    case 6: // Ver pistas
                        show_hints(current_player); // Mostramos las pistas
                        break;
                }

                if (winner || !running) break; // Si hay ganador o el juego terminó, salimos
                if (current_player->time_left > 0) queue_insert(turns, current_player); // Reinsertamos al jugador si tiene tiempo
            }
restart_game:
            continue; // Volvemos al menú principal
        } else if (choice == 3) { // Opción 3: Salir del juego
            free_resources(scenarios, NULL, 0); // Liberamos los escenarios
            free(scenarios); // Liberamos el mapa
            running = 0; // Terminamos el juego
        } else if (choice == 4) { // Opción 4: Ver pistas sin partida
            limpiarPantalla(); // Limpiamos la pantalla
            printf("Inicie una partida para ver las pistas.\n"); // Avisamos que debe iniciar una partida
            wait_for_key(); // Pausamos
        }
    }

    // Liberamos los recursos finales antes de terminar
    free_resources(scenarios, NULL, 0);
    free(scenarios); // Liberamos el mapa
    return 0; // Terminamos el programa
}