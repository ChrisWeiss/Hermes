#include <math.h>

// LED imports
#include <SPI.h>
#include "LPD8806.h"

// Accel imports
#include <Wire.h>
#include "Adafruit_LSM303.h"

#include "AccelReading.h"

#define WAIT_FOR_KEYBOARD 0
#define BRIGHTNESS 1

void setup() {
  if (WAIT_FOR_KEYBOARD) {
    Serial.begin(9600);
    while (!Serial) { }		// Wait for serial to initalize.

  	Serial.println("Strike any key to start...");
  	while (!Serial.available()) { }
    Serial.read();
  }
  
  colorSetup();
  
  accelSetup();
}

void loop() {
  if (WAIT_FOR_KEYBOARD) {
    pauseOnKeystroke();
  }

  // showColors();

  accelPoll(10);
}

void pauseOnKeystroke() {
  if (Serial.available()) {
    Serial.read();
    Serial.println("Paused. Strike any key to resume...");
    colorOff();
    while (!Serial.available());
    Serial.read();
  }
}


///////////
// accel //
///////////

Adafruit_LSM303 lsm; // Bridge to accelerometer hardware.
AccelReading accelBuffer[10]; // Buffer for storing the last 10 readings.
int bufferPosition; // Current read position of the buffer.

// Initialization.
void accelSetup() {
  Serial.println("BEGIN");
  
  lsm.begin();
  
  bufferPosition = 0;

  // Initialize the entire buffer.
  for (int i = 0; i < bufferSize(); i++) {
    accelBuffer[i].x = 0;
    accelBuffer[i].y = 0;
    accelBuffer[i].z = 0;
  }
  
  printBuffer();
}

// Gathers data from accelerometer into the buffer, pausing afterwards for the given delay.
// Currently discards duplicate readings (when we're reading faster than the hardware is updating),
// so delay might actually be irrelevant.
void accelPoll(int delayMilliseconds) {
  // Read from the hardware.
  lsm.read();
  
  int newX = lsm.accelData.x;
  int newY = lsm.accelData.y;
  int newZ = lsm.accelData.z;

  AccelReading *mutableCurrentReading = &accelBuffer[bufferPosition];

  mutableCurrentReading->x = newX;
  mutableCurrentReading->y = newY;
  mutableCurrentReading->z = newZ;
  
  // If the new reading is the same as the previous reading, don't advance the buffer.
  if (equalReadings(getPreviousReading(), getCurrentReading())) {
    delay(delayMilliseconds);
    return;
  }
  
  // PRINT DATA:
  printBuffer();
  // printDelta();
  // printMagnitude();
  // Serial.println();
  
  // USE DELTA:
  // For now, use 1500 as delta ceiling.
  // float scale = getDelta() / 1500.0;

  // USE VECTOR:
  // LED color takes a value from 0.0 to 1.0. Calculate scale from the current vector.

  // Resting vector (0.0).
  double calibration = 925.0;
  // Largest vector needed to hit max color (1.0).
  double upperBound = 1600.0;
  
  double normalizedVector = abs(calibration - getMagnitude());
  
  double scale = normalizedVector / upperBound;
  
  Serial.print("n "); Serial.print((int)normalizedVector);
  Serial.print("\t s: "); Serial.println(scale);
  
  // Change LED color.
  showColor(scale, BRIGHTNESS * (scale + 0.2));
  // transitionToColor(scale, 0.2, 5);
  
  // Advance the buffer.
  if (++bufferPosition >= bufferSize()) {
    bufferPosition = 0;
  }
  
  delay(delayMilliseconds);
}

//////////

// Gets the average difference between the latest buffer and previous buffer.
int getDelta() {
  AccelReading previousReading = getPreviousReading();
  AccelReading currentReading  = getCurrentReading();
  
  int deltaX = abs(abs(currentReading.x) - abs(previousReading.x));
  int deltaY = abs(abs(currentReading.y) - abs(previousReading.y));
  int deltaZ = abs(abs(currentReading.z) - abs(previousReading.z));
  
  return (deltaX + deltaY + deltaZ) / 3;
}

