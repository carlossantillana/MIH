// scrolltext demo for Adafruit RGBmatrixPanel library.
// Demonstrates double-buffered animation on our 16x32 RGB LED matrix:
// http://www.adafruit.com/products/420

// Written by Limor Fried/Ladyada & Phil Burgess/PaintYourDragon
// for Adafruit Industries.
// BSD license, all text above must be included in any redistribution.

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define LAT A3
#define OE  9
#define A   A0
#define B   A1
#define C   A2
// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);
// Double-buffered mode consumes nearly all the RAM available on the
// Arduino Uno -- only a handful of free bytes remain.  Even the
// following string needs to go in PROGMEM:
 
 //included from benripley.com
  unsigned short usartValue;
  unsigned char inputValue;
  unsigned char r, g, b;
  int inputPin = 10;

void setup() {
  Serial.begin(9600);
  pinMode(inputPin, INPUT);
  matrix.begin();
  matrix.setTextSize(2);
}

void loop() {
  inputValue = digitalRead(inputPin);
  if (inputValue){// changes to weather
    if (Serial.available()) {
      matrix.fillScreen(0);//clears screen
      usartValue = Serial.read();//reads temperaure from atmega
      Serial.println(usartValue);
      changeColor();//changes color depending on temp
      matrix.setTextColor(matrix.Color333(r,g,b));
      matrix.setCursor(1, 0);
      matrix.print(usartValue);//prints temp to matrix
      delay(100);
      // Update display
      matrix.swapBuffers(false);
    }
  }
  else{//changes to alarm
    matrix.fillScreen(matrix.Color333(7,3,1));
  }
  
}
void changeColor(){
        if (usartValue <= 50){
        r = 0;
        b = 7;
        g = 0;
      }
      else if (usartValue > 50 && usartValue < 60){
        r = 1;
        b = 6;
        g = 0;
      }
      else if (usartValue > 60 && usartValue < 70){
        r = 2;
        b = 5;
        g = 0;
      }
      else if (usartValue > 70 && usartValue < 80){
        r = 3;
        b = 4;
        g = 0;
      }
      else if (usartValue > 80 && usartValue < 90){
        r = 4;
        b = 2;
        g = 0;
      }
      else if (usartValue >= 90){
        r = 7;
        b = 0;
        g = 0;
      }
      else{
        r = 7;
        b = 7;
        g = 7;
      }
}

