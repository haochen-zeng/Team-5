// UNO

#include <Wire.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>

#define I2C_ADDR 0x08

uint8_t currentMode = 0;
uint8_t remoteValue = 0;
uint8_t remoteTrigger = 0;
unsigned long lastSend = 0;

const int servoPin = 2;
const int neoPin = 3;
const int buttonPlusPin = 4;
const int buttonMinusPin = 5;
const int numberOfPixels = 12;

Servo myServo;
Adafruit_NeoPixel pixels(numberOfPixels, neoPin, NEO_GRB + NEO_KHZ800);

int pos = 112;
int posSpeed = 5;

enum class TiltState { LEFT, MIDDLE, RIGHT };
TiltState currentTilt = TiltState::MIDDLE;
TiltState previousTilt = TiltState::MIDDLE;

long hueRotation = 0;

enum ArcherState { IDLE, CHARGING, RELEASE };
ArcherState archerState = IDLE;

int tension = 0;
const int maxTension = 100;

bool lastPollenTrigger = false;
long pollenHue = 0;

unsigned long lastTiltTime = 0;
const unsigned long idleResetTime = 3000;
int tiltChangeCounter = 0;

int lureCount = 0;
const int maxLureCount = 3;

enum SeesawPhase { LURE, SURPRISE_FLIP, HUE_DIVERGE };
SeesawPhase seesawPhase = LURE;

long leftHue = 0;
long rightHue = 32768;
int flipCount = 0;
const int maxFlips = 4;
float leftProgress = 0;
float rightProgress = 0;

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  myServo.write(pos);
  pixels.begin();
  pixels.setBrightness(100);
  pixels.clear();
  pixels.show();
  pinMode(buttonPlusPin, INPUT_PULLUP);
  pinMode(buttonMinusPin, INPUT_PULLUP);
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void loop() {
  handleServoButtons();
  updateTiltState();
  runCurrentMode();
  delay(25);
}

void receiveEvent(int howMany) {
  if (howMany < 3) return;
  currentMode = Wire.read();
  remoteValue = Wire.read();
  remoteTrigger = Wire.read();
}

void handleServoButtons() {
  if (digitalRead(buttonPlusPin) == LOW) pos += posSpeed;
  if (digitalRead(buttonMinusPin) == LOW) pos -= posSpeed;
  pos = constrain(pos, 75, 150);
  myServo.write(pos);
}

void updateTiltState() {
  if (pos < 100) currentTilt = TiltState::LEFT;
  else if (pos > 125) currentTilt = TiltState::RIGHT;
  else currentTilt = TiltState::MIDDLE;
}

void runCurrentMode() {
  switch (currentMode) {
    case 0: runSeesawMode(); break;
    case 1: runArcherMode(); break;
    case 2: runPollenMode(); break;
  }
}

void requestEvent() {
  uint8_t dataToSend[2] = {0, 0};
  switch (currentMode) {
    case 0:
      dataToSend[0] = (uint8_t)tiltChangeCounter;
      dataToSend[1] = (currentTilt != TiltState::MIDDLE) ? 1 : 0;
      break;
    case 1:
      dataToSend[0] = (uint8_t)tension;
      dataToSend[1] = (archerState == RELEASE) ? 1 : 0;
      break;
    case 2:
      dataToSend[0] = 0;
      dataToSend[1] = 0;
      break;
  }
  Wire.write(dataToSend, 2);
}

void runSeesawMode() {
  if (currentTilt != previousTilt) {
    previousTilt = currentTilt;
    lastTiltTime = millis();
    tiltChangeCounter++;
    if (tiltChangeCounter % 2 == 0) handleSeesawPhase();
  }
  resetSeesawIfIdle();
}

void handleSeesawPhase() {
  switch (seesawPhase) {
    case LURE: runSeesawLure(); break;
    case SURPRISE_FLIP: runSeesawFlip(); break;
    case HUE_DIVERGE: runSeesawHue(); break;
  }
}

void runSeesawLure() {
  fillPixelsWhite();
  lureCount++;
  if (lureCount >= maxLureCount) {
    seesawPhase = SURPRISE_FLIP;
    flipCount = 0;
  }
}

