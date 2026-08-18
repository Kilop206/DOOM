#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>

#define DHT_PIN 4
#define DHT_TYPE DHT11

#define LDR_PIN 34

#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

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

    delay(2000);
}

void loop()
{
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    int lightState = digitalRead(LDR_PIN);

    // Validação individual do sensor DHT11
    bool dhtOk = !isnan(temperature) && !isnan(humidity);
    bool isBright = (lightState == LOW);

    // Monitor Serial
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

    // Atualização do Display OLED
    display.clearDisplay();
    display.setTextSize(1);

    // Cabeçalho
    display.setCursor(0, 0);
    display.println("ESTACAO METEOROLOGICA");

    // Temperatura
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

    // Umidade
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

    // Luminosidade (LDR)
    display.setCursor(0, 40);
    display.print("Luz:  ");
    display.println(isBright ? "CLARO" : "ESCURO");

    // Rodapé de diagnóstico do status dos sensores
    display.setCursor(0, 54);
    display.print("DHT: ");
    display.print(dhtOk ? "OK" : "FALHA");
    display.print(" | LDR: ");
    display.print("OK");

    display.display();

    delay(2000);
}