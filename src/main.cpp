#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// Pin definitions
#define BTN_FWD 18
#define BTN_TURN_R 32
#define BTN_TURN_L 33
#define BTN_FIRE 23
#define BUZZER_PIN 25

// Doom theme frequencies
#define NOTE_E3  165
#define NOTE_E4  330
#define NOTE_D4  294
#define NOTE_C4  262
#define NOTE_AS3 233
#define NOTE_B3  247

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int MAP_W = 16;
const int MAP_H = 16;
int mapData[MAP_W][MAP_H];

#define MAX_HEALTH 300
int playerHealth = MAX_HEALTH;

// Difficulty setup
int currentDifficulty = 1; 
const char* diffNames[]   = {"EASY", "MEDIUM", "HARD"};
const int diffDamage[]    = {10, 20, 35};
const int diffScoreMult[] = {1, 2, 4};

int score = 0;
unsigned long lastDamageTime = 0;

struct Enemy {
    double x;
    double y;
    bool alive;
    unsigned long deathTime;
};

#define MAX_ENEMIES 20
Enemy enemies[MAX_ENEMIES];

// Player state
double posX = 1.5, posY = 1.5;
double dirX = -1.0, dirY = 0.0;
double planeX = 0.0, planeY = 0.66;

bool isShooting = false;
unsigned long shootTimer = 0;

int doom_melody[] = {
  NOTE_E3, NOTE_E3, NOTE_E4, NOTE_E3, NOTE_E3, NOTE_D4, NOTE_E3, NOTE_E3, 
  NOTE_C4, NOTE_E3, NOTE_E3, NOTE_AS3, NOTE_B3, NOTE_C4
};

int doom_tempo[] = {
  16, 16, 8, 16, 16, 8, 16, 16, 
  8, 16, 16, 8, 8, 8
};

bool isAnyButtonPressed() {
    return (digitalRead(BTN_FIRE) == LOW || 
            digitalRead(BTN_FWD) == LOW || 
            digitalRead(BTN_TURN_R) == LOW || 
            digitalRead(BTN_TURN_L) == LOW);
}

void waitForButtonRelease() {
    unsigned long timeout = millis();
    while(isAnyButtonPressed() && (millis() - timeout < 1000)) {
        delay(10);
    }
}

int getRemainingEnemies() {
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) count++;
    }
    return count;
}

// Plays melody and interrupts if any button is pressed
bool playDoomThemeWithInterrupt() {
    int size = sizeof(doom_melody) / sizeof(int);
    for (int i = 0; i < size; i++) {
        if (isAnyButtonPressed()) {
            noTone(BUZZER_PIN);
            return true; 
        }
        int noteDuration = 1500 / doom_tempo[i]; 
        tone(BUZZER_PIN, doom_melody[i], noteDuration);
        
        int pauseBetweenNotes = noteDuration * 1.20;
        unsigned long startWait = millis();
        
        while(millis() - startWait < pauseBetweenNotes) {
            if (isAnyButtonPressed()) {
                noTone(BUZZER_PIN);
                return true;
            }
            delay(5);
        }
        noTone(BUZZER_PIN);
    }
    return false;
}

void generateMap() {
    for (int x = 0; x < MAP_W; x++) {
        for (int y = 0; y < MAP_H; y++) {
            if (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1) {
                mapData[x][y] = 1; 
            } else {
                mapData[x][y] = 0; 
            }
        }
    }
    
    int numPillars = random(10, 25);
    for (int i = 0; i < numPillars; i++) {
        int px = random(2, MAP_W - 2);
        int py = random(2, MAP_H - 2);
        if (px >= 3 || py >= 3) {
            mapData[px][py] = 1;
        }
    }
}

void spawnEnemy(int idx) {
    int rx, ry;
    int attempts = 0;
    do {
        rx = random(1, MAP_W - 1);
        ry = random(1, MAP_H - 1);
        attempts++;
    } while ((mapData[rx][ry] != 0 || (abs(rx - (int)posX) <= 1 && abs(ry - (int)posY) <= 1)) && attempts < 100);

    enemies[idx].x = rx + 0.5;
    enemies[idx].y = ry + 0.5;
    enemies[idx].alive = true;
}

void rotatePlayer(double speed) {
    double oldDirX = dirX;
    dirX = dirX * cos(speed) - dirY * sin(speed);
    dirY = oldDirX * sin(speed) + dirY * cos(speed);
    double oldPlaneX = planeX;
    planeX = planeX * cos(speed) - planeY * sin(speed);
    planeY = oldPlaneX * sin(speed) + planeY * cos(speed);
}

