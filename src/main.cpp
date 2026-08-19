#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "../include/secrets.hpp"

#define DHT_PIN 4
#define DHT_TYPE DHT11

#define LDR_PIN 34
#define PIR_PIN 19
#define BUZZER_PIN 25

#define BTN_NEXT_PIN 32
#define BTN_PREV_PIN 33

#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// Tamanho do histórico para o gráfico
#define GRAPH_SAMPLES 30

const char* SSID = ssid;
const char* PASSWORD = password;
const char* MQTT_SERVER = mqtt_server;
const int PORT = port;

DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

WiFiClient espClient;
PubSubClient client(espClient);

// Controle do Buzzer e Sensores
unsigned long lastBuzzerToggle = 0;
unsigned long motionStartTime = 0;
bool buzzerState = false;

// Controle de Telas (0: Resumo, 1: DHT11+Gráfico, 2: LDR, 3: PIR)
int currentScreen = 0;
const int TOTAL_SCREENS = 4;

// Debounce dos botões
unsigned long lastBtnPress = 0;
const long debounceDelay = 200;

// Histórico para gráficos
float tempHistory[GRAPH_SAMPLES];
int historyIndex = 0;
unsigned long lastSampleTime = 0;

void addTemperatureSample(float temp) {
    tempHistory[historyIndex] = temp;
    historyIndex = (historyIndex + 1) % GRAPH_SAMPLES;
}

