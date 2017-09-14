//Headers included int this program were written by Adafruit.

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define LAT A3
#define OE  9
#define A   A0
#define B   A1
#define C   A2
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

unsigned short usartValue;
unsigned char previousValue;
unsigned char inputValue;
unsigned char r, g, b;
int inputPin = 10;
const char hours[] PROGMEM = "Hours";
const char minutes[] PROGMEM = "Min";
const char error[] PROGMEM = "Error";
const char inputError[] PROGMEM = "IError";

void setup() {
  Serial.begin(9600);
  pinMode(inputPin, INPUT);
  matrix.begin();
  matrix.setTextSize(1);
}

void loop() {
  inputValue = digitalRead(inputPin);
  if (inputValue == 1){// changes to weather
    if (Serial.available()) {
      matrix.fillScreen(0);//clears screen
      usartValue = Serial.read();//reads temperaure from atmega
      changeColor();//changes color depending on temp
      matrix.setTextColor(matrix.Color333(r,g,b));
      matrix.setTextSize(2);
      matrix.setCursor(1, 0);
      if (usartValue - previousValue < 5 || usartValue - previousValue > -5){
        matrix.print(usartValue);//prints temperature to matrix
      }
      delay(100);
      // Update display
      matrix.swapBuffers(false);
    }
  }
  else{//changes to alarm
   if (Serial.available()) {
      matrix.fillScreen(0);//clears screen
      usartValue = Serial.read();//reads temperaure from atmega
      matrix.setCursor(1, 0);
      matrix.setTextSize(1);
     printAlarm(usartValue);
      delay(100);
      // Update display
      matrix.swapBuffers(false);
   }
  } 
  previousValue = usartValue;
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
void printAlarm(short uv){
  if (uv == 200){
      matrix.print(F2(hours));
}
  else if(uv == 201){
      matrix.print(F2(minutes));
  }
  else if(uv == 220){
      matrix.print(F2(error));
  }
  else if(uv == 150){
        matrix.fillScreen(matrix.Color333(7,3,1));
  }
//  else
//    matrix.print(F2(inputError));
}

