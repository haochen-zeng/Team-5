// MEGA2560

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pitch.h"

#define UNO_ADDR 0x08

LiquidCrystal_I2C myLcd(0x3f, 16, 2);

const int potPin = A0;
const int lightSensorPin = A1;
const int ledPin = 12;

const int buzzerPin = 13;
int notes[] = {NOTE_C5, NOTE_D5, NOTE_E5};
int noteCount = sizeof(notes) / sizeof(notes[0]);

enum Mode { SEESAW, ARCHER, POLLEN, MODE_COUNT };
constexpr int ZONE_SIZE = 1024 / MODE_COUNT;
Mode currentMode = SEESAW;

uint8_t valueFromMega = 0;
uint8_t triggerFromMega = 0;
uint8_t lastTriggerFromMega = 0;
uint8_t valueFromUno = 0;
uint8_t triggerFromUno = 0;
uint8_t lastTriggerFromUno = 0;
unsigned long lastExchange = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  myLcd.init();
  myLcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(lightSensorPin, INPUT);
  digitalWrite(ledPin, HIGH);
  updateLcd();
}

void loop() {
  handleModeSwitch();
  readSensors();
  exchangeData();
  playToneForMode();
}

void handleModeSwitch() {
  static int lastZone = -1;
  int zone = min(analogRead(potPin) / ZONE_SIZE, MODE_COUNT - 1);
  if (zone != lastZone) {
    currentMode = (Mode)zone;
    playModeTone();
    updateLcd();
    lastZone = zone;
  }
}

void exchangeData() {
  if (millis() - lastExchange < 50) return;
  lastExchange = millis();

  Wire.beginTransmission(UNO_ADDR);
  Wire.write((uint8_t)currentMode);
  Wire.write(valueFromMega);
  Wire.write(triggerFromMega);
  Wire.endTransmission();

  Wire.requestFrom(UNO_ADDR, 2);
  if (Wire.available() >= 2) {
    valueFromUno = Wire.read();
    lastTriggerFromUno = triggerFromUno;
    triggerFromUno = Wire.read();
  }
}

void readSensors() {
  valueFromMega = 0;
  triggerFromMega = 0;
  digitalWrite(ledPin, (currentMode == POLLEN) ? HIGH : LOW);

  switch (currentMode) {
    case POLLEN:
      triggerFromMega = (analogRead(lightSensorPin) > 600) ? 1 : 0;
      break;
  }
}

void updateLcd() {
  myLcd.setCursor(0, 0);
  myLcd.print("Team Bread Bored");
  myLcd.setCursor(0, 1);
  const char* modeNames[MODE_COUNT] = {"Seesaw", "Archer", "Pollen"};
  myLcd.print(modeNames[currentMode]);
  for (int i = 6; i < 16; i++) myLcd.print(" ");
}

void playModeTone() {
  tone(buzzerPin, notes[currentMode], 10);
}

void playToneForMode() {
  switch (currentMode) {
    case SEESAW: playSeesawTone(); break;
    case ARCHER: playArcherTone(); break;
    case POLLEN: playPollenTone(); break;
  }
}

void playSeesawTone() {
  if (triggerFromUno == 1 && lastTriggerFromUno == 0) {
    playRandomTone();
  }
}

void playArcherTone() {
  static uint8_t lastArcherTrigger = 0;
  static unsigned long lastArcherPulse = 0;

  if (triggerFromUno == 1 && lastArcherTrigger == 0) {
    tone(buzzerPin, NOTE_C5, 10);
  } 
  else if (valueFromUno > 0 && triggerFromUno == 0) {
    int pulseInterval = map(valueFromUno, 0, 100, 200, 20); 
    if (millis() - lastArcherPulse > pulseInterval) {
      int freq = map(valueFromUno, 0, 100, NOTE_C5, NOTE_C6);
      tone(buzzerPin, freq, 10);
      lastArcherPulse = millis();
    }
  }
  
  lastArcherTrigger = triggerFromUno;
}

void playPollenTone() {
  if (triggerFromMega == 1 && lastTriggerFromMega == 0) {
    playRandomTone();
  }
  lastTriggerFromMega = triggerFromMega;
}

void playRandomTone() {
  tone(buzzerPin, notes[random(noteCount)], 10);
}