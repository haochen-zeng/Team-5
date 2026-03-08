// MEGA2560

#include <Wire.h>
#include <PrintStream.h>

const int tiltSensorPin = A0;
const int irSensorPin = A1;
const int photoresistorPin = A2;
const int ledPin = 6; // only PWM pins

int tiltThreshold = 50; // above
int irThreshold = 100; // below
int lightThreshold = 150; // above

float energy = 0.0;

float chargeRateSensors = 1;
float chargeRateLight = 2;
float decayRate = 0.2;

float maxEnergy = 10.0;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}


void loop() {
  int tiltValue = analogRead(tiltSensorPin);
  int irValue = analogRead(irSensorPin);
  int lightValue = analogRead(photoresistorPin);

  bool tiltActive = tiltValue > tiltThreshold;
  bool irActive = irValue < irThreshold;
  bool lightActive = lightValue > lightThreshold;

  if(tiltActive && irActive){
    energy += chargeRateSensors;
  }

  if(lightActive){
    energy += chargeRateLight;
  }

  energy -= decayRate;

  if(energy < 0) energy = 0;
  if(energy > maxEnergy) energy = maxEnergy;

  int brightness = map(energy * 100, 0, maxEnergy * 100, 0, 255);
  brightness = constrain(brightness,0,255);

  analogWrite(ledPin, brightness);

  Serial << "Tilt:" << tiltValue
        << " IR:" << irValue
        << " Light:" << lightValue
        << " | Active -> "
        << "T:" << tiltActive
        << " I:" << irActive
        << " L:" << lightActive
        << " | Energy:" << energy
        << " | LED:" << brightness
        << '\n';

  delay(50);
}