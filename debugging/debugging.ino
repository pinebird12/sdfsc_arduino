#define USE_ARDUINO_INTERUPTS false
#include <PulseSensorPlayground.h>


PulseSensorPlayground tester;
const int pulseWire = 1;

void setup() {
  Serial.begin(9600);
  tester.analogInput(::pulseWire);
  tester.setThreshold(500);
  Serial.println("Initilized");
  delay(10000);
  Serial.println("Delaying helping?");
  bool started = tester.begin();
  if (started) {
    Serial.println("Works Here");
  } else {
    Serial.println("Also Fails, but different");
  }
}

void loop() {
}
