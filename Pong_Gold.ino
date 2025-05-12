#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define JOYSTICK_Y A0
#define BUTTON_PIN D3

const int paddleHeight = 20;
const int paddleWidth = 4;
int paddleY = (SCREEN_HEIGHT - paddleHeight) / 2;
int paddleSpeed = 2;

int aiPaddleY = (SCREEN_HEIGHT - paddleHeight) / 2;
float aiSpeedBase = 1.5;

int ballX, ballY;
float ballSpeedX, ballSpeedY;
float ballSpeedBase = 2.0;
int ballSize = 3;

int score = 0;
unsigned long lastFrame = 0;
const int frameDelay = 33;
unsigned long gameStartTime = 0;

bool inSettings = false;
bool buttonPressed = false;

void setup() {
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(0);
  display.ssd1306_command(SSD1306_SEGREMAP | 0x1);
  display.ssd1306_command(SSD1306_COMSCANDEC);
  display.setTextColor(1);

  display.clearDisplay();
  display.setCursor(10, 28);
  display.setTextSize(1);
  display.println("PONG DOC AI");
  display.display();
  delay(1500);

  resetGame();
}

void loop() {
  if (!inSettings && digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    inSettings = true;
    settingsMenu();
    buttonPressed = false;
  }

  if (inSettings) return;

  unsigned long now = millis();
  if (now - lastFrame >= frameDelay) {
    lastFrame = now;

    updateJoystick();
    updateAI();
    updateBall();
    drawGame();
  }
}

void settingsMenu() {
  int ballLevel = (int)ballSpeedBase;
  int aiLevel = (int)aiSpeedBase;

  while (true) {
    display.clearDisplay();
    display.setCursor(10, 5);
    display.setTextSize(1);
    display.println("CAI DAT TOC DO");
    display.setCursor(10, 20);
    display.print("Bong: ");
    display.print(ballLevel);
    display.setCursor(10, 35);
    display.print("AI:   ");
    display.print(aiLevel);
    display.setCursor(10, 52);
    display.println("Gat de doi, Nhan de OK");
    display.display();

    int y = analogRead(JOYSTICK_Y);
    if (y < 400) {
      ballLevel = constrain(ballLevel - 1, 1, 5);
      delay(250);
    } else if (y > 600) {
      ballLevel = constrain(ballLevel + 1, 1, 5);
      delay(250);
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(300);
      while (digitalRead(BUTTON_PIN) == LOW);
      while (true) {
        display.clearDisplay();
        display.setCursor(10, 20);
        display.print("Chon AI: ");
        display.print(aiLevel);
        display.setCursor(10, 52);
        display.println("Gat de doi, Nhan de OK");
        display.display();

        int y2 = analogRead(JOYSTICK_Y);
        if (y2 < 400) {
          aiLevel = constrain(aiLevel - 1, 1, 5);
          delay(250);
        } else if (y2 > 600) {
          aiLevel = constrain(aiLevel + 1, 1, 5);
          delay(250);
        }

        if (digitalRead(BUTTON_PIN) == LOW) {
          delay(300);
          while (digitalRead(BUTTON_PIN) == LOW);
          ballSpeedBase = ballLevel;
          aiSpeedBase = aiLevel;
          inSettings = false;
          resetGame();
          return;
        }
      }
    }
  }
}

void resetGame() {
  score = 0;
  paddleY = (SCREEN_HEIGHT - paddleHeight) / 2;
  aiPaddleY = (SCREEN_HEIGHT - paddleHeight) / 2;
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballSpeedX = -ballSpeedBase;
  ballSpeedY = random(-1, 2);
  if (ballSpeedY == 0) ballSpeedY = 1;
  gameStartTime = millis();
}

void updateJoystick() {
  int yValue = analogRead(JOYSTICK_Y);
  if (yValue < 400 && paddleY > 0) {
    paddleY -= paddleSpeed;
  } else if (yValue > 600 && paddleY + paddleHeight < SCREEN_HEIGHT) {
    paddleY += paddleSpeed;
  }
}

