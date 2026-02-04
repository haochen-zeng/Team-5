#include <Servo.h>
#include <Adafruit_NeoPixel.h> 

// int potPin = A0;
int servoPin = 2;
int pos = 0;
int posSpeed = 5;
int neoPin = 3;
int numberOfPixels = 12;
long hue = 0;
int brightness = 100;
int buttonPlusPin = 4;
int buttonMinusPin = 5;

enum class TiltState {LEFT, MIDDLE, RIGHT};
TiltState currentTilt;
TiltState previousTilt;
  
Servo myServo;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numberOfPixels, neoPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin); 
  pixels.begin();
  pixels.setBrightness(brightness);
  pixels.clear();
  pixels.show();
  pinMode(buttonPlusPin, INPUT_PULLUP);
  pinMode(buttonMinusPin, INPUT_PULLUP);
  myServo.write(112);
  currentTilt = TiltState::MIDDLE;
  previousTilt = TiltState::MIDDLE;
}
  
void loop() {
  // int potValue = analogRead(potPin);
  // potValue = map(potValue, 0, 1023, 75, 150);
  // myServo.write(potValue);
  if (digitalRead(buttonPlusPin) == LOW) {
    pos += posSpeed;
  }
  else if (digitalRead(buttonMinusPin) == LOW) {
    pos -= posSpeed;
  }
  pos = constrain(pos, 75, 150);
  myServo.write(pos); 
  updateTilt(pos);
  delay(25);
}

void updateTilt(int posInput) {
  if (posInput >= 75 && posInput < 100) {
    currentTilt = TiltState::LEFT;
  }
  else if (posInput >= 125 && posInput <= 150) {
    currentTilt = TiltState::RIGHT;
  }
  else {
    currentTilt = TiltState::MIDDLE;
  }

  if (currentTilt != previousTilt) {
    updateLight();
  }

  previousTilt = currentTilt;
}

void updateLight() {
  pixels.rainbow(hue);
  pixels.show();
  hue += 2560;
  hue = hue > (5 * 65536) ? 0 : hue;
  pixels.show();
}