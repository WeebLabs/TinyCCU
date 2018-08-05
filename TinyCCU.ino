// This sketch enables very precise control over a variety of parameters for Blackmagic cinema cameras, using the Blackmagic Arduino Shield
// in conjunction with a quadratic rotary encoder and SH1106 OLED display.

// Please feel free to modify and redistribute this code as you see fit but credit is always greatly appreciated.

// Author: Haru-tan | Otaku+ | Troy Dunn-Higgins 

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <Wire.h>
#include <BMDSDIControl.h>
#include <Rotary.h>

// Declare variables for previous milliseconds and the execution intervals //
unsigned long oMillis = 0;
const long intervalTurn = 1000;
const long intervalActivate = 250;

// Declare variables for all settings and an integer for menu item selection //
int menuSelection = 0;
int WB = 5500;
int ISO = 8;
int avDisplay = 0;
float AV = 0.00;
float FOCUS = 0.5;
float ANGLE = 0;
String distFocus = "PUSH";

// Declare variables for the pin to which the encoder button will be connected, number of presses, current state and previous state //
int buttonPin = 4;
int buttonPushCounter = 0;
int buttonState = 0;
int lastButtonState = 0;

// Declare variables for current and previous encoder values //
int oldEncoderValue = 0;
int currentEncoderValue = 0;

// Handles the shutter speed selection and translates absolute logic level micro-second values into display-friendly degree values //
int angleSelection = 3;
int angleDisplay[6] = {45,90,172,180,270,360};
int32_t angleLogic[6] = {5208,10417,20000,20833,31250,41667};

// Initializes the OLED library //
#define OLED_RESET 4
Adafruit_SH1106 display(OLED_RESET);

// Specifies the Blackmagic shield's I2C address and initializes the library //
const int shieldAddress = 0x6E;
BMD_SDICameraControl_I2C sdiCameraControl(shieldAddress);

// Initializes the rotary encoder library, specifies the input pins and declares the relative position counter variable //
Rotary rotary = Rotary(3, 2);
int counter =0;

void setup() {
  Serial.begin(9600);

  // Initializes the OLED display, clears the buffer and then displays it //
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  pinMode(buttonPin, INPUT);

  // Initializes camera control and ATEM overrides //
  sdiCameraControl.begin();
  sdiCameraControl.setOverride(true);

  // Send default setting values to the camera, ensuring seamless control //
  sdiCameraControl.writeCommandInt16(1,1,2,0,WB);
  sdiCameraControl.writeCommandFixed16(1,0,3,0,AV);
  sdiCameraControl.writeCommandInt8(1,1,1,0,ISO);

  // Interrupt handling for the rotary encoder //
  attachInterrupt(0, rotate, CHANGE);
  attachInterrupt(1, rotate, CHANGE);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(40,25);
  display.println("Andromeda");
  display.setCursor(33,35);
  display.println("Electronics");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(23,20);
  display.println("TinyCCU");
  display.setCursor(54,40);
  display.setTextSize(1);
  display.println("v1.1");
  display.display();
  delay(3000);
  display.clearDisplay();
}

void loop() {
  // Read the current encoder, button and millisecond values //
  currentEncoderValue = counter; //Rotary library testing
  buttonState = digitalRead(buttonPin);
  unsigned long nMillis = millis();

  // Handle menu item selection and value adjustment, employing more if() statements than your parents when you were a teenager // 
  if(buttonState != lastButtonState && buttonState == HIGH) {buttonPushCounter+=1;}
  
  if(buttonPushCounter == 0 && currentEncoderValue > oldEncoderValue) {menuSelection+=1;}
  if(buttonPushCounter == 0 && currentEncoderValue < oldEncoderValue) {menuSelection-=1;}
  if(menuSelection > 6) {menuSelection = 0;}
  if(menuSelection < 0) {menuSelection = 6;}
  
  if(menuSelection == 0 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1 && WB < 9900) {WB+=100; changeWB();}
  if(menuSelection == 0 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1 && WB > 2500) {WB-=100; changeWB();}
  
  if(menuSelection == 1 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1 && ISO < 16) {ISO*=2; changeISO();}
  if(menuSelection == 1 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1 && ISO > 2) {ISO/=2; changeISO();}

  if(menuSelection == 2 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1) {AV-=0.02; changeAV();}
  if(menuSelection == 2 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1) {AV+=0.02; changeAV();}

  if(menuSelection == 3 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1) {FOCUS=0.012; distFocus = "FAR"; changeFOCUS(); nMillis=millis(); oMillis=millis();}
  if(menuSelection == 3 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1) {FOCUS=-0.012; distFocus = "NEAR"; changeFOCUS();}
  if(menuSelection == 3 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1) {nMillis=millis(); oMillis=millis();}
  if(menuSelection == 3 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1) {nMillis=millis(); oMillis=millis();}
  if(menuSelection == 3 && currentEncoderValue == oldEncoderValue && buttonPushCounter == 1 && nMillis - oMillis >= intervalTurn) {distFocus = "TURN"; oMillis = nMillis;}
  
  if(menuSelection == 4 && currentEncoderValue > oldEncoderValue && buttonPushCounter == 1 && angleSelection < 5) {angleSelection+=1; changeANGLE();}
  if(menuSelection == 4 && currentEncoderValue < oldEncoderValue && buttonPushCounter == 1 && angleSelection > 0) {angleSelection-=1; changeANGLE();}

  if(menuSelection == 5 && buttonPushCounter == 1) {triggerAF(); buttonPushCounter = 0;}

  if(menuSelection == 6 && buttonPushCounter == 1) {triggerIRIS(); buttonPushCounter = 0;}

  if(buttonPushCounter == 2) {buttonPushCounter = 0; distFocus="PUSH";}

  // Constrain the aperture value because the above logic does not prevent it from clipping below zero occasionally //
  AV = constrain(AV, 0.0, 1.0);

  // Display the results of the above logic //
  menuItems();

  // Update button state and relative encoder handling for next trip through this loop //
  oldEncoderValue = currentEncoderValue;
  lastButtonState = buttonState;  
}