void playGunshotSound() {
    for (int freq = 900; freq > 150; freq -= 60) {
        tone(BUZZER_PIN, freq, 6);
        delay(3);
    }
    noTone(BUZZER_PIN);
}

void checkHit() {
    int targetIdx = -1;
    double closestDist = 999.0;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;

        double dx = enemies[i].x - posX;
        double dy = enemies[i].y - posY;
        double dist = sqrt(dx * dx + dy * dy);

        double dot = (dx / dist) * dirX + (dy / dist) * dirY;

        if (dot > 0.92 && dist < 5.0 && dist < closestDist) {
            closestDist = dist;
            targetIdx = i;
        }
    }

    if (targetIdx != -1) {
        enemies[targetIdx].alive = false;
        enemies[targetIdx].deathTime = millis();
        score += 50 * diffScoreMult[currentDifficulty]; 
        tone(BUZZER_PIN, 100, 150);
    }
}

void updateEnemies(unsigned long currentMillis) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) {
            double dx = enemies[i].x - posX;
            double dy = enemies[i].y - posY;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 3.0 && (currentMillis - lastDamageTime > 1500)) {
                playerHealth -= diffDamage[currentDifficulty];
                if (playerHealth < 0) playerHealth = 0;
                lastDamageTime = currentMillis;
                tone(BUZZER_PIN, 50, 200); 
            }
        }
    }
}

void resetGame() {
    playerHealth = MAX_HEALTH;
    score = 0; 
    posX = 1.5; 
    posY = 1.5;
    dirX = -1.0; 
    dirY = 0.0;
    planeX = 0.0; 
    planeY = 0.66;
    
    generateMap(); 
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        spawnEnemy(i);
    }
}

void drawGun(bool flash) {
    int offsetY = flash ? 3 : 0;
    display.fillRect(58, 50 + offsetY, 12, 14, SSD1306_WHITE);
    display.fillRect(62, 44 + offsetY, 4, 6, SSD1306_WHITE);

    if (flash) {
        display.fillTriangle(64, 36, 58, 43, 70, 43, SSD1306_WHITE);
        display.drawLine(64, 33, 64, 43, SSD1306_BLACK);
    } else {
        display.drawPixel(63, 43, SSD1306_BLACK);
    }
}

void drawUI() {
    display.drawRect(2, 2, 35, 6, SSD1306_WHITE);
    int hpWidth = (playerHealth * 31) / MAX_HEALTH;
    if (hpWidth > 0) {
        display.fillRect(4, 4, hpWidth, 2, SSD1306_WHITE);
    }
    
    display.setTextSize(1);
    
    display.setCursor(42, 1);
    display.print("E:");
    display.print(getRemainingEnemies());

    display.setCursor(75, 1);
    display.print("PTS:");
    display.print(score);
}