void drawTemperatureGraph(int x, int y, int w, int h) {
    // 1. Encontra valores mínimo e máximo reais para calcular a escala
    float minT = 100.0, maxT = -100.0;
    int validSamples = 0;

    for (int i = 0; i < GRAPH_SAMPLES; i++) {
        if (tempHistory[i] > 0) {
            if (tempHistory[i] < minT) minT = tempHistory[i];
            if (tempHistory[i] > maxT) maxT = tempHistory[i];
            validSamples++;
        }
    }

    // Valores padrão caso não haja leituras suficientes ainda
    if (validSamples == 0) { minT = 20.0; maxT = 30.0; }
    if (maxT <= minT) { maxT = minT + 1.0; } // Evita divisão por zero

    // 2. Rótulos do Eixo Y (Temperatura em °C)
    display.setTextSize(1);
    display.setCursor(x, y);
    display.print((int)ceil(maxT));
    display.print("C"); // Limite superior

    display.setCursor(x, y + h - 16);
    display.print((int)floor(minT));
    display.print("C"); // Limite inferior

    // Ajuste da área útil do gráfico (reservando espaço para o texto da esquerda)
    int graphX = x + 20;
    int graphW = w - 20;
    int graphH = h - 10;

    // Moldura do Gráfico
    display.drawRect(graphX, y, graphW, graphH, SSD1306_WHITE);

    // 3. Rótulos do Eixo X (Escola de Tempo)
    display.setCursor(graphX, y + graphH + 2);
    display.print("-60s"); // Início do histórico (30 amostras * 2s)
    display.setCursor(graphX + graphW - 24, y + graphH + 2);
    display.print("agora");

    // 4. Desenho da Linha de Dados
    int stepX = graphW / (GRAPH_SAMPLES - 1);

    for (int i = 0; i < GRAPH_SAMPLES - 1; i++) {
        int idx1 = (historyIndex + i) % GRAPH_SAMPLES;
        int idx2 = (historyIndex + i + 1) % GRAPH_SAMPLES;

        if (tempHistory[idx1] == 0 || tempHistory[idx2] == 0) continue;

        // Mapeia os valores de temperatura para os pixels da tela
        int y1 = y + graphH - 2 - (int)((tempHistory[idx1] - minT) / (maxT - minT) * (graphH - 4));
        int y2 = y + graphH - 2 - (int)((tempHistory[idx2] - minT) / (maxT - minT) * (graphH - 4));

        display.drawLine(graphX + (i * stepX), y1, graphX + ((i + 1) * stepX), y2, SSD1306_WHITE);
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(DHT_PIN, INPUT_PULLUP);
    dht.begin();
    pinMode(LDR_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(BTN_PREV_PIN, INPUT_PULLUP);

    for (int i = 0; i < GRAPH_SAMPLES; i++) tempHistory[i] = 0;

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        while (true) { delay(1000); }
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESTACAO METEOROLOGICA");
    display.println("\nInicializando...");
    display.display();

    delay(1500);
}

void loop()
{
    unsigned long currentMillis = millis();

    // 1. Leitura de Botões (Navegação de Tela)
    if (currentMillis - lastBtnPress > debounceDelay) {
        if (digitalRead(BTN_NEXT_PIN) == LOW) {
            currentScreen = (currentScreen + 1) % TOTAL_SCREENS;
            lastBtnPress = currentMillis;
        }
        else if (digitalRead(BTN_PREV_PIN) == LOW) {
            currentScreen = (currentScreen - 1 + TOTAL_SCREENS) % TOTAL_SCREENS;
            lastBtnPress = currentMillis;
        }
    }

    // 2. Leitura contínua dos Sensores
    bool motionDetected = (digitalRead(PIR_PIN) == HIGH);
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int lightState = digitalRead(LDR_PIN);
    bool dhtOk = !isnan(temperature) && !isnan(humidity);
    bool isBright = (lightState == LOW);

    // Amostragem do histórico para o gráfico (a cada 2s)
    if (currentMillis - lastSampleTime >= 2000) {
        lastSampleTime = currentMillis;
        if (dhtOk) addTemperatureSample(temperature);
    }

    // 3. Lógica do Buzzer
    if (motionDetected) {
        if (motionStartTime == 0) motionStartTime = currentMillis;
        unsigned long duration = currentMillis - motionStartTime;
        int beepInterval = map(constrain(duration, 0, 5000), 0, 5000, 500, 40);

        if (currentMillis - lastBuzzerToggle >= (unsigned long)beepInterval) {
            lastBuzzerToggle = currentMillis;
            buzzerState = !buzzerState;
            digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        }
    } else {
        motionStartTime = 0;
        digitalWrite(BUZZER_PIN, LOW);
        buzzerState = false;
    }

    // 4. Renderização da Tela Ativa
    display.clearDisplay();
    display.setTextSize(1);

    switch (currentScreen) {
        case 0: // RESUMO
            display.setCursor(0, 0);
            display.println("[1/4] DASHBOARD");
            display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
            
            display.setCursor(0, 16);
            display.print("Temp: "); display.print(dhtOk ? String(temperature, 1) + " C" : "ERR");
            display.setCursor(0, 28);
            display.print("Umid: "); display.print(dhtOk ? String(humidity, 1) + " %" : "ERR");
            display.setCursor(0, 40);
            display.print("Luz:  "); display.println(isBright ? "CLARO" : "ESCURO");
            display.setCursor(0, 52);
            display.print("PIR:  "); display.println(motionDetected ? "ALERTA!" : "OK");
            break;

        case 1: // DHT11 + GRÁFICO
            display.setCursor(0, 0);
            display.println("[2/4] SENSOR DHT11");
            display.setCursor(0, 10);
            display.print("T:"); 
            display.print(dhtOk ? String(temperature, 1) + "C" : "--");
            display.print(" | U:"); 
            display.print(dhtOk ? String(humidity, 0) + "%" : "--");
            
            // Desenha o gráfico na posição Y=20 com altura de 44 pixels
            drawTemperatureGraph(0, 20, 128, 44);
            break;

        case 2: // LDR
            display.setCursor(0, 0);
            display.println("[3/4] SENSOR LDR");
            display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

            display.setCursor(0, 22);
            display.print("Luminosidade:");
            display.setTextSize(2);
            display.setCursor(10, 38);
            display.println(isBright ? "CLARO" : "ESCURO");
            break;

        case 3: // PIR
            display.setCursor(0, 0);
            display.println("[4/4] SENSOR PIR");
            display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

            display.setCursor(0, 20);
            display.print("Status: ");
            display.println(motionDetected ? "MOVIMENTO!" : "SEM PRESENCA");

            display.setCursor(0, 36);
            if (motionDetected) {
                display.print("Tempo: ");
                display.print((currentMillis - motionStartTime) / 1000);
                display.print(" s");
            } else {
                display.print("Ambiente limpo");
            }
            break;
    }

    display.display();
}