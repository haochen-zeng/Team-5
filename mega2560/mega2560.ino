// MEGA2560

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define UNO_ADDR 0x08

LiquidCrystal_I2C myLcd(0x3f, 16, 2);

const int potPin = A0;
const int lightSensorPin = A1;
const int buzzerPin = 13;

enum Mode { SEESAW, ARCHER, POLLEN, MODE_COUNT };
constexpr int ZONE_SIZE = 1024 / MODE_COUNT;
Mode currentMode = SEESAW;

uint8_t remoteValue = 0;
uint8_t remoteTrigger = 0;
unsigned long lastSend = 0;

void setup() {
  Wire.begin();
  Wire.onReceive(receiveEvent);
  myLcd.init();
  myLcd.backlight();
  pinMode(buzzerPin, OUTPUT);
  updateLcd();
}

void loop() {
  handleModeSwitch();
  sendToUNO();
}

void receiveEvent(int howMany) {
  if (howMany < 3) return;
  currentMode = Wire.read();
  remoteValue = Wire.read();
  remoteTrigger = Wire.read();
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

void sendToUNO() {
  if (millis() - lastSend < 30) return;
  lastSend = millis();
  Wire.beginTransmission(UNO_ADDR);
  Wire.write((uint8_t)currentMode);
  Wire.write(remoteValue);
  Wire.write(remoteTrigger);
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

void playModeTone() {
  int freq = 261 + 32 * currentMode; // different tone per mode
  tone(buzzerPin, freq, 10);
}

void playToneForMode() {
  switch (currentMode) {
    case SEESAW: playSeesawTone(); break;
    case ARCHER: playArcherTone(); break;
    case POLLEN: playPollenTone(); break;
  }
}

void playSeesawTone() {
  if(remoteTrigger) {
    tone(buzzerPin, 261, 10);
  }
}

void playArcherTone() {
  if(remoteTrigger) {
    tone(buzzerPin, 261, 10);
  } else {
    int freq = map(remoteValue, 0, 100, 261, 1046);
    tone(buzzerPin, freq, 32);
  }
}

void playPollenTone() {
  if(remoteTrigger) {
    tone(buzzerPin, 261, 10);
  }
}