void runSeesawFlip() {
  for (int i = 0; i < numberOfPixels; i++) {
    if (i < numberOfPixels / 2)
      pixels.setPixelColor(i, (flipCount % 2 == 0) ? pixels.Color(255,255,255) : pixels.Color(0,0,0));
    else
      pixels.setPixelColor(i, (flipCount % 2 == 0) ? pixels.Color(0,0,0) : pixels.Color(255,255,255));
  }
  pixels.show();
  flipCount++;
  if (flipCount >= maxFlips) {
    seesawPhase = HUE_DIVERGE;
    leftProgress = rightProgress = 0;
    leftHue = 0;
    rightHue = 32768;
  }
}

void runSeesawHue() {
  leftProgress = min(leftProgress + 0.2, 1);
  rightProgress = min(rightProgress + 0.2, 1);
  leftHue = (leftHue + 4000) % 65536;
  rightHue = (rightHue - 4000 + 65536) % 65536;
  for (int i = 0; i < numberOfPixels; i++) {
    uint32_t c = (i < numberOfPixels / 2) ? pixels.ColorHSV(leftHue, 255, 255) : pixels.ColorHSV(rightHue, 255, 255);
    pixels.setPixelColor(i, c);
  }
  pixels.show();
}

void resetSeesawIfIdle() {
  if (millis() - lastTiltTime > idleResetTime) {
    tiltChangeCounter = 0;
    seesawPhase = LURE;
    lureCount = 0;
    leftProgress = rightProgress = 0;
    leftHue = 0;
    rightHue = 32768;
    flipCount = 0;
    fillPixelsWhite();
  }
}

void fillPixelsWhite() {
  for (int i = 0; i < numberOfPixels; i++)
    pixels.setPixelColor(i, pixels.Color(255,255,255));
  pixels.show();
}

void runArcherMode() {
  bool plus = digitalRead(buttonPlusPin) == LOW;
  bool minus = digitalRead(buttonMinusPin) == LOW;

  switch (archerState) {
    case IDLE: enterArcherIdle(plus); break;
    case CHARGING: runArcherCharging(plus, minus); break;
    case RELEASE: runArcherRelease(); break;
  }
}

void enterArcherIdle(bool plus) {
  pos = 75;
  myServo.write(pos);
  fillPixelsWhite();
  tension = 0;
  if (plus) archerState = CHARGING;
}

void runArcherCharging(bool plus, bool minus) {
  if (plus) tension = min(tension + 3, maxTension);
  pos = 75 + map(tension, 0, maxTension, 0, 75);
  myServo.write(pos);
  archerFlicker();
  if (minus) archerState = RELEASE;
}

void archerFlicker() {
  int flickerDelay = map(tension, 0, maxTension, 120, 10);
  int flickerPixel = random(numberOfPixels);
  pixels.clear();
  pixels.setPixelColor(flickerPixel, pixels.Color(255, 255, 255));
  pixels.show();
  delay(flickerDelay);
}

void runArcherRelease() {
  pos = 75;
  myServo.write(pos);
  for (int i = 0; i < 96; i++) {
    pixels.clear();
    for (int p = 0; p < numberOfPixels; p++)
      pixels.setPixelColor(p, pixels.ColorHSV(random(65535), 255, 255));
    pixels.show();
    delay(20);
  }
  fillPixelsWhite();
  tension = 0;
  archerState = IDLE;
}

void runPollenMode() {
  static int head = 0;
  if (remoteTrigger && !lastPollenTrigger) {
    head = (head + 1) % numberOfPixels;
    flipTilt();
  }
  lastPollenTrigger = remoteTrigger;
  pixels.clear();
  pixels.setPixelColor(head, pixels.ColorHSV(pollenHue, 200, 150));
  pixels.show();
  pollenHue = (pollenHue + 500) % 65536;
}

void flipTilt() {
  if (currentTilt == TiltState::LEFT) pos = 150;
  else if (currentTilt == TiltState::RIGHT) pos = 75;
  else pos = 112;
  myServo.write(pos);
  updateTiltState();
}
