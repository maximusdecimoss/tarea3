
GraphQuest es un juego interactivo de consola donde exploras un laberinto, recoges objetos valiosos y buscas la salida antes de quedarte sin tiempo. Imagina que eres un aventurero en un castillo encantado: cada habitación tiene tesoros, pero moverte y llevar objetos consume turnos. Tu misión es llegar a la habitación final, que puede ser una sala especial o la habitación número 16, mientras acumulas puntos. 
El juego está diseñado para uno o dos jugadores, quienes toman turnos para moverse, recoger objetos o pedir pistas. Todo el laberinto se carga desde un archivo de texto, lo que hace que cada partida pueda ser única dependiendo de cómo esté configurado.
El juego usa un mapa binario (un grafo) para conectar las habitaciones. Cada habitación puede llevar a otras por caminos hacia arriba, abajo, izquierda o derecha. Llevas una mochila donde guardas objetos, pero estos tienen peso, y cuanto más pesado lleves, más tiempo tardas en moverte. Si se te acaban los turnos, pierdes. Al final, tus puntos determinan si eres un explorador de nivel Oro, Plata o Bronce.

Cómo instalar y jugar

Qué necesitas:

- Un computador con Windows (el juego usa una librería especial para leer teclas).
- Un programa para compilar, como GCC.
- Los archivos del juego: el principal (tarea3.c), otros archivos de apoyo (list.c, map.c, heap.c, queue.c, extra.c), y el archivo del laberinto (graphquest.csv).

(Si usas visual code studio puede clonar el link de este repositorio , compilar y con ./tarea3.exe correr el juego).


Organiza los archivos:

- Pon todo en una carpeta, como C:\Users\TuNombre\tarea3.
- Dentro, crea una subcarpeta TDAs\tdas con los archivos de apoyo.
- Coloca graphquest.csv en la carpeta principal (tarea3).


Compila el juego:

- Abre una terminal (como PowerShell).
- Ve a la carpeta de los archivos de apoyo:cd C:\Users\TuNombre\tarea3\TDAs\tdas


Compila con:gcc tarea3.c list.c map.c heap.c queue.c extra.c -o tarea3.exe


Esto crea un archivo tarea3.exe.


Juega:

- Ejecuta el juego:.\tarea3.exe


- Sigue las instrucciones en pantalla para navegar por el menú y jugar.




¿Cómo se juega?

Al iniciar GraphQuest, ves un menú principal con cuatro opciones:

- Cargar Laberinto: Prepara el juego leyendo el archivo del laberinto (un archivo llamado graphquest.csv).
- Iniciar Partida: Comienzas a jugar, eligiendo si eres uno o dos jugadores.
- Salir: Terminas el juego.
/ Pistas: Intenta mostrarte pistas, pero necesitas estar en una partida para usarlas.

Si eliges "Iniciar Partida", decides cuántos jugadores (1 o 2). Luego, cada jugador comienza en la primera habitación con 10 turnos. En cada turno, ves:

- La descripción de la habitación, como "Una puerta rechinante cruje al entrar".
- Los objetos en el suelo, como una espada o un rubí, con su peso y valor en puntos.
- Tu mochila, que muestra qué llevas.
- Tu peso total, puntaje y turnos restantes.
- Las direcciones posibles: arriba, abajo, izquierda o derecha.

Tienes seis opciones en el juego:

- Recoger un objeto: Tomas algo del suelo, como una espada, y se añade a tu mochila. Esto suma peso y puntos, pero consume un turno.
- Descartar un objeto: Dejas algo de tu mochila en la habitación, reduciendo peso y puntos. También consume un turno.
- Moverte: Usas las teclas w (arriba), s (abajo), a (izquierda) o d (derecha) para ir a otra habitación. Moverte consume turnos según tu peso: más peso, más turnos gastas.
- Volver al menú principal: Sales de la partida y regresas al inicio.
- Reiniciar partida: Vuelves al menú principal, empezando de cero.
- Pistas: Resuelves acertijos matemáticos para obtener rutas sugeridas.

- Si llegas a la habitación final, ganas, y ves tus puntos en una tabla.

- Si se te acaban los turnos, pierdes. En modo de dos jugadores, el primero en llegar al final gana.


Detalles del Juego

* El Laberinto

El laberinto es como un mapa de habitaciones conectadas. Cada habitación tiene:

- Un número único (como 1, 2, 16).
- Un nombre, como "Inicio" o "Pasadizo Oscuro".
- Una descripción que te sumerge en la aventura.
- Objetos en el suelo, como un "Cuchillo (1 kg, 10 puntos)".
- Conexiones a otras habitaciones (o ninguna, si es un callejón sin salida).
- Una marca que indica si es la habitación final.

