#include <Arduino.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>             // LCD Communication protocol library

#include <Encoder.h> // Rotary encoder library
#include <DHT.h>     // Temperature and humidity sensor library

// LCD Colors
#define BROWN_COLOR 0x6a01
#define YELLOW_COLOR 0x9bc3
#define GOLD_TEXT_COLOR 0xfdc1

#define TFT_CS 10 // chip select
#define TFT_RST 9 // reset
#define TFT_DC 8  // data/command

// TFT_SCL 13
// TFT_SDA 11

// Rotary encoder pins
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_BUTTON A2

// Sensor pins
#define DHT_PIN 7
#define CURRENT_SENSOR_PIN A0

// module pins
#define HEATER_PIN 6
#define COOLER_PIN 5

Adafruit_ST7735 tftLcd = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Encoder encoder(ENCODER_CLK, ENCODER_DT);
DHT temperatureSensorDHT11(DHT_PIN, DHT11);

// modules initial values
int lastEncoderPosition = 0;
float lastTemperatureCelcius = 0;
float currentTemperatureCelcius = 0;
int currentEncoderPosition = 25;

bool shouldSetTemp = false;
bool shouldRefresh = false;

// modules initial status
bool heaterState = false;
bool coolerState = false;

String temperatureString = "";
int currentSensorStartReadTime = 0;

// prototypes for functions
void temperatureSettingLogic();
void temperatureControlLogic();
void displayLogic();
float pollCurrentSensor(int noSamples = 10);

void setup(void)
{
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);
    pinMode(CURRENT_SENSOR_PIN, INPUT);
    pinMode(HEATER_PIN, OUTPUT);
    pinMode(COOLER_PIN, OUTPUT);

    temperatureSensorDHT11.begin();

    tftLcd.initR(INITR_BLACKTAB);
    tftLcd.setRotation(1); // 1. landscape, 0. portrait
    tftLcd.fillScreen(BROWN_COLOR);
    tftLcd.fillRect(5, 5, 150, 118, YELLOW_COLOR);
    tftLcd.fillRect(7, 7, 146, 114, BROWN_COLOR);
    tftLcd.fillRect(30, 2, 100, 18, YELLOW_COLOR);
    tftLcd.fillRect(31, 3, 98, 16, BROWN_COLOR);

    tftLcd.setTextColor(GOLD_TEXT_COLOR);
    tftLcd.setCursor(38, 4);
    tftLcd.setTextSize(2);
    tftLcd.print("R-T-C-S");

    tftLcd.setCursor(0, 0);
    tftLcd.setTextSize(1);

    currentSensorStartReadTime = millis();
}

void loop()
{
    temperatureSettingLogic();
    temperatureControlLogic();
    displayLogic();
}

void temperatureSettingLogic()
{
    if (digitalRead(ENCODER_BUTTON) == LOW)
    {
        delay(100);
        if (digitalRead(ENCODER_BUTTON) == LOW)
        {
            shouldSetTemp = !shouldSetTemp;
            shouldRefresh = true;
        }
    }

    if (shouldSetTemp)
    {
        currentEncoderPosition += encoder.readAndReset() / 2;               // divide by 2 as we get 2 steps per click
        currentEncoderPosition = map(currentEncoderPosition, 0, 69, 0, 69); // map to 0-69 range
    }

    if (shouldSetTemp)
        temperatureString = "<" + String(currentEncoderPosition) + " C>";
    else
        temperatureString = String(currentEncoderPosition) + " C";

    if (shouldRefresh)
    {
        tftLcd.fillRect(100, 41, 50, 10, BROWN_COLOR);
        shouldRefresh = false;
    }
}
void temperatureControlLogic()
{
    currentTemperatureCelcius = temperatureSensorDHT11.readTemperature();

    heaterState = currentTemperatureCelcius < currentEncoderPosition;
    coolerState = currentTemperatureCelcius > currentEncoderPosition;

    digitalWrite(HEATER_PIN, heaterState);
    digitalWrite(COOLER_PIN, coolerState);
}

float pollCurrentSensor(int noSamples = 10)
{
    float totalOfAllSamples = 0.0;
    float currentSensorAverageValue = 0.0;
    float currentSensorFinalValue = 0.0;

    for (int i = 0; i < noSamples; i++)
    {
        totalOfAllSamples += analogRead(A0);
        delay(3);
    }
    currentSensorAverageValue = totalOfAllSamples / (float)noSamples; // Taking Average of totalOfAllSamples
    currentSensorFinalValue = (2.5 - (currentSensorAverageValue * (5.0 / 1024.0))) / 0.185;
    return currentSensorFinalValue;
}

void displayLogic()
{
    if (currentEncoderPosition != lastEncoderPosition)
        tftLcd.fillRect(100, 41, 50, 10, BROWN_COLOR);

    if (currentTemperatureCelcius != lastTemperatureCelcius)
    {
        tftLcd.fillRect(90, 28, 50, 10, BROWN_COLOR);
        tftLcd.fillRect(90, 80, 30, 10, BROWN_COLOR);
        tftLcd.fillRect(90, 95, 40, 10, BROWN_COLOR);
    }

    lastEncoderPosition = currentEncoderPosition;
    lastTemperatureCelcius = currentTemperatureCelcius;

    if (millis() - currentSensorStartReadTime > 500)
    {
        currentSensorStartReadTime = millis();
        tftLcd.fillRect(90, 67, 40, 10, BROWN_COLOR);
        tftLcd.setCursor(9, 67);
        tftLcd.print("                ");
        tftLcd.print(String(-pollCurrentSensor()));
    }

    tftLcd.setCursor(9, 28);
    tftLcd.print("Room Temp :     ");
    tftLcd.print(String(currentTemperatureCelcius) + " C");

    tftLcd.setCursor(9, 41);
    tftLcd.print("Desired Temp.:  ");
    tftLcd.print(temperatureString);

    tftLcd.setCursor(9, 54);
    tftLcd.print("Voltage (V):    12 V");

    tftLcd.setCursor(9, 67);
    tftLcd.print("Current (A):    ");

    tftLcd.setCursor(9, 80);
    tftLcd.print("HEATER STATUS:  ");
    tftLcd.print((heaterState ? "ON" : "OFF"));

    tftLcd.setCursor(9, 95);
    tftLcd.print("COOLER STATUS:  ");
    tftLcd.print((coolerState ? "ON" : "OFF"));
}
