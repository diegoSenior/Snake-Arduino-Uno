// Proyecto: Juego de la Viborita (Snake) con Arduino y Pantalla OLED
// Autor: [Diego Rangel]

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuración de mi Pantalla ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
// Inicializo el objeto de la pantalla
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Mis Conexiones de Hardware ---
// Pines donde conecté el Joystick
#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2 // El botón del joystick (pulsador)

// --- Parámetros del Juego ---
#define SNAKE_BLOCK_SIZE 4   // Tamaño de cada cuadrito de la serpiente
#define MAX_SNAKE_LENGTH 64  // Longitud máxima que puede alcanzar
#define HEADER_HEIGHT 10     // Espacio arriba para el puntaje

// Uso un Enum para manejar las direcciones más fácilmente
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction currentDirection = RIGHT; // Empiezo moviéndome a la derecha

// Arrays para guardar las posiciones (X, Y) de cada parte del cuerpo
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 3; // Tamaño inicial

// Variables generales
int foodX, foodY;      // Posición de la comida
int score = 0;         // Mi puntaje actual
bool isPaused = false;
bool isGameRunning = false; // Bandera para saber si estoy en el menú o jugando
bool buttonState = HIGH;    // Para leer el botón sin rebotes
bool justAte = false;       // Bandera para hacer el efecto visual al comer
unsigned long startTime;    // Para contar el tiempo jugado

// --- Mis Gráficos (Sprites) ---
// Dibujo de la comida en formato bitmap (es una pequeña fruta de 4x4)
// Los 1s son pixeles encendidos y los 0s apagados.
const unsigned char PROGMEM foodBitmap[] = {
  0b00100000, // Tallo
  0b01110000, // Parte de arriba
  0b11110000, // Centro
  0b01100000  // Parte de abajo
};

// --- Mis Funciones Auxiliares ---

// Función para poner la comida en una posición aleatoria
void spawnFood() {
  // Calculo cuántas celdas caben en la pantalla
  int gridWidth = SCREEN_WIDTH / SNAKE_BLOCK_SIZE;
  int gridHeight = (SCREEN_HEIGHT - HEADER_HEIGHT) / SNAKE_BLOCK_SIZE;
  
  // Genero coordenadas al azar dentro de la cuadrícula
  foodX = random(0, gridWidth) * SNAKE_BLOCK_SIZE;
  foodY = HEADER_HEIGHT + random(0, gridHeight) * SNAKE_BLOCK_SIZE;
}

// Función para reiniciar todo cuando empiezo una nueva partida
void resetGame() {
  snakeLength = 3; // Reinicio tamaño
  
  // Coloco a la serpiente en su posición inicial
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = 20 - i * SNAKE_BLOCK_SIZE;
    snakeY[i] = HEADER_HEIGHT + SNAKE_BLOCK_SIZE;
  }
  
  score = 0;
  currentDirection = RIGHT;
  isPaused = false;
  spawnFood();          // Genero la primera comida
  startTime = millis(); // Reinicio el cronómetro
}

// --- Funciones para Dibujar en Pantalla ---

// Pantalla de bienvenida (Menú Principal)
void displayReady() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 0); 
  display.println("SNAKE"); // Título grande
  
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Click para Jugar"); // Instrucciones
  display.display();
}

// Pantalla de Pausa
void displayPause() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 8); 
  display.println("PAUSA");
  display.display();
}

// Pantalla de Fin de Juego (Game Over)
void gameOver() {
  // Efecto visual: Hago parpadear la pantalla 3 veces
  for(int i=0; i<3; i++) {
    display.invertDisplay(true);  // Invierto colores
    delay(100);
    display.invertDisplay(false); // Regreso a normal
    delay(100);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 5);   
  display.println("FIN DEL JUEGO");
  
  display.setCursor(35, 20);  
  display.print("Puntaje: ");
  display.println(score); // Muestro cuánto logré
  
  display.display();
  delay(3000); // Espero 3 segundos para que se vea el puntaje
  
  isGameRunning = false; // Regreso al menú principal
}

// --- Configuración Inicial (Setup) ---
void setup() {
  pinMode(JOY_SW, INPUT_PULLUP); // Configuro el botón con resistencia interna
  randomSeed(analogRead(0));     // Inicializo el generador de números aleatorios
  
  // Intento iniciar la pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Si falla, me quedo aquí trabado
  }
  
  display.clearDisplay();
  display.display();
  isGameRunning = false; // Arranco en el menú, no jugando
}

