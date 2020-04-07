/**
 * Trun a 2-Button Foot switch to a little MIDI Controller 
 * 
 * Author: Robert Gröber
 * Version: 0.6
 * 
 * Changelog:
 * 0.6: added send CC when switching using the right button
 * 0.5: added 5th mode for sending 4 static MIDI CC messaged with left button and switch between them with the right button
 * 0.4: added 4th mode for sending two static MIDI CC messages
 */

#include <EEPROM.h>

// ON/OFF - Switches on PIN 3 and 4
const int buttonUpPin = 3;
const int buttonDownPin = 4;

// A button on PIN 5 cycles through Modes
const int modeSwitch = 5;

// 1st RGB LED indicates which Mode we are on
const int PIN_LedR = 6;
const int PIN_LedG = 7;
const int PIN_LedB = 8;

// remember current button/switch states
int modeState = 0;
int lastModeState = 0;
int buttonUpState = 0;
int lastUpState = 0;
int buttonDownState = 0;
int lastDownState = 0;

int noteState = LOW;
int note = 0x1E;

// constants defining modes
const int MODE_PROG=1;
const int MODE_SNAP=2;
const int MODE_LOOP=3;
const int MODE_CC=4;
const int MODE_CC4=5;
const int MODE_MAX=MODE_CC4;

// initial mode
//int currentMode=MODE_PROG;

const int MIDI_NOTE = 0x90;

//MIDI Program Change:
const int MIDI_PC = 0xC0; //0xC6 = Chan7; 0xC0 = Chan1

const int MIDI_CC = 0xB0;

// the following variables are unsigned long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 250;

const int minProgram = 1;
//int program = min;
const int maxProgram = 20;

const int minSnap = 0;
//int snap = 0;
const int maxSnap = 7;

int currentLoopMode = 0;
int currentCC4=0;
const int maxCC4=3;

const int saveAddr = 0;
struct Config {
  int program;
  int snap;
  int currentMode;
};

Config config = {
  0,
  0,
  MODE_PROG
};

void led(int r, int g, int b)
{
   analogWrite(PIN_LedR, r);
   analogWrite(PIN_LedG, g);
   analogWrite(PIN_LedB, b);
}

void loadConfig()
{
  EEPROM.get(saveAddr, config);
  if(config.program > maxProgram) config.program=minProgram;
  if(config.program < minProgram) config.program=minProgram;
  if(config.snap > maxSnap) config.snap=minSnap;
  if(config.snap < minSnap) config.snap=minSnap;
  if(config.currentMode > MODE_MAX) config.program=MODE_PROG;
  if(config.currentMode < MODE_PROG) config.program=MODE_PROG;
}

void setup() {
  //  Set MIDI baud rate:
  Serial.begin(31250);// MIDI = 31250
  //Serial.println("hello");

  // initialize the pushbutton pin as an input:
  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);
  pinMode(modeSwitch, INPUT_PULLUP);
  modeState = digitalRead(modeSwitch);
  buttonUpState = digitalRead(buttonUpPin);
  buttonDownState = digitalRead(buttonDownPin);

  // RGB LED as output:
  pinMode(PIN_LedR, OUTPUT);
  pinMode(PIN_LedG, OUTPUT);
  pinMode(PIN_LedB, OUTPUT);

  // load last values from EEPROM
  loadConfig();

  //led(LOW,LOW,HIGH); //start in Blue
  updateModeLed();
}

void loopDebug() {
  config.currentMode++;
  if(config.currentMode>MODE_MAX) config.currentMode=MODE_PROG;

  if(config.currentMode==MODE_PROG) led(LOW,LOW,HIGH);
  if(config.currentMode==MODE_SNAP) led(LOW,HIGH,HIGH);
  if(config.currentMode==MODE_LOOP) led(HIGH,LOW,LOW);
  Serial.println(config.currentMode);
  delay(1000);
}

