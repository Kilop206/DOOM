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

#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

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

void setup()
{
    Serial.begin(115200);

    WiFi.setTxPower(WIFI_POWER_11dBm);

    // Sensores
    pinMode(DHT_PIN, INPUT_PULLUP);
    dht.begin();
    pinMode(LDR_PIN, INPUT);

    // OLED
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println("Erro ao inicializar OLED!");

        while (true)
        {
            delay(1000);
        }
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESTACAO METEOROLOGICA");
    display.println();
    display.println("Testando sensores...");

    display.display();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    client.setServer(mqtt_server, PORT);

    delay(2000);
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("ESP32Client")) {
        // Succesfully Connected
        } else {
            delay(5000);
        }
    }
}

void loop()
{
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    int lightState = digitalRead(LDR_PIN);

    // DHT11 validation
    bool dhtOk = !isnan(temperature) && !isnan(humidity);
    bool ldrOk = !isnan(lightState);

    // LDR validation
    bool isBright = (lightState == LOW);

    // Serial Monitor
    Serial.println("====================");
    if (dhtOk)
    {
        Serial.print("Temperatura: ");
        Serial.print(temperature);
        Serial.println(" C");

        Serial.print("Umidade: ");
        Serial.print(humidity);
        Serial.println(" %");
    }
    else
    {
        Serial.println("DHT11: ERRO DE LEITURA / DESCONECTADO");
    }

    Serial.print("Ambiente: ");
    Serial.println(isBright ? "CLARO" : "ESCURO");

    // OLED displaye update
    display.clearDisplay();
    display.setTextSize(1);

    // Header
    display.setCursor(0, 0);
    display.println("ESTACAO METEOROLOGICA");

    // Temperature
    display.setCursor(0, 16);
    display.print("Temp: ");
    if (dhtOk)
    {
        display.print(temperature, 1);
        display.println(" C");
    }
    else
    {
        display.println("ERRO!");
    }

    // Umidity
    display.setCursor(0, 28);
    display.print("Umid: ");
    if (dhtOk)
    {
        display.print(humidity, 1);
        display.println(" %");
    }
    else
    {
        display.println("ERRO!");
    }

    // Lumonosity (LDR)
    display.setCursor(0, 40);
    display.print("Luz:  ");
    display.println(isBright ? "CLARO" : "ESCURO");

    // Sensors status diagnosis footer
    display.setCursor(0, 54);
    display.print("DHT: ");
    display.print(dhtOk ? "OK" : "FALHA");
    display.print(" | LDR: ");
    display.print(ldrOk ? "OK" : "FALHA");

    display.display();

    String message = "Temperature: " + String(temperature) + "\nHumidity: " + humidity + "\nLuminosity: " + isBright;
    client.publish("esp32/sensor", message.c_str());

    delay(2000);
}