El juego lee este laberinto desde un archivo de texto (graphquest.csv). Por ejemplo, una habitación puede estar conectada solo hacia abajo, lo que significa que solo puedes moverte con la tecla s.

Objetos y Mochila

- Los objetos tienen un nombre, peso (en kilogramos, como 5.0) y valor (en puntos, como 10). Al recoger un objeto, se suma a tu mochila, aumentando tu peso total y puntaje.
- Si lo descartas, lo dejas en la habitación y pierdes esos puntos. No hay límite de peso, lo que puede ser un problema, ya que llevar mucho peso hace que moverte consuma más turnos.

Turnos y Movimiento

- Empiezas con 15 turnos. Cada acción (recoger, descartar, pedir pista) gasta 1 turno.
- Moverte gasta turnos según tu peso: por cada 10 kg (o fracción), gastas un turno extra.
-  Por ejemplo, si llevas 15 kg, moverte gasta 2 turnos. Esto te obliga a decidir si vale la pena llevar objetos pesados.

Pistas

Las pistas son un sistema de ayuda. Si eliges la opción "Pistas", ves tres niveles de dificultad:

- Oro: Un acertijo difícil, como calcular cuánto vale un tesoro que crece según una fórmula matemática (respuesta: 96). Si aciertas, obtienes una ruta detallada, como "ve al sur, luego al este dos veces".
- Plata: Un acertijo intermedio, como resolver un enigma de números donde el resultado es 35. La ruta es menos directa pero útil.
- Bronce: Un acertijo más simple, con respuesta 23, que da una ruta básica.

Cada intento consume 1 turno, y si no tienes turnos, pierdes. Las pistas están diseñadas para darte una ventaja estratégica, pero resolver los acertijos requiere pensar como matemático.

- El puntaje se deivide en tres nivels Niveles Oro
- Al final del juego, tus puntos determinan tu nivel:

- Oro (más de 70 puntos): Eres un maestro explorador, con muchos objetos valiosos.
- Plata (30 a 70 puntos): Un buen aventurero, balanceando peso y valor.
- Bronce (menos de 30 puntos): Completaste el juego, pero con pocos tesoros.

Estos niveles se inspiran en medallas deportivas, dando un sentido de logro. 

También se usan en las pistas para clasificar las rutas: Oro da el camino más valioso, Plata uno intermedio, y Bronce el más simple. Esto motiva a los jugadores a mejorar su estrategia y competir por el mejor nivel.

Tablas para Resultados

Cuando terminas el juego, ves una tabla con los resultados, algo así:

+---------+---------+--------+

| Jugador | Puntaje | Nivel  |

+---------+---------+--------+

| 1       | 80      | Oro    |

| 2       | 40      | Plata  |

+---------+---------+--------+

La tabla organiza los puntajes de forma clara, mostrando quién jugó, cuántos puntos ganó y su nivel. Usamos este formato porque es fácil de leer en la consola, con líneas y columnas que dan orden, como un cuadro en un cuaderno. Es especialmente útil con dos jugadores, para comparar resultados.
¿Por qué tantos comentarios en el código?
El juego usa ideas avanzadas, como mapas (para encontrar habitaciones rápido), listas (para guardar objetos), pilas (para recordar tu camino) y colas (para turnos entre jugadores). Estas ideas son como un rompecabezas complejo, así que comentamos casi cada línea del código para explicar qué hace y por qué. Por ejemplo:

Explicamos que una habitación se guarda en un mapa para buscarla rápido.
Detallamos cómo moverte usa las teclas w, s, a, d para que sea intuitivo.
Aclaramos por qué guardamos el camino en una pila, para que puedas volver atrás si quieres.

Los comentarios son como un guía turístico: te llevan paso a paso por el código, especialmente porque los mapas y grafos pueden ser confusos. Esto ayuda a cualquiera que quiera entender o mejorar el juego, como un compañero de equipo o un profesor.


Notas importantes:

Asegúrate de que graphquest.csv esté en la carpeta correcta, o el juego no cargará el laberinto.
Guarda todos los archivos en formato de texto simple (UTF-8 sin BOM) para evitar errores.
Si el juego no compila, revisa los errores mencionados más abajo.



Ejemplo de una partida
Imagina que inicias el juego:

Eliges "Iniciar Partida" y seleccionas 1 jugador.
Ves que estás en la habitación "Inicio", con una espada en el suelo (5 kg, 2 puntos). Tienes 10 turnos y puedes ir hacia abajo.
Escoges "Recoger item" y tomas la espada. Ahora tu mochila tiene la espada, tu peso es 5 kg, tu puntaje es 2, y te quedan 9 turnos.
Eliges "Pistas" y seleccionas el nivel Bronce. Resuelves un acertijo (respondes 23) y obtienes una ruta: "Ve al sur, luego al este". Te quedan 8 turnos.
Escoges "Moverse" y presionas s para ir abajo a un "Pasadizo Oscuro". Como llevas 5 kg, gastas 1 turno, quedándote con 7.
Continúas hasta llegar a la habitación final (número 16). Ganas, y ves:¡HAZ GANADO! Eres un explorador indomable
+---------+---------+--------+
| Jugador | Puntaje | Nivel  |
+---------+---------+--------+
| 1       | 2       | Bronce |
+---------+---------+--------+
Jugador 1 ha ganado!


Eliges "Volver al menú principal", pero el juego se cierra (un error conocido).

Problemas conocidos

El menú final no vuelve al menú principal:

Cuando terminas el juego (ganas o pierdes), ves un menú con dos opciones: "Volver al menú principal" o "Salir". Si eliges "Volver al menú principal", el juego se cierra en lugar de regresar al inicio. Esto frustra porque esperas seguir jugando.
Por qué pasa: El programa usa una instrucción que termina todo en lugar de reiniciar el menú.
Cómo arreglarlo: Los desarrolladores podrían cambiar esa instrucción para que regrese al menú principal, como cuando eliges "Reiniciar partida".


Error al buscar la habitación del jugador:

Hay un problema en el programa que impide compilarlo porque usa un símbolo raro (¤) al buscar en qué habitación está el jugador.
Por qué pasa: Parece un error de tipeo en el código.
Cómo arreglarlo: Cambiar ese símbolo por la referencia correcta al jugador actual.


El archivo del laberinto puede causar errores:

El programa lee el laberinto desde graphquest.csv, pero no revisa si el archivo tiene errores, como nombres de objetos muy largos o números inválidos. Esto puede hacer que el juego falle.
Por qué pasa: No hay suficientes comprobaciones para datos incorrectos.
Cómo arreglarlo: Añadir reglas que revisen el archivo antes de usarlo, como limitar la longitud de los nombres o asegurarse de que los números sean válidos.


Puedes llevar peso infinito:

No hay límite para cuántos objetos puedes recoger, lo que te permite acumular peso sin restricción. Esto hace que moverte sea muy lento, gastando muchos turnos.
Por qué pasa: El juego no establece un peso máximo para la mochila.
Cómo arreglarlo: Poner un límite, como 50 kg, y avisar si intentas recoger algo que lo exceda.



¿Por qué este diseño?

Pistas con acertijos: Elegimos acertijos matemáticos para las pistas porque añaden un desafío intelectual, como resolver un enigma en una aventura real. Los niveles Oro, Plata y Bronce hacen que las pistas sean emocionantes, como si desbloquearas secretos de mayor valor.
Tablas para resultados: Mostrar los puntajes en una tabla con bordes es como presentar un trofeo al final. Es claro, ordenado y te hace sentir que tu esfuerzo se reconoce.
Comentarios detallados: Como el juego usa mapas y conexiones complicadas (piensa en un laberinto con flechas entre habitaciones), explicamos cada paso en el código. Esto es como dejar notas en un mapa del tesoro, para que cualquiera pueda seguirlo.
Niveles de puntaje: Oro, Plata y Bronce dan un objetivo claro, como ganar una medalla. Hacen que quieras jugar mejor para alcanzar el nivel más alto.

Qué hacer si algo falla

Si no compila: Revisa el error en la terminal. Si menciona un símbolo raro (¤), necesitarás corregir esa línea en el archivo principal. Pide ayuda a alguien que sepa programar si no estás seguro.
Si el laberinto no carga: Asegúrate de que graphquest.csv esté en la carpeta C:\Users\TuNombre\tarea3. Verifica que tenga el formato correcto, con columnas para ID, nombre, descripción, objetos, conexiones y si es la habitación final.
Si el juego se cierra al volver al menú: Es un error conocido. Por ahora, elige "Salir" y reinicia el juego manualmente.
Si llevas demasiado peso: Sé estratégico y descarta objetos pesados que no valgan muchos puntos.

Conclusión
GraphQuest es una aventura donde tus decisiones importan: ¿llevas esa espada pesada o la dejas? ¿resuelves un acertijo para encontrar el camino? Con su laberinto dinámico, pistas ingeniosas y tabla de resultados, te reta a ser un explorador astuto. Aunque tiene algunos errores, como el menú final que no regresa al inicio, es una base divertida para aprender sobre mapas y estrategias. ¡Prepara tu mochila, carga el laberinto y encuentra la salida!
