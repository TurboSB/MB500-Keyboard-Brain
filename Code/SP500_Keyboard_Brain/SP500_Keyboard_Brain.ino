/*  SP500_Keyboard_Brain
    Replacement Controller for Korg SP500 keybed
    Using Teensy 4.1

    written by TurboSB 2021
*/

#include "MultiMap.h"
#include <MIDI.h>

//Setup Midi
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const uint8_t channel = 1;

//Key Matrix Pin Assignments
const uint8_t keyPins[8] = {2, 3, 4, 5, 6, 7, 8, 9}; //key 1,2,3,4,5,6,7,8
const uint8_t groupPins[22] = {11, 10, 24, 12, 26, 25, 28, 27, 30, 29, 32, 31, 34, 33, 36, 35, 38, 37, 40, 39, 14, 41}; //{G1a,G1b,G2a,G2b,G3a,G3b,G4a,G4b,G5a,G5b,G6a,G6b,G7a,G7b,G8a,G8b,G9a,G9b,G10a,G1l0b,G11a,G11b}

//Key Matrix Note Values
const uint8_t keys[8][22] = {
  //{G1a,G1b,G2a,G2b,G3a,G3b,G4a,G4b,G5a,G5b,G6a,G6b,G7a,G7b,G8a,G8b,G9a,G9b,G10a,G10b,G11a,G11b}
  {21, 21, 29, 29, 37, 37, 45, 45, 53, 53, 61, 61, 69, 69, 77, 77, 85, 85, 93, 93, 101, 101}, //key1
  {22, 22, 30, 30, 38, 38, 46, 46, 54, 54, 62, 62, 70, 70, 78, 78, 86, 86, 94, 94, 102, 102}, //key2
  {23, 23, 31, 31, 39, 39, 47, 47, 55, 55, 63, 63, 71, 71, 79, 79, 87, 87, 95, 95, 103, 103}, //key3
  {24, 24, 32, 32, 40, 40, 48, 48, 56, 56, 64, 64, 72, 72, 80, 80, 88, 88, 96, 96, 104, 104}, //key3
  {25, 25, 33, 33, 41, 41, 49, 49, 57, 57, 65, 65, 73, 73, 81, 81, 89, 89, 97, 97, 105, 105}, //key5
  {26, 26, 34, 34, 42, 42, 50, 50, 58, 58, 66, 66, 74, 74, 82, 82, 90, 90, 98, 98, 106, 106}, //key6
  {27, 27, 35, 35, 43, 43, 51, 51, 59, 59, 67, 67, 75, 75, 83, 83, 91, 91, 99, 99, 107, 107}, //key7
  {28, 28, 36, 36, 44, 44, 52, 52, 60, 60, 68, 68, 76, 76, 84, 84, 92, 92, 100, 100, 108, 108}, //key8
};

//Damper Pins
const uint8_t damperPin = 16;
const uint8_t damperCC = 64;
bool damperState = false;

//TODO: add adjustable velocity curve
const unsigned long maxTime = 250000; //time at which velocity goes to 0
const unsigned long minTime = 500; //time at which velocity goes to 127
int velocityCurveIn[11] = {0, 4, 8, 16, 18, 29, 42, 64, 80, 105, 127};
int velocityCurveOut[11] = {0, 3, 6, 13, 15, 25, 38, 60, 76, 103, 127};
int velocityMapSize = 11;

//array of times to calculate velocity
unsigned long keyTimes[128];

//array of states of each key (0 = off, 1 = A pressed, 2 = A+B Pressed)
uint8_t keyStates[128];
uint8_t noteStates[128];

bool LED = LOW;

void setup() {
  MIDI.begin();
  //set key pins as inputs
  for (uint8_t i = 0; i < 8; i++)
  {
    pinMode(keyPins[i], INPUT_PULLUP);
  }
  //set group pins as outputs and set them high
  for (uint8_t i = 0; i < 22; i++)
  {
    pinMode(groupPins[i], OUTPUT);
    digitalWrite(groupPins[i], true);
  }
  //set LED as output
  pinMode(13, OUTPUT);
  //set damper pin as input
  pinMode(damperPin, INPUT_PULLUP);
}

