#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define UNO_ADDR 0x08

LiquidCrystal_I2C myLcd(0x3f, 16, 2);

const int potPin = A0;
const int lightSensorPin = A1;
const int buzzerPin = 13;
const int ledPin = 12;

enum Mode { SEESAW, ARCHER, POLLEN, MODE_COUNT };
constexpr int ZONE_SIZE = 1024 / MODE_COUNT;
Mode currentMode = SEESAW;

uint8_t value = 0;
uint8_t trigger = 0;
unsigned long lastSend = 0;

void setup() {
  Wire.begin();
  myLcd.init();
  myLcd.backlight();
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  updateLcd();
}

void loop() {
  handleModeSwitch();
  readSensors();
  sendToUNO();
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

void readSensors() {
  value = 0;
  trigger = 0;
  if (currentMode == POLLEN) checkPollenLight();
  else analogWrite(ledPin, 0);
}

void checkPollenLight() {
  analogWrite(ledPin, 180);
  trigger = (analogRead(lightSensorPin) > 600) ? 1 : 0;
  if (trigger) playPollenTone();
}

void sendToUNO() {
  if (millis() - lastSend < 30) return;
  lastSend = millis();
  Wire.beginTransmission(UNO_ADDR);
  Wire.write((uint8_t)currentMode);
  Wire.write(value);
  Wire.write(trigger);
  Wire.endTransmission();
}

void updateLcd() {
  myLcd.setCursor(0, 0);
  myLcd.print("Team Bread Bored");
  myLcd.setCursor(0, 1);
  const char* modeNames[MODE_COUNT] = {"Seesaw", "Archer", "Pollen"};
  myLcd.print(modeNames[currentMode]);
  for (int i = 6; i < 16; i++) myLcd.print(" ");
}

// ================== BUZZER ==================
void playModeTone() {
  int freq = 261 + 32 * currentMode; // different tone per mode
  tone(buzzerPin, freq, 10);          // 150ms beep
}

void playPollenTone() {
  tone(buzzerPin, 261, 10);           // short tone for pollen trigger
}