void drawMinimap() {
    int blockSize = 4;
    int mapPixelW = MAP_W * blockSize;
    int mapPixelH = MAP_H * blockSize;
    int offsetX = (SCREEN_WIDTH - mapPixelW) / 2;
    int offsetY = (SCREEN_HEIGHT - mapPixelH) / 2;

    display.fillRect(offsetX - 2, offsetY - 2, mapPixelW + 4, mapPixelH + 4, SSD1306_BLACK);
    display.drawRect(offsetX - 2, offsetY - 2, mapPixelW + 4, mapPixelH + 4, SSD1306_WHITE);

    for (int x = 0; x < MAP_W; x++) {
        for (int y = 0; y < MAP_H; y++) {
            if (mapData[x][y] == 1) {
                display.fillRect(offsetX + x * blockSize, offsetY + y * blockSize, blockSize, blockSize, SSD1306_WHITE);
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) {
            int ex = offsetX + (int)(enemies[i].x * blockSize);
            int ey = offsetY + (int)(enemies[i].y * blockSize);
            display.drawRect(ex - 1, ey - 1, 3, 3, SSD1306_WHITE);
        }
    }

    int px = offsetX + (int)(posX * blockSize);
    int py = offsetY + (int)(posY * blockSize);
    display.drawCircle(px, py, 2, SSD1306_WHITE);
    display.drawLine(px, py, px + (int)(dirX * 5), py + (int)(dirY * 5), SSD1306_WHITE);
}

void drawEnemies(double zBuffer[]) {
    int order[MAX_ENEMIES];
    double dists[MAX_ENEMIES];

    for (int i = 0; i < MAX_ENEMIES; i++) {
        order[i] = i;
        dists[i] = ((posX - enemies[i].x) * (posX - enemies[i].x) + (posY - enemies[i].y) * (posY - enemies[i].y));
    }

    for (int i = 0; i < MAX_ENEMIES - 1; i++) {
        for (int j = i + 1; j < MAX_ENEMIES; j++) {
            if (dists[i] < dists[j]) {
                double tempD = dists[i]; dists[i] = dists[j]; dists[j] = tempD;
                int tempO = order[i]; order[i] = order[j]; order[j] = tempO;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        int idx = order[i];
        if (!enemies[idx].alive) continue;

        double spriteX = enemies[idx].x - posX;
        double spriteY = enemies[idx].y - posY;

        double invDet = 1.0 / (planeX * dirY - dirX * planeY);
        double transformX = invDet * (dirY * spriteX - dirX * spriteY);
        double transformY = invDet * (-planeY * spriteX + planeX * spriteY);

        if (transformY > 0) {
            int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
            int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));
            int spriteWidth = spriteHeight;

            int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawStartY < 0) drawStartY = 0;
            int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
            if (drawEndY >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;

            int drawStartX = -spriteWidth / 2 + spriteScreenX;
            if (drawStartX < 0) drawStartX = 0;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
            if (drawEndX >= SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;

            for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
                if (transformY < zBuffer[stripe]) {
                    if (stripe == drawStartX || stripe == drawEndX - 1) {
                        display.fillRect(stripe, drawStartY, 1, drawEndY - drawStartY, SSD1306_WHITE);
                    } else {
                        display.fillRect(stripe, drawStartY, 1, drawEndY - drawStartY, SSD1306_BLACK);
                        display.drawPixel(stripe, drawStartY, SSD1306_WHITE);
                        display.drawPixel(stripe, drawEndY - 1, SSD1306_WHITE);
                        if (stripe > drawStartX + spriteWidth / 3 && stripe < drawEndX - spriteWidth / 3) {
                            display.fillRect(stripe, drawStartY + spriteHeight / 3, 1, spriteHeight / 3, SSD1306_WHITE);
                        }
                    }
                }
            }
        }
    }
}

void showTitleScreen() {
    waitForButtonRelease();
    int selectedDiff = 1;
    
    int musicIndex = 0;
    unsigned long noteStartTime = 0;
    int noteDuration = 0;
    
    unsigned long lastInputTime = 0;
    bool confirmed = false;

    while(!confirmed) {
        unsigned long now = millis();

        if (now - noteStartTime >= (unsigned long)noteDuration) {
            noTone(BUZZER_PIN);
            int melodySize = sizeof(doom_melody) / sizeof(int);
            musicIndex = (musicIndex + 1) % melodySize;
            int rawDuration = 1500 / doom_tempo[musicIndex];
            noteDuration = rawDuration * 1.20;
            tone(BUZZER_PIN, doom_melody[musicIndex], rawDuration);
            noteStartTime = now;
        }

        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        
        display.setTextSize(1);
        display.setCursor(22, 5);
        display.println("DOOM: SURVIVAL");
        
        display.setCursor(28, 22);
        display.println("DIFFICULTY:");
        
        display.setTextSize(2);
        int textOffset = (selectedDiff == 0) ? 32 : ((selectedDiff == 1) ? 26 : 38);
        display.setCursor(textOffset, 34);
        display.print(diffNames[selectedDiff]);

        display.setTextSize(1);
        display.setCursor(2, 54);
        display.println("L/R: Change | FIRE: OK");
        display.display();

        if (now - lastInputTime > 200) {
            if (digitalRead(BTN_TURN_L) == LOW) {
                selectedDiff = (selectedDiff - 1 + 3) % 3;
                tone(BUZZER_PIN, 400, 40);
                lastInputTime = now;
            } else if (digitalRead(BTN_TURN_R) == LOW) {
                selectedDiff = (selectedDiff + 1) % 3;
                tone(BUZZER_PIN, 400, 40);
                lastInputTime = now;
            } else if (digitalRead(BTN_FIRE) == LOW) {
                tone(BUZZER_PIN, 800, 150);
                confirmed = true;
                lastInputTime = now;
            }
        }
        delay(10);
    }

    noTone(BUZZER_PIN);
    currentDifficulty = selectedDiff;
    waitForButtonRelease();
}

void showGameOverScreen() {
    waitForButtonRelease();
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.print("GAME OVER");
    
    display.setTextSize(1);
    display.setCursor(20, 35);
    display.print("Score: ");
    display.print(score);
    
    display.setCursor(5, 55);
    display.print("Press ANY button");
    display.display();
    
    while(true) {
        if (playDoomThemeWithInterrupt()) break;
    }
    
    waitForButtonRelease();
}

void showVictoryScreen() {
    waitForButtonRelease();
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(15, 10);
    display.print("VICTORY!");
    
    display.setTextSize(1);
    display.setCursor(20, 35);
    display.print("Final Score: ");
    display.print(score);
    
    display.setCursor(5, 55);
    display.print("Press ANY button");
    display.display();
    
    while(true) {
        if (playDoomThemeWithInterrupt()) break;
    }
    
    waitForButtonRelease();
}

void setup() {
    pinMode(BTN_FWD, INPUT_PULLUP);
    pinMode(BTN_TURN_R, INPUT_PULLUP);
    pinMode(BTN_TURN_L, INPUT_PULLUP);
    pinMode(BTN_FIRE, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);

    randomSeed(analogRead(34));

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        while (true);
    }

    showTitleScreen();
    resetGame();
}

