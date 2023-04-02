/* 
 Thermostat set by potentiometer with display
 Controlled by an Arduino Nano
 
 Hardware:
 - 2x 7-Segment LED
 - 10k Potentiometer
 - 10k Thermistor
 - 10k Resistor
 - Relay PCB
 */

#include <Arduino.h>
#include "SevSeg.h"

SevSeg sevseg;

int ThermistorPin = A1;
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

int mosfetPin = 10;

int potentiometerPin = A0;
int potentiometerValue = 0;
int targetTemp = 0;
int newTargetTemp = 0;

int roomTemp = 22;
int tempOffset = 40;

void setup() {
  byte numDigits = 2;
  byte digitPins[] = {11,12};
  byte segmentPins[] = {2,3,4,5,6,7,8};
  bool resistorsOnSegments = false;
  byte hardwareConfig = COMMON_CATHODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = true;

  sevseg.begin(
    hardwareConfig,
    numDigits,
    digitPins,
    segmentPins,
    resistorsOnSegments,
    updateWithDelays,
    leadingZeros,
    disableDecPoint
  );
  sevseg.setBrightness(90);

  pinMode(mosfetPin, OUTPUT);
  pinMode(potentiometerPin, INPUT);
  digitalWrite(mosfetPin, LOW);
  
  potentiometerValue = analogRead(potentiometerPin);
  targetTemp = newTargetTemp = map(potentiometerValue, 0, 1023, roomTemp, 99);
}

unsigned int showTargetTempDuration = 2500;
unsigned long showTargetTempStartTime;

// cooldown time for mosfet switching
unsigned int mosfetDebounceTimespan = 2000;
unsigned int mosfetDebounceStartTime;
boolean mosfetDebounceState = false;
const int NOT_DEBOUNCING = 0;
const int DEBOUNCING = 1;

void loop() {
  if ((millis()%100) == 0) {
    Vo = analogRead(ThermistorPin);
    R2 = R1 * (1023.0 / (float)Vo - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    T = T - 273.15;

    // Read potentiometer value and project it onto the adjustable range
    potentiometerValue = analogRead(potentiometerPin);
    targetTemp = map(potentiometerValue, 0, 1023, roomTemp + tempOffset, 99);

    // Show adjusted temperature for X seconds on change, otherwise show current measurement
    if(targetTemp != newTargetTemp) {
          showTargetTempStartTime = millis();
    }

    if(millis() < (showTargetTempStartTime + showTargetTempDuration)) {
      newTargetTemp = targetTemp;
      sevseg.setNumber(targetTemp);
    } else {
      sevseg.setNumber(T);
    }

    // Control mosfet with cooldown time for relay triggering
    if(T < targetTemp) {
      if(mosfetDebounceState == DEBOUNCING) {
        if(millis() > (mosfetDebounceStartTime + mosfetDebounceTimespan)) {
          mosfetDebounceState = NOT_DEBOUNCING;
        }
      } else {
        digitalWrite(mosfetPin, HIGH);
        mosfetDebounceState = DEBOUNCING;
        mosfetDebounceStartTime = millis();
      }
    } else {
      if(mosfetDebounceState == DEBOUNCING) {
        if(millis() > mosfetDebounceStartTime + mosfetDebounceTimespan) {
          mosfetDebounceState = NOT_DEBOUNCING;
        }
      } else {
        mosfetDebounceState = DEBOUNCING;
        mosfetDebounceStartTime = millis();
        digitalWrite(mosfetPin, LOW);
      }
    }
  }
  
  sevseg.refreshDisplay();
}