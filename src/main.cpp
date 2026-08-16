#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

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

    // Sensores
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
    display.println("Inicializando...");

    display.display();

    delay(2000);
}

void loop()
{
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    int lightState = digitalRead(LDR_PIN);

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("Erro ao ler DHT11!");

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("ERRO");
        display.println();
        display.println("Falha no DHT11");
        display.display();

        delay(2000);
        return;
    }

    bool isBright = lightState == LOW;

    // Serial Monitor
    Serial.println("====================");
    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Umidade: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Ambiente: ");

    if (isBright)
    {
        Serial.println("CLARO");
    }
    else
    {
        Serial.println("ESCURO");
    }

    // OLED
    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("ESTACAO METEOROLOGICA");

    display.setCursor(0, 18);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.println(" C");

    display.setCursor(0, 30);
    display.print("Umid: ");
    display.print(humidity, 1);
    display.println(" %");

    display.setCursor(0, 42);
    display.print("Luz:  ");

    if (isBright)
    {
        display.println("CLARO");
    }
    else
    {
        display.println("ESCURO");
    }

    display.setCursor(0, 54);
    display.println("ESP32 ONLINE");

    display.display();

    delay(2000);
}