void loop() {
    unsigned long currentMillis = millis();

    if (playerHealth <= 0) {
        showGameOverScreen();
        showTitleScreen();
        resetGame();
        return;
    }

    if (getRemainingEnemies() <= 0) {
        showVictoryScreen();
        showTitleScreen();
        resetGame();
        return;
    }

    updateEnemies(currentMillis);

    double zBuffer[SCREEN_WIDTH];

    bool pressFwd = (digitalRead(BTN_FWD) == LOW);
    bool pressTurnR = (digitalRead(BTN_TURN_R) == LOW);
    bool pressTurnL = (digitalRead(BTN_TURN_L) == LOW);
    bool pressFire = (digitalRead(BTN_FIRE) == LOW);
    
    bool showMap = (pressTurnR && pressTurnL);

    if (pressFire) {
        if (!isShooting && !showMap) {
            isShooting = true;
            shootTimer = currentMillis;
            playGunshotSound();
            checkHit();
        }
    } else if (!showMap) { 
        if (pressFwd) {
            double moveSpeed = 0.12;
            if (mapData[(int)(posX + dirX * moveSpeed)][(int)posY] == 0) posX += dirX * moveSpeed;
            if (mapData[(int)posX][(int)(posY + dirY * moveSpeed)] == 0) posY += dirY * moveSpeed;
            tone(BUZZER_PIN, 180, 15);
        }
        if (pressTurnR) {
            rotatePlayer(-0.15);
        }
        if (pressTurnL) {
            rotatePlayer(0.15); 
        }
    }

    if (isShooting && (currentMillis - shootTimer > 120)) {
        isShooting = false;
    }

    display.clearDisplay();

    for (int x = 0; x < 64; x++) {
        double cameraX = 2 * (x * 2) / (double)SCREEN_WIDTH - 1;
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;

        int mapX = (int)posX;
        int mapY = (int)posY;

        double sideDistX, sideDistY;
        double deltaDistX = abs(1 / rayDirX);
        double deltaDistY = abs(1 / rayDirY);
        double perpWallDist;

        int stepX, stepY;
        int hit = 0, side = 0;

        if (rayDirX < 0) { stepX = -1; sideDistX = (posX - mapX) * deltaDistX; }
        else { stepX = 1; sideDistX = (mapX + 1.0 - posX) * deltaDistX; }

        if (rayDirY < 0) { stepY = -1; sideDistY = (posY - mapY) * deltaDistY; }
        else { stepY = 1; sideDistY = (mapY + 1.0 - posY) * deltaDistY; }

        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX; mapX += stepX; side = 0;
            } else {
                sideDistY += deltaDistY; mapY += stepY; side = 1;
            }
            if (mapData[mapX][mapY] > 0) hit = 1;
        }

        if (side == 0) perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
        else          perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;

        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

        display.fillRect(x * 2, drawStart, 2, drawEnd - drawStart, SSD1306_WHITE);

        zBuffer[x * 2] = perpWallDist;
        zBuffer[x * 2 + 1] = perpWallDist;
    }

    drawEnemies(zBuffer);

    if (showMap) {
        drawMinimap();
    } else {
        drawGun(isShooting);
    }
    
    drawUI(); 

    display.display();
    delay(10);
}