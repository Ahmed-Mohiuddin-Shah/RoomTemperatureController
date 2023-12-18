#include <Arduino.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include <Encoder.h>

#include <DHT.h>

#define TERMINALBROWN 0x6a01
#define TERMINALOUTLINEYELLOW 0x9bc3
#define TERMINALTEXTGOLD 0xfdc1

#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8

// TFT_SCL 13
// TFT_SDA 11

#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_BUTTON A2

#define DHTPIN 7

#define HEATER 6
#define COOLER 5

#define CURRENT_SENSOR A0

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Encoder encoder(ENCODER_CLK, ENCODER_DT);
DHT dht11(DHTPIN, DHT11);

int previousX = 0;
float prevTempC = 0;
float tempC;
int x = 25;

bool shouldSetTemp = false;
bool shouldRefresh = false;
bool heaterStatus = false;
bool coolerStatus = false;

String setTempString = "";

int currentSensorDelay = 0;
int prevCurrentSensorDelay = 0;

void setup(void)
{

  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(CURRENT_SENSOR, INPUT);

  pinMode(HEATER, OUTPUT);
  pinMode(COOLER, OUTPUT);

  dht11.begin();

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(TERMINALBROWN);
  tft.fillRect(5, 5, 150, 118, TERMINALOUTLINEYELLOW);
  tft.fillRect(7, 7, 146, 114, TERMINALBROWN);
  tft.fillRect(30, 2, 100, 18, TERMINALOUTLINEYELLOW);
  tft.fillRect(31, 3, 98, 16, TERMINALBROWN);

  tft.setTextColor(TERMINALTEXTGOLD);
  tft.setCursor(38, 4);
  tft.setTextSize(2);
  tft.print("R-T-C-S");

  tft.setCursor(0, 0);
  tft.setTextSize(1);

  currentSensorDelay = millis();
}

void temperatureSettingLogic() {
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
    x += encoder.readAndReset() / 2;

    if (x > 69)
    {
      x = 69;
    }
    else if (x < 0)
    {
      x = 0;
    }
    setTempString = "<" + String(x) + " C" + ">";
    if (shouldRefresh)
    {
      tft.fillRect(100, 41, 50, 10, TERMINALBROWN);
      shouldRefresh = false;
    }
  }
  else
  {
    setTempString = String(x) + " C";
    if (shouldRefresh)
    {
      tft.fillRect(100, 41, 50, 10, TERMINALBROWN);
      shouldRefresh = false;
    }
  }
}

float pollCurrentSensor(int noSamples = 10) {
  float AcsValue = 0.0, Samples = 0.0, AvgAcs = 0.0, AcsValueF = 0.0;

  for (int i = 0; i < noSamples; i++)
  {                               // Get 150 samples
    AcsValue = analogRead(A0);    // Read current sensor values
    Samples = Samples + AcsValue; // Add samples together
    delay(3);                     // let ADC settle before following sample 3ms
  }
  AvgAcs = Samples / 10.0; // Taking Average of Samples
  AcsValueF = (2.5 - (AvgAcs * (5.0 / 1024.0))) / 0.185;
  return AcsValueF;
}

void displayLogic() {
  if (x != previousX)
  {
    tft.fillRect(100, 41, 50, 10, TERMINALBROWN);
  }

  if (tempC != prevTempC)
  {
    tft.fillRect(90, 28, 30, 10, TERMINALBROWN);
    tft.fillRect(90, 80, 30, 10, TERMINALBROWN);
    tft.fillRect(90, 95, 30, 10, TERMINALBROWN);
  }

  previousX = x;
  prevTempC = tempC;

  if (millis() - currentSensorDelay > 500)
  {
    currentSensorDelay = millis();
    tft.fillRect(90, 67, 40, 10, TERMINALBROWN);
    tft.setCursor(9, 67);
    tft.print("              ");
    tft.print(String(pollCurrentSensor()));
  }

  tft.setCursor(9, 28);
  tft.print("Room Temp.:  ");
  tft.print(String(tempC) + " C");

  tft.setCursor(9, 41);
  tft.print("Desired Temp.:  ");
  tft.print(setTempString);

  tft.setCursor(9, 54);
  tft.print("Voltage (V): 12 V");

  tft.setCursor(9, 67);
  tft.print("Current (Ah): ");

  tft.setCursor(9, 80);
  tft.print("HEATER STATUS: ");
  tft.print(String(heaterStatus ? "ON" : "OFF"));

  tft.setCursor(9, 95);
  tft.print("COOLER STATUS: ");
  tft.print(String(coolerStatus ? "ON" : "OFF"));
}

void temperatureControlLogic() {
  tempC = dht11.readTemperature();

  if (tempC < x)
  {
    heaterStatus = true;
    coolerStatus = false;
  }
  else
  {
    heaterStatus = false;
    coolerStatus = true;
  }

  digitalWrite(HEATER, heaterStatus);
  digitalWrite(COOLER, coolerStatus);
  
}

void loop()
{
  temperatureSettingLogic();

  temperatureControlLogic();

  displayLogic();
}