// --- Bucle Principal (Loop) ---
void loop() {
  // 1. Lectura del Botón del Joystick
  bool currentButtonState = digitalRead(JOY_SW);
  
  // Detecto si presioné el botón (cambio de HIGH a LOW)
  if (buttonState == HIGH && currentButtonState == LOW) {
    if (!isGameRunning) {
      // Si estaba en el menú -> Empiezo el juego
      resetGame();
      isGameRunning = true;
    } else {
      // Si estaba jugando -> Pongo o quito pausa
      isPaused = !isPaused;
    }
    delay(200); // Pequeña espera para evitar rebotes del botón
  }
  buttonState = currentButtonState;

  // 2. Control de Estados del Juego
  if (!isGameRunning) {
    displayReady(); // Muestro el menú
    return;
  }

  if (isPaused) {
    displayPause(); // Muestro pausa
    return;
  }

  // 3. Lógica de Movimiento (Joystick)
  int xVal = analogRead(JOY_X);
  int yVal = analogRead(JOY_Y);
  
  // Cambio la dirección según hacia dónde mueva la palanca
  // Evito que pueda regresar en la dirección opuesta (ej. ir arriba si voy abajo)
  if (xVal < 400 && currentDirection != RIGHT) currentDirection = LEFT;
  else if (xVal > 600 && currentDirection != LEFT) currentDirection = RIGHT;
  else if (yVal < 400 && currentDirection != DOWN) currentDirection = UP;
  else if (yVal > 600 && currentDirection != UP) currentDirection = DOWN;

  // Muevo el cuerpo: cada parte toma el lugar de la anterior
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // Muevo la cabeza según la dirección actual
  switch (currentDirection) {
    case UP:    snakeY[0] -= SNAKE_BLOCK_SIZE; break;
    case DOWN:  snakeY[0] += SNAKE_BLOCK_SIZE; break;
    case LEFT:  snakeX[0] -= SNAKE_BLOCK_SIZE; break;
    case RIGHT: snakeX[0] += SNAKE_BLOCK_SIZE; break;
  }

  // 4. Detección de Colisiones
  
  // ¿Choqué con las paredes?
  if (snakeX[0] < 0 || snakeX[0] >= SCREEN_WIDTH ||
      snakeY[0] < HEADER_HEIGHT || snakeY[0] >= SCREEN_HEIGHT) {
    gameOver();
    return;
  }

  // ¿Choqué con mi propio cuerpo?
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver();
      return;
    }
  }

  // 5. Comer
  justAte = false; // Reseteo la bandera visual
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    // Si la cabeza toca la comida...
    if (snakeLength < MAX_SNAKE_LENGTH) snakeLength++; // Crezco
    score++;        // Aumento puntaje
    justAte = true; // Activo efecto visual
    spawnFood();    // Pongo nueva comida
  }

  // === DIBUJADO EN PANTALLA ===
  display.clearDisplay();

  // Dibujo la barra superior (Header)
  display.setTextSize(1);
  
  // Si acabo de comer, invierto colores del puntaje momentáneamente
  if (justAte) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(0, 0);
  display.print("Pts: "); 
  display.print(score);
  
  // Dibujo el tiempo jugado
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(70, 0);
  unsigned long elapsedSeconds = (millis() - startTime) / 1000;
  display.print("Time: ");  
  display.print(elapsedSeconds);

  // Dibujo una línea divisoria
  display.drawRect(0, HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, SSD1306_WHITE);
  
  // Dibujo la comida (usando mi bitmap de fruta)
  display.drawBitmap(foodX, foodY, foodBitmap, 4, 4, SSD1306_WHITE);

  // Dibujo la Serpiente
  for (int i = 0; i < snakeLength; i++) {
    if (i == 0) {
      // -- Cabeza (Cuadrado Sólido) --
      display.fillRect(snakeX[i], snakeY[i], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, SSD1306_WHITE);
      
      // -- Ojos (Detalle) --
      // Calculo donde poner los ojos según a dónde miro
      int headX = snakeX[i];
      int headY = snakeY[i];
      
      switch (currentDirection) {
        case UP:
          display.drawPixel(headX + 1, headY, SSD1306_BLACK);
          display.drawPixel(headX + 2, headY, SSD1306_BLACK);
          break;
        case DOWN:
          display.drawPixel(headX + 1, headY + 3, SSD1306_BLACK);
          display.drawPixel(headX + 2, headY + 3, SSD1306_BLACK);
          break;
        case LEFT:
          display.drawPixel(headX, headY + 1, SSD1306_BLACK);
          display.drawPixel(headX, headY + 2, SSD1306_BLACK);
          break;
        case RIGHT:
          display.drawPixel(headX + 3, headY + 1, SSD1306_BLACK);
          display.drawPixel(headX + 3, headY + 2, SSD1306_BLACK);
          break;
      }
      
    } else {
      // -- Cuerpo (Cuadrado Hueco) --
      display.drawRect(snakeX[i], snakeY[i], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, SSD1306_WHITE);
    }
  }

  // Muestro todo lo que dibujé
  display.display();
  
  // Control de velocidad del juego
  delay(135); 
}