void updateModeLed()
{
  if(config.currentMode==MODE_PROG) led(0,  0,  255);
  if(config.currentMode==MODE_SNAP) led(127,127,255);
  if(config.currentMode==MODE_LOOP) led(255,0,  0);
  if(config.currentMode==MODE_CC)   led(80, 255,80);
  if(config.currentMode==MODE_CC4)  led(127,255,255);
}

void loop() {
  modeState = digitalRead(modeSwitch);
  buttonUpState = digitalRead(buttonUpPin);
  buttonDownState = digitalRead(buttonDownPin);

  // mode switching
  if(modeState!=lastModeState)
  {
    if(modeState == LOW && (millis() - lastDebounceTime) > debounceDelay)
    {
      config.currentMode++;
      if(config.currentMode>MODE_MAX) config.currentMode=MODE_PROG;

      updateModeLed();
      //Serial.println(currentMode);
      saveConfig();  
    }    
  }

  if(buttonUpState != lastUpState)
  {
    if ((millis() - lastDebounceTime) > debounceDelay) {

      if(config.currentMode==MODE_PROG){ // program mode
        config.program++;
        if(config.program>maxProgram) config.program=minProgram;
        sendProgram(MIDI_PC, config.program);
        saveConfig();
      }
      else if(config.currentMode==MODE_SNAP){ //snapshot mode
        config.snap++;
        if(config.snap>maxSnap) config.snap=minSnap;
        noteOn(MIDI_CC, 0x45, config.snap); // DEZ 69
        saveConfig();
      }else if(config.currentMode==MODE_LOOP){
        noteOn(MIDI_CC, 0x3C, currentLoopMode == 1? 0x00 : 0xff); //Lopper RECORD DEZ 60 = 0x3C
        currentLoopMode=currentLoopMode==1? 0 : 1;
      }else if(config.currentMode==MODE_CC) { // CC Mode
        noteOn(MIDI_CC, 0x53, 0x64);
      }else{ // CC4 Mode
        noteOn(MIDI_CC, 0x53+currentCC4, 0x64);
      }
      
      lastDebounceTime = millis();
    }
  }
  else if(buttonDownState != lastDownState)
  {
    if ((millis() - lastDebounceTime) > debounceDelay) {

      if(config.currentMode==MODE_PROG){ // program mode
        config.program--;
        if(config.program<minProgram) config.program=maxProgram;
        sendProgram(MIDI_PC, config.program);
        saveConfig();
      }
      else if(config.currentMode==MODE_SNAP){ //snapshot mode
        config.snap--;
        if(config.snap<minSnap) config.snap=maxSnap;
        noteOn(MIDI_CC, 0x45, config.snap);
        saveConfig();
      }else if(config.currentMode==MODE_LOOP){
        noteOn(MIDI_CC, 0x3D, 0x0); //Lopper STOP DEZ 61
      }else if(config.currentMode==MODE_CC) { // CC Mode
        noteOn(MIDI_CC, 0x54, 0x64);
      }else{ // MIDI CC4 Mode
        currentCC4++;
        if(currentCC4>maxCC4)currentCC4=0;
        //also send a Midi CC on change
        noteOn(MIDI_CC, 0x99+currentCC4, 0x64);
        if(currentCC4==0) led(127, 255, 255);
        if(currentCC4==1) led(127, 127, 40);
        if(currentCC4==2) led(127, 255, 127);
        if(currentCC4==3) led(0,   0,   200);
      }
      
      lastDebounceTime = millis();
    }
  }
  lastUpState = buttonUpState;
  lastDownState = buttonDownState;
  lastModeState = modeState;
}

//  plays a MIDI note.  Doesn't check to see that
//  cmd is greater than 127, or that data values are  less than 127:
void sendProgram(int cmd, int pitch) {
  Serial.write(cmd);
  Serial.write(pitch);
  //Serial.write(velocity); 
}

void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}

void saveConfig()
{
  EEPROM.put(saveAddr, config); 
}

