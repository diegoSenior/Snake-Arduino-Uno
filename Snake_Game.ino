// Part of this code was written with the help of AI
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración de pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pines Joystick
#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2

// Pin Buzzer
#define BUZZER_PIN 3

// Configuración del juego
#define SNAKE_BLOCK_SIZE 4
#define MAX_SNAKE_LENGTH 64
#define HEADER_HEIGHT 10

enum Direction { UP, DOWN, LEFT, RIGHT };
Direction currentDirection = RIGHT;

int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 3;

int foodX, foodY;
int score = 0;
bool isPaused = false;
bool buttonState = HIGH;
unsigned long startTime;

// Melodía
const int melodyNotes[] = { 659, 659, 0, 659, 0, 523, 659, 784, 0, 392 };
const int melodyDurations[] = { 150, 150, 150, 150, 150, 150, 150, 300, 150, 300 };
const int melodyLength = sizeof(melodyNotes) / sizeof(melodyNotes[0]);

unsigned long musicTimer = 0;
int currentNoteIndex = 0;

// --- Funciones Auxiliares ---

void spawnFood() {
  // Ajustamos para que la comida no caiga pegada al borde
  int gridWidth = SCREEN_WIDTH / SNAKE_BLOCK_SIZE;
  int gridHeight = (SCREEN_HEIGHT - HEADER_HEIGHT) / SNAKE_BLOCK_SIZE;
  foodX = random(0, gridWidth) * SNAKE_BLOCK_SIZE;
  foodY = HEADER_HEIGHT + random(0, gridHeight) * SNAKE_BLOCK_SIZE;
}

void beep(int duration, int frequency) {
  tone(BUZZER_PIN, frequency);
  delay(duration);
  noTone(BUZZER_PIN);
}

void playBackgroundMusic() {
  unsigned long currentMillis = millis();
  if (currentMillis - musicTimer >= melodyDurations[currentNoteIndex]) {
    noTone(BUZZER_PIN);
    if (melodyNotes[currentNoteIndex] != 0) {
      tone(BUZZER_PIN, melodyNotes[currentNoteIndex]);
    }
    musicTimer = currentMillis;
    currentNoteIndex++;
    if (currentNoteIndex >= melodyLength) currentNoteIndex = 0;
  }
}

// --- Funciones de Pantalla ---

void displayPause() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(34, 8); 
  display.println("PAUSA");
  display.display();
}

void gameOver() {
  noTone(BUZZER_PIN);
  beep(500, 200);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 5);   
  display.println("FIN DEL JUEGO");
  
  // CAMBIO AQUÍ: De "Puntos" a "Puntaje"
  display.setCursor(35, 20);  
  display.print("Puntaje: ");
  display.println(score);
  
  display.display();
  delay(3000);

  // Reiniciar juego
  snakeLength = 3;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = 20 - i * SNAKE_BLOCK_SIZE;
    snakeY[i] = HEADER_HEIGHT + SNAKE_BLOCK_SIZE;
  }
  score = 0;
  startTime = millis();
  currentDirection = RIGHT;
  spawnFood();
  musicTimer = millis();
  currentNoteIndex = 0;
  if (melodyNotes[0] != 0) tone(BUZZER_PIN, melodyNotes[0]);
}

// --- Setup y Loop ---

void setup() {
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0));
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); 
  }
  
  display.clearDisplay();
  display.display();

  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = 20 - i * SNAKE_BLOCK_SIZE;
    snakeY[i] = HEADER_HEIGHT + SNAKE_BLOCK_SIZE;
  }

  spawnFood();
  startTime = millis();
  musicTimer = millis();
  currentNoteIndex = 0;
  if (melodyNotes[0] != 0) tone(BUZZER_PIN, melodyNotes[0]);
}

void loop() {
  bool currentButtonState = digitalRead(JOY_SW);
  if (buttonState == HIGH && currentButtonState == LOW) {
    isPaused = !isPaused;
    beep(100, isPaused ? 400 : 800);
    delay(200); 
  }
  buttonState = currentButtonState;

  if (isPaused) {
    noTone(BUZZER_PIN);
    displayPause();
    return;
  }

  playBackgroundMusic();

  int xVal = analogRead(JOY_X);
  int yVal = analogRead(JOY_Y);
  
  if (xVal < 400 && currentDirection != RIGHT) currentDirection = LEFT;
  else if (xVal > 600 && currentDirection != LEFT) currentDirection = RIGHT;
  else if (yVal < 400 && currentDirection != DOWN) currentDirection = UP;
  else if (yVal > 600 && currentDirection != UP) currentDirection = DOWN;

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  switch (currentDirection) {
    case UP:    snakeY[0] -= SNAKE_BLOCK_SIZE; break;
    case DOWN:  snakeY[0] += SNAKE_BLOCK_SIZE; break;
    case LEFT:  snakeX[0] -= SNAKE_BLOCK_SIZE; break;
    case RIGHT: snakeX[0] += SNAKE_BLOCK_SIZE; break;
  }

  if (snakeX[0] < 0 || snakeX[0] >= SCREEN_WIDTH ||
      snakeY[0] < HEADER_HEIGHT || snakeY[0] >= SCREEN_HEIGHT) {
    gameOver();
    return;
  }

  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver();
      return;
    }
  }

  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if (snakeLength < MAX_SNAKE_LENGTH) snakeLength++;
    score++;
    beep(100, 1000);
    spawnFood();
  }

  // === DIBUJADO ===
  display.clearDisplay();

  // HEADER
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Pts: "); 
  display.print(score);
  
  display.setCursor(70, 0);
  unsigned long elapsedSeconds = (millis() - startTime) / 1000;
  display.print("Time: ");  
  display.print(elapsedSeconds);

  // MARCO
  display.drawRect(0, HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, SSD1306_WHITE);
  
  // COMIDA (CÍRCULO)
  display.fillCircle(foodX + 1, foodY + 1, 1, SSD1306_WHITE);

  // SERPIENTE (CUADROS VACÍOS)
  for (int i = 0; i < snakeLength; i++) {
    display.drawRect(snakeX[i], snakeY[i], SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE, SSD1306_WHITE);
  }

  display.display();
  delay(135);
}