void loop() {

  //Arrays to hold key reads
  uint8_t tempA[128], tempB[128];

  //Read keys
  for (uint8_t i = 0; i < 22; i += 2)
  {
    // Read Group A
    digitalWrite(groupPins[i], LOW); //select group
    delayMicroseconds(20); //wait to settle;
    for (uint8_t j = 0; j < 8; j++)
    {
      tempA[keys[j][i]] = !digitalRead(keyPins[j]);
    }
    digitalWrite(groupPins[i], HIGH); //deselect group

    // Read Group B
    digitalWrite(groupPins[i + 1], LOW); //select group
    delayMicroseconds(20); //wait to settle;
    for (uint8_t j = 0; j < 8; j++)
    {
      tempB[keys[j][i + 1]] = !digitalRead(keyPins[j]);
    }
    digitalWrite(groupPins[i + 1], HIGH); //deselect group
  }

  //record time for velocity calc
  unsigned long scanTime = micros();

  for (uint8_t i = 21; i < 109; i++)
  {
    switch (keyStates[i])
    {
      case 0b00:
        if (tempA[i])
        {
          if (tempB[i])
          {
            keyStates[i] = 0b11;
            if (noteStates[i] == 0) {
              noteStates[i]++;
              usbMIDI.sendNoteOn(i, 127, channel);
              MIDI.sendNoteOn(i, 127, channel);
            }
          }
          else
          {
            keyStates[i] = 0b01;
            keyTimes[i] = scanTime;
          }
        }
        else
        {
          if (noteStates[i] > 0)
          {
            noteStates[i]--;
            usbMIDI.sendNoteOff(i, 127, channel);
            MIDI.sendNoteOff(i, 127, channel);
            keyTimes[i] = 0;
          }
        }
        break;
      case 0b01:
        if (tempA[i])
        {
          if (tempB[i] && (noteStates[i] == 0))
          {
            keyStates[i] = 0b11;
            if (noteStates[i] == 0)
            {
              noteStates[i]++;
              unsigned long diff = scanTime - keyTimes[i];
              uint8_t tempVel = map(constrain(diff, 0, maxTime), 0, maxTime, 127, 1);
              tempVel = multiMap<int>(tempVel, velocityCurveIn, velocityCurveOut, velocityMapSize);
              usbMIDI.sendNoteOn(i, tempVel, channel);
              MIDI.sendNoteOn(i, tempVel, channel);
            }
            keyTimes[i] = 0;
          }
        }
        else
        {
          keyStates[i] = 0b00;
          if (noteStates[i] > 0)
          {
            noteStates[i]--;
            unsigned long diff = scanTime - keyTimes[i];
            uint8_t tempVel = map(constrain(diff, 0, maxTime), 0, maxTime, 127, 1);
            tempVel = multiMap<int>(tempVel, velocityCurveIn, velocityCurveOut, velocityMapSize);
            usbMIDI.sendNoteOff(i, tempVel, channel);
            MIDI.sendNoteOff(i, tempVel, channel);
          }
          keyTimes[i] = 0;
        }
        break;
      case 0b11:
        if (tempA[i])
        {
          if (tempB[i])
          {
          }
          else
          {
            keyStates[i] = 0b01;
            keyTimes[i] = scanTime;
          }
        }
        else
        {

          keyStates[i] = 0b00;
          keyTimes[i] = 0;
          if (noteStates[i] > 0)
          {
            noteStates[i]--;
            usbMIDI.sendNoteOff(i, 127, channel);
            MIDI.sendNoteOff(i, 127, channel);
          }
        }
        break;
      default:
        if (noteStates[i] > 0)
        {
          noteStates[i]--;
          keyStates[i] = 0b00;
          keyTimes[i] = 0;
          usbMIDI.sendNoteOff(i, 127, channel);
          MIDI.sendNoteOff(i, 127, channel);
        }
        break;
    }
  }

  bool tempState = digitalRead(damperPin);
  uint8_t tempVal = 0;
  if (tempState != damperState) {
    damperState = tempState;
    if (damperState) {
      tempVal = 127;
    }
    usbMIDI.sendControlChange(damperCC, tempVal, channel);
    MIDI.sendControlChange(damperCC, tempVal, channel);
  }

  // Handle incoming MIDI messages.
  while (usbMIDI.read()) {
  }
}