void printDelta() {
  AccelReading previousReading = getPreviousReading();
  AccelReading currentReading  = getCurrentReading();
  
  int deltaX = abs(abs(currentReading.x) - abs(previousReading.x));
  int deltaY = abs(abs(currentReading.y) - abs(previousReading.y));
  int deltaZ = abs(abs(currentReading.z) - abs(previousReading.z));

  Serial.print(deltaX); Serial.print ("\t");
  Serial.print(deltaY); Serial.print ("\t");
  Serial.print(deltaZ); Serial.print ("\t");
  Serial.print(getDelta()); Serial.println();
}

// Gets the most recent vector magnitude.
// http://en.wikipedia.org/wiki/Euclidean_vector#Length
double getMagnitude() {
  AccelReading currentReading  = getCurrentReading();

  double x = currentReading.x;
  double y = currentReading.y;
  double z = currentReading.z;

  double vector = x * x + y * y + z * z;

  return sqrt(vector);
}

void printMagnitude() {
  Serial.println(getMagnitude());
}

// Prints the latest buffer reading to the screen.
void printBuffer() {
  Serial.print(accelBuffer[bufferPosition].x); Serial.print ("\t");
  Serial.print(accelBuffer[bufferPosition].y); Serial.print ("\t");
  Serial.print(accelBuffer[bufferPosition].z); Serial.println();
}

//////////

// Returns the number of items held by the buffer.
int bufferSize() {
  return sizeof(accelBuffer) / sizeof(accelBuffer[0]);
}

AccelReading getCurrentReading() {
  return accelBuffer[bufferPosition];
}

// Gets the previous buffer reading.
AccelReading getPreviousReading() {
  int previous = bufferPosition - 1;
  if (previous < 0) previous = bufferSize() - 1;
  return accelBuffer[previous];
}

// Returns true if two readings are equal.
bool equalReadings(AccelReading a, AccelReading b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}


///////////
// color //
///////////

int LED_COUNT = 16;
int DATA_PIN = 6;
int CLOCK_PIN = 12;
int COLOR_RANGE = 384;

LPD8806 strip = LPD8806(LED_COUNT, DATA_PIN, CLOCK_PIN);

void colorSetup() {
  // Turn the strip on.
  strip.begin();
  
  // Refresh the strip.
  strip.show();
}

// Sets the strip all one color.
// Scale parameter is a value 0.0 to 1.0,
// representing how far on the rainbow to go.
// Brightness is measured 0.0 to 1.0.
void showColor(float scale, float brightness) {
  int c = COLOR_RANGE * scale;
  // Serial.print("Show "); Serial.print(scale); Serial.println(c);
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color(c, brightness));
  }
  strip.show();
}

// Transitions the strip to all one color.
// Scale parameter is a value 0.0 to 1.0,
// representing how far on the rainbow to go.
// Brightness is measured 0.0 to 1.0.
// Speed is measured in milliseconds per step (5 steps).
void transitionToColor(float scale, float brightness, int speedMilliseconds) {
  static int STEPS = 5;
  static int currentColor = 0;

  int newColor = COLOR_RANGE * scale;
  float step = (newColor - currentColor) / STEPS;

  for (int x = 0; x < STEPS; x++) {
    int stepColor = currentColor + step * x;
    
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, color(stepColor, brightness));
    }
  }
  
  strip.show();
  
  // Store for next time.
  currentColor = newColor;
  
  delay(speedMilliseconds);
}

// Shows the color progression.
void showColors() {
  for (int j = 0; j < 384; j++) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, color(j, 0.5));
    }
    strip.show();
    delay(1);
  }
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  delay(1);
}

// Color 1 from 384; brightness 0.0 to 1.0.
uint32_t color(uint16_t color, float brightness)  {
  byte r, g, b;
  int range = color / 128;
  switch (range) {
    case 0: // Red to Yellow (1 to 128)
      r = 127 - color % 128;
      g = color % 128;
      b = 0;
      break;
    case 1: // Yellow to Teal (129 to 256)
      r = 0;
      g = 127 - color % 128;
      b = color % 128;
      break;
    case 2: // Teal to Purple (257 to 384)
      r = color % 128;
      g = 0;
      b = 127 - color % 128;
      break;
  }
  r *= brightness;
  g *= brightness;
  b *= brightness;
  return strip.Color(r, g, b);
}

void colorOff() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}