void updateAI() {
  if (ballSpeedX > 0) {
    float timeToReach = (SCREEN_WIDTH - paddleWidth - ballX) / ballSpeedX;
    float predictedY = ballY + ballSpeedY * timeToReach;

    while (predictedY < 0 || predictedY + ballSize > SCREEN_HEIGHT) {
      if (predictedY < 0) predictedY = -predictedY;
      if (predictedY + ballSize > SCREEN_HEIGHT) predictedY = 2 * (SCREEN_HEIGHT - ballSize) - predictedY;
    }

    int aiCenter = aiPaddleY + paddleHeight / 2;
    if (predictedY < aiCenter - 2) {
      aiPaddleY -= aiSpeedBase + 0.5;
    } else if (predictedY > aiCenter + 2) {
      aiPaddleY += aiSpeedBase + 0.5;
    }
  } else {
    int aiCenter = aiPaddleY + paddleHeight / 2;
    int targetCenter = SCREEN_HEIGHT / 2;
    if (aiCenter < targetCenter - 1) aiPaddleY += aiSpeedBase;
    else if (aiCenter > targetCenter + 1) aiPaddleY -= aiSpeedBase;
  }
  aiPaddleY = constrain(aiPaddleY, 0, SCREEN_HEIGHT - paddleHeight);
}

void updateBall() {
  float timeFactor = 1.0 + (millis() - gameStartTime) / 30000.0;
  float currentSpeedX = ballSpeedX * timeFactor;
  float currentSpeedY = ballSpeedY * timeFactor;

  ballX += currentSpeedX;
  ballY += currentSpeedY;

  if (ballY <= 0 || ballY + ballSize >= SCREEN_HEIGHT) {
    ballSpeedY *= -1;
    if (ballY <= 0) ballY = 1;
    else ballY = SCREEN_HEIGHT - ballSize - 1;
  }

  if (ballX <= paddleWidth && ballX + ballSize >= 0 &&
      ballY + ballSize >= paddleY && ballY <= paddleY + paddleHeight) {
    int ballCenter = ballY + ballSize / 2;
    int paddleCenter = paddleY + paddleHeight / 2;
    float normalized = (float)(ballCenter - paddleCenter) / (paddleHeight / 2);
    ballSpeedY = normalized * ballSpeedBase * 1.5;
    ballSpeedX = abs(ballSpeedBase);
    score++;
    ballX = paddleWidth + 1;
  }

  if (ballX + ballSize >= SCREEN_WIDTH - paddleWidth && ballX <= SCREEN_WIDTH &&
      ballY + ballSize >= aiPaddleY && ballY <= aiPaddleY + paddleHeight) {
    int ballCenter = ballY + ballSize / 2;
    int aiCenter = aiPaddleY + paddleHeight / 2;
    float normalized = (float)(ballCenter - aiCenter) / (paddleHeight / 2);
    ballSpeedY = normalized * ballSpeedBase * 1.5;
    ballSpeedX = -abs(ballSpeedBase);
    ballX = SCREEN_WIDTH - paddleWidth - ballSize - 1;
  }

  if (ballX + ballSize < 0) {
    showGameOver();
  }

  if (ballX > SCREEN_WIDTH) {
    showWin();
  }
}

void drawGame() {
  display.clearDisplay();
  display.fillRect(0, paddleY, paddleWidth, paddleHeight, 1);
  display.fillRect(SCREEN_WIDTH - paddleWidth, aiPaddleY, paddleWidth, paddleHeight, 1);
  display.fillRect(ballX, ballY, ballSize, ballSize, 1);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Score: ");
  display.print(score);
  display.display();
}

void showGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 20);
  display.println("GAME OVER");
  display.setTextSize(1);
  display.setCursor(30, 50);
  display.print("Score: ");
  display.print(score);
  display.display();
  delay(3000);
  resetGame();
}

void showWin() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(25, 20);
  display.println("YOU WIN");
  display.setTextSize(1);
  display.setCursor(30, 50);
  display.print("Score: ");
  display.print(score);
  display.display();
  delay(3000);
  resetGame();
}
