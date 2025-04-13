// #define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math
#include "HRMonitor.hpp"

const int btnPin = 53;
bool inverted[8] = {false, false, false, false, false, false, false, false}
unsigned long buttonState = 0;
unsigned long heartScalar = 1.0;
int minThresh = 0;
int pulseWire = 1;
bool strandActive[8] = {true, false, false, false, false, false, false, false}; // if a strand is cycling
long strandLastUpdate[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Tracks last update to each strand
const long strandRates[8] = { // Used to generate adjusted rates for HR
  0, 21.4285714, 21.4285714, 21.4285714, 120.0, 1.111111111, 6.25, 6.25
};
const int strandMinPin[8] = {1, 3, 8, 14, 16, 18, 33, 27};
int strandLastPin[8] = {1, 3, 8, 14, 16, 18, 33, 27}; // last pin in a strand to be updated
// NOTE: This value is incrimented BEFORE being used, so initilized to strandMinPin - 1
const int strandMaxPin[8] = {3, 8, 14, 16, 18, 27, 41, 33}; // Last pin in strand
int numStrands = 8; // number of strands in use
const bool isNode[8] = {true, false, false, false, true, false, false, false}; // if a strand is a node, and should therefore activate all at pins
bool ledState[40]; // array tracking if a pin is enabled

int Threshold = 550;  // Determine which Signal to "count as a beat" and which to ignore

int window = 10;
HRMonitor monitor(window, pulseWire);

bool changeLED(int strand) {
  /*
  ** Updates each LED pin, checking if the pin
  ** is set to be cycled in reverse
  ** @param strand: the strand that is being cycled
   */
  bool isInverted = ::inverted[strand];
  int activePin = ::strandLastPin[strand] + 1;
  bool pinActive = ::ledState[activePin - 2];
  bool lastPin = false;

  if (isInverted) {
    int minPin = ::strandMinPin[strand];
    if (!pinActive) {
      digitalWrite(activePin, HIGH);
      ::ledState[activePin - 2] = true;
    } else {
      digitalWrite(activePin, LOW);
      ::ledState[activePin - 2] = false;
    }
    if (activePin - 1 <= minPin) { // Case where this is the last pin in the strand
      ::strandActive[strand] = false;
      ::strandLastPin[strand] = minPin;
      lastPin = true;
      ::inverted[strand] = false;
    } else { // if this is not the last pin in the strand
      ::strandLastPin[strand] = activePin - 1;
    }
  } else { //non inverted strands
    if (!pinActive) {
      digitalWrite(activePin, HIGH);
      ::ledState[activePin - 2] = true;
    } else {
      digitalWrite(activePin, LOW);
      ::ledState[activePin - 2] = false;
    }
    ::strandLastPin[strand] = activePin + 1;
    if (activePin == ::strandMaxPin[strand]) {
      ::strandActive = false;
      ::strandLastPin[strand] = ::strandMaxPin[strand];
      lastPin = true;
      ::inverted[strand] = true;
    }
  }
  return lastPin;
}

void updateStrand(int strand) {
  /*
   * strand updating code
   * Calls changeLED which updates the LED state in the strand,
   * and then if the strand has finished will cycle the states
   * of the other strand according to the phisology
   * NOTE: if you want to change the activation order, modify
   * the swtich case block
   * @param strand: the strand index number
   */
  int activePin = ::strandLastPin[strand] + 1;
  bool pinActive = ::ledState[activePin - 2];
  bool lastPin = changeLED(strand);
  if (lastPin) {
    switch (strand) {
    case 0:
      ::strandActive[1] = true;
      ::strandActive[2] = true;
      ::strandActive[3] = true;
      break;
    case 3: // Switch clause to activate subsequent strands at termination of previous
      if (strand == 0) {
        break;
      } else {
        digitalWrite(17, HIGH);
        digitalWrite(18, HIGH); // If the lower IN Path strand finishes, activate Av Node
      // but do no mark as active, so the delay will occur
      }
      break;
    case 4:
      ::strandActive[5] = true;
    case 2:
      if (strand == 4) {
        break;
      } else {
        ::strandActive[4] = true;
        break;
      }
    case 5:
      ::strandActive[6] = true;
      ::strandActive[7] = true;
      break;
    default:
      break;
    }
  }
}

void resetAll() {
  /*
   * resets all values and deactivates all LEDs
   */
  for (int i = 2; i < 42; i++) {
    digitalWrite(i, LOW);
  }
  for (int i = 0; i < 8; i++) {
    ::strandLastUpdate[i] = 0;
    if (i == 0) {
      strandActive[i] = true;
    } else {
      strandActive[i] = false;
    }
  }
  for (int i=0; i < 40; i++) {
    ledState[i] = false;
  }
}

void itterLED(long strandAdjRates[8]) {
  /*
   * Cycles one loop, updating every strand at time. All globals should be
   * reset if loop to call this function is ever broken
   * @param strandAdjRates: array containing adjusted strand update rates,
   * to be generated from strandRates * heartRate
   */
  for (int strand=0; strand < ::numStrands; strand++) {
    long currentTime = millis();
    long elapsedTime = currentTime - ::strandLastUpdate[strand];
    if ((elapsedTime >= strandAdjRates[strand]) && (::strandActive[strand])) {
      updateStrand(strand);
      ::strandLastUpdate[strand] = currentTime;
    }
 }
}



void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 40; i++) {
    ledState[i] = false;
  }
  pinMode(btnPin, INPUT_PULLUP);
  for (int i = 2; i < 42; i++) { // Initilize the pins
    pinMode(i, OUTPUT);
  }
}

void loop() {
  bool fast = true;
  bool mark = false;
  float childBPM = 5 * heartScalar;
  long strandRealRate[8]; // Updates the adjusted heartrate for the person
  for (int i = 0; i < 8; i++) {
    strandRealRate[i] = strandRates[i] * 60 / childBPM;
  }
  long timeStart = millis();
  long ctime = millis();
  buttonState = pulseIn(btnPin, LOW, 1000000);
  while (childBPM > minThresh){
    ctime=millis();
    itterLED(strandRealRate);
    if ((ctime - timeStart > (420 * (60 / childBPM))) && (not mark)) {
      strandActive[0] = true;
      timeStart = millis();
    }
  }
}