// Display the selected menu item its corresponding variable, drawing a one-pixel border around the OLED display to indicate that a given setting has been selected //
void menuItems() {
  switch (menuSelection) {
    case 0:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(55,7);
    display.println("WB");
    display.setTextSize(4);
    display.setCursor(7,30);
    display.print(WB);
    display.println("K");
    if(buttonPushCounter == 1) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 1:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(47,7);
    display.println("ISO");
    display.setTextSize(4);
    display.setCursor(31,30);
    if(ISO*100 == 1600) {display.setCursor(17,30);}
    display.print(ISO*100);
    if(buttonPushCounter == 1) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 2:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(54,7);
    display.println("AV");
    display.setTextSize(4);
    display.setCursor(28,30);
    avDisplay = map(AV/1.00*100, 100, 0, 1, 100);
    if(avDisplay < 100) {display.setCursor(42,30);}
    if(avDisplay < 10) {display.setCursor(54,30);}
    display.print(avDisplay);
    if(buttonPushCounter == 1) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 3:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(36,7);
    display.println("FOCUS");
    display.setTextSize(4);
    display.setCursor(19,30);
    if(distFocus == "FAR") {display.setCursor(31,30);}
    display.print(distFocus);
    //display.println("%");
    if(buttonPushCounter == 1) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 4:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(36,7);
    display.println("ANGLE");
    display.setTextSize(4);
    display.setCursor(29,30);
    if(angleDisplay[angleSelection] < 100) {display.setCursor(42,30);}
    display.print(angleDisplay[angleSelection]);
    if(buttonPushCounter == 1) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 5:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(56,7);
    display.println("AF");
    display.setTextSize(4);
    display.setCursor(19,30);
    display.print("PUSH");
    if(buttonState == HIGH) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;

    case 6:
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(41,7);
    display.println("IRIS");
    display.setTextSize(4);
    display.setCursor(19,30);
    display.print("PUSH");
    if(buttonState == HIGH) {display.drawRect(0, 0, display.width(), display.height(), WHITE);}
    display.display();
    break;
  }
}

// Send each of the setting values to the camera when instructed to do so by the loop logic //
void triggerAF() {
  sdiCameraControl.writeCommandVoid(1,0,1);
}

void triggerIRIS() {
  sdiCameraControl.writeCommandVoid(1,0,5);
}

void changeAV() {
  sdiCameraControl.writeCommandFixed16(1,0,3,0,AV);
}

void changeISO() {
  sdiCameraControl.writeCommandInt8(1,1,1,0,ISO);
}

void changeFOCUS() {
  sdiCameraControl.writeCommandFixed16(1,0,0,1,FOCUS);
}

void changeWB() {
  sdiCameraControl.writeCommandInt16(1,1,2,0,WB);
}

void changeANGLE() {
  sdiCameraControl.writeCommandInt32(1,1,5,0,angleLogic[angleSelection]);
}

// Increment and decrement the rotary encoder's relative position when it changes //
void rotate() {
  unsigned char result = rotary.process();
  if (result == DIR_CW) {
    counter++;
  } else if (result == DIR_CCW) {
    counter--;
  }
}

