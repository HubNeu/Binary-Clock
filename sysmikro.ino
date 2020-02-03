//libraries:
#include <Wire.h>
#include "RTClib.h"
#include "LedControl.h"
#include <SoftwareSerial.h>
//Variables://///////////////////////////////////////////////////////////
LedControl lc = LedControl(12, 10, 11, 1); //12 is DataIn, 11 is CLK, 10 is LOAD
DS1307 rtc;
DateTime now;
byte bYear, bMonth, bDay, bHour, bMinute, bSecond, nowSecond = 0, lastSecond = 0;
int currentDirection = 0;
SoftwareSerial btSerial(6, 5); //RX,TX
String input = "";
char chr;
int counter;
bool flagTimeUpdate;
//flag for display control
bool buttonFlag = true;
//button with interrupts
const int buttonPin = 2;
//Screens:
byte ledMatrix[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte screenFour[8] = {
  B11111111,
  B10001001,
  B10010001,
  B10100001,
  B10111101,
  B10001001,
  B10001001,
  B11111111,
};
byte screenThree[8] = {
  B11111111,
  B10111001,
  B10000101,
  B10001001,
  B10000101,
  B10000101,
  B10111001,
  B11111111,
};
byte screenTwo[8] = {
  B11111111,
  B10011001,
  B10100101,
  B10001001,
  B10010001,
  B10111101,
  B10000001,
  B11111111,
};
byte screenOne[8] = {
  B11111111,
  B10001001,
  B10011001,
  B10101001,
  B10001001,
  B10001001,
  B10001001,
  B11111111,
};
//Main functions:////////////////////////////////////////////////////////
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(38400);
  Serial.println("Client-side works");
  // set the data rate for the SoftwareSerial port
  btSerial.begin(9600);
  Serial.println("BTside works too.");
  //LedMatrix setup
  lc.shutdown(0, false);
  lc.setIntensity(0, 4);//You can change the intensity from 0 to 15
  lc.clearDisplay(0);
  //button setup
  attachInterrupt(digitalPinToInterrupt(2), stateChange, FALLING);
  //Setting up RTC
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin();
#endif
  rtc.begin();
  //Delete following lines after the time has been set
  bool forceRestart = false;
  if (! rtc.isrunning() || forceRestart) {
    //Serial.println(rtc.isrunning());
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(__DATE__, __TIME__));
    forceRestart = false;
  } else {
    Serial.println("RTC is running!");
  }
  //Delete to this line
  now = rtc.now();
  Serial.print("It's:");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" of day:");
  Serial.print(now.day());
  Serial.print(" of month:");
  Serial.print(now.month());
  Serial.print(" of year:");
  Serial.println(now.year());
}

void loop() {
  //Check bluetooth://///////////////////////////////////////////////////////
  /*if (btSerial.available()) {
    Serial.write(btSerial.read());
    }
    if (Serial.available()) {
    btSerial.write(Serial.read());
    }
  */
  if (btSerial.available()) {
    //read the values transmitted, cast them to char and add to a string for a
    chr = btSerial.read();
    //Serial.print(chr);
    //when chr=0, then it's the end of a transmission
    input += chr;
    counter++;
    if (chr == 0 ||  input.length() == 20) {
      Serial.println("EoT:");
      Serial.println(input);
      flagTimeUpdate=true;
      parseInput(input);
      input = "";
    }
  }
  //Calculate clock and display it://////////////////////////////////////////
  now = rtc.now();
  nowSecond = now.second();
  if (nowSecond > lastSecond || (nowSecond == 0 && lastSecond == 59 || flagTimeUpdate)) { 
    Serial.println("Second passed");
    /* Serial.print("It's: ");
      Serial.print(now.hour());
      Serial.print(" : ");
      Serial.print(now.minute());
      Serial.print(" : ");
      Serial.println(now.second());*/
    calculateDisplay((byte)(now.year() - 2000), now.month(), now.day(), now.hour(), now.minute(), now.second(), currentDirection);
    if (!buttonFlag) {
      displayMatrix();
    }
    buttonFlag = false;
    flagTimeUpdate=false;
    lastSecond = nowSecond;
  }
}
//Other functions://///////////////////////////////////////////////////////////////
void displayMatrix() {
  if (currentDirection == 1 || currentDirection == 3) {
    //This one reverses the image left to right
    lc.setRow(0, 0, ledMatrix[0]);
    lc.setRow(0, 1, ledMatrix[1]);
    lc.setRow(0, 2, ledMatrix[2]);
    lc.setRow(0, 3, ledMatrix[3]);
    lc.setRow(0, 4, ledMatrix[4]);
    lc.setRow(0, 5, ledMatrix[5]);
    lc.setRow(0, 6, ledMatrix[6]);
    lc.setRow(0, 7, ledMatrix[7]);
  } else {
    //This one doesn't
    lc.setColumn(0, 0, ledMatrix[0]);
    lc.setColumn(0, 1, ledMatrix[1]);
    lc.setColumn(0, 2, ledMatrix[2]);
    lc.setColumn(0, 3, ledMatrix[3]);
    lc.setColumn(0, 4, ledMatrix[4]);
    lc.setColumn(0, 5, ledMatrix[5]);
    lc.setColumn(0, 6, ledMatrix[6]);
    lc.setColumn(0, 7, ledMatrix[7]);
  }
}
void stateChange() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200)
  {
    ++currentDirection;
    if (currentDirection > 3)currentDirection = 0;
    displayChangeStatus(currentDirection);
    //Serial.print("Pressed button, current Direction: ");
    //Serial.println(currentDirection);
  }
  last_interrupt_time = interrupt_time;
}
void calculateDisplay(byte argYear, byte argMonth, byte argDay, byte argHour, byte argMinute, byte argSecond, int argDirection) {
  /*
    direction=0->one under the other aligned to right
    direction=1->one under the other aligned to left
    direction=2->Horizontal from top to bot
    direction=3->Horizontal from bot to top
    direction=4->Horizontal from right with the year at the top optional (unimplemented, too much work with this implementation)
    Every one has to be preceeded with a numeral representing the form of displaying
    Year has to be calculated like: now.year()-2000=toByte, taken care in void loop{}
    C doesn't allow to return an array from a function so it would have to be done by pointers but I dislike them and don't support them
  */
  switch (argDirection) {
    case 0:
      calculateDisplayToRight(argYear, argMonth, argDay, argHour, argMinute, argSecond);
      shiftNumbers(0);
      break;
    case 1:
      //We don't have to calculate the bitReverse as the function setColumn already reverses the display
      calculateDisplayToLeft(argYear, argMonth, argDay, argHour, argMinute, argSecond);
      shiftNumbers(1);
      break;
    case 2:
      calculateDisplayToLeft(argYear, argMonth, argDay, argHour, argMinute, argSecond);
      shiftNumbers(2);
      break;
    case 3:
      calculateDisplayToRight(argYear, argMonth, argDay, argHour, argMinute, argSecond);
      shiftNumbers(3);
      break;
    default:
      //Shouldn't happen, just for safety
      calculateDisplayToRight(argYear, argMonth, argDay, argHour, argMinute, argSecond);
      shiftNumbers(0);
  }
}
void shiftNumbers(int argDirection) {
  switch (argDirection) {
    case 0:
      for (int i = 0; i < 8; i++) {
        ledMatrix[i] = ledMatrix[i] << 1;
      }
      //Serial.print("shofted for dir 1");
      break;
    case 1:
      for (int i = 0; i < 8; i++) {
        ledMatrix[i] = ledMatrix[i] >> 1;
      }
      //Serial.print("shofted for dir 2");
      break;
    case 2:
      for (int i = 0; i < 8; i++) {
        ledMatrix[i] =  ledMatrix[i] >> 1;
      }
      //Serial.print("shofted for dir 3");
      break;
    case 3:
      for (int i = 0; i < 8; i++) {
        ledMatrix[i] = ledMatrix[i] << 1;
      }
      //Serial.print("shofted for dir 4");
      break;
  }
}
void calculateDisplayToRight(byte argYear, byte argMonth, byte argDay, byte argHour, byte argMinute, byte argSecond) {
  ledMatrix[0] = 0;
  ledMatrix[1] = argYear;
  ledMatrix[2] = argMonth;
  ledMatrix[3] = argDay;
  ledMatrix[4] = argHour;
  ledMatrix[5] = argMinute;
  ledMatrix[6] = argSecond;
  ledMatrix[7] = 0;
}
void calculateDisplayToLeft(byte argYear, byte argMonth, byte argDay, byte argHour, byte argMinute, byte argSecond) {
  ledMatrix[0] = 0;
  ledMatrix[1] = bitReverse(argYear);
  ledMatrix[2] = bitReverse(argMonth);
  ledMatrix[3] = bitReverse(argDay);
  ledMatrix[4] = bitReverse(argHour);
  ledMatrix[5] = bitReverse(argMinute);
  ledMatrix[6] = bitReverse(argSecond);
  ledMatrix[7] = 0;
}
void displayChangeStatus (byte argByte) {
  switch (argByte++) {
    case 0:
      lc.setColumn(0, 7, screenOne[0]);
      lc.setColumn(0, 6, screenOne[1]);
      lc.setColumn(0, 5, screenOne[2]);
      lc.setColumn(0, 4, screenOne[3]);
      lc.setColumn(0, 3, screenOne[4]);
      lc.setColumn(0, 2, screenOne[5]);
      lc.setColumn(0, 1, screenOne[6]);
      lc.setColumn(0, 0, screenOne[7]);
      break;
    case 1:
      lc.setColumn(0, 7, screenTwo[0]);
      lc.setColumn(0, 6, screenTwo[1]);
      lc.setColumn(0, 5, screenTwo[2]);
      lc.setColumn(0, 4, screenTwo[3]);
      lc.setColumn(0, 3, screenTwo[4]);
      lc.setColumn(0, 2, screenTwo[5]);
      lc.setColumn(0, 1, screenTwo[6]);
      lc.setColumn(0, 0, screenTwo[7]);
      break;
    case 2:
      lc.setColumn(0, 7, screenThree[0]);
      lc.setColumn(0, 6, screenThree[1]);
      lc.setColumn(0, 5, screenThree[2]);
      lc.setColumn(0, 4, screenThree[3]);
      lc.setColumn(0, 3, screenThree[4]);
      lc.setColumn(0, 2, screenThree[5]);
      lc.setColumn(0, 1, screenThree[6]);
      lc.setColumn(0, 0, screenThree[7]);
      break;
    case 3:
      lc.setColumn(0, 7, screenFour[0]);
      lc.setColumn(0, 6, screenFour[1]);
      lc.setColumn(0, 5, screenFour[2]);
      lc.setColumn(0, 4, screenFour[3]);
      lc.setColumn(0, 3, screenFour[4]);
      lc.setColumn(0, 2, screenFour[5]);
      lc.setColumn(0, 1, screenFour[6]);
      lc.setColumn(0, 0, screenFour[7]);
      break;
  }
  buttonFlag = true;
}
byte bitReverse( byte x ) {
  //          01010101  |         10101010
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  //          00110011  |         11001100
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  //          00001111  |         11110000
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;
}
void setTime(String argString) {
  byte numbers[3] = {0, 0, 0};
  //String cast = argString.substring(0, 20);
  int ind = 0, number = 0;
  //Serial.print("The cast:");
  //Serial.println(cast);
  //Serial.println(argString.length());
  for (int i = 3; i < argString.length(); i++) {
    if (argString[i] == ':') {
      ind++;
      number = 0;
    } else {
      number++;;
      numbers[ind] = numbers[ind] * 10 + (byte)(argString.charAt(i) - 48);
    }
  }
  //Set the dates:
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), numbers[0], numbers[1], numbers[2]));
  now = rtc.now();
  Serial.println(numbers[0]);
  Serial.println(numbers[1]);
  Serial.println(numbers[2]);
  Serial.print("It's:");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print("of day:");
  Serial.print(now.day());
  Serial.print("of month:");
  Serial.print(now.month());
  Serial.print("of year:");
  Serial.println(now.year());
}
void parseInput(String argString) {
  Serial.println("parsing input...");
  if (argString.startsWith("st")) {
    setTime(argString);
    //Serial.println("setTime");
  } else if (argString.startsWith("sd")) {
    Serial.println("before setDate(arg)");
    setDate(argString);
    Serial.println("after setDate(arg)");
    //Serial.println("setDate");
  } else {
    Serial.println("Input not recognized!");
  }
}
void setDate(String argString) {
  Serial.println("In setDate");
  byte numbers[3] = {0, 0, 0};
  //String cast = argString.substring(0, 20);
  int ind = 0;
  //Serial.print("The cast:");
  //Serial.println(cast);
  //Serial.println(argString.length());
  for (int i = 3; i < argString.length(); i++) {
    if (argString[i] == ':') {
      ind++;
    } else {
      //add a check for numbers between 0 and 9 and a counter for going too far
      numbers[ind] = numbers[ind] * 10 + (byte)(argString.charAt(i) - 48);
    }
  }
  //Set the dates:
  rtc.adjust(DateTime(numbers[0], numbers[1], numbers[2], now.hour(), now.minute(), now.second()));
  Serial.println("passed time adjust");
  now = rtc.now();
  Serial.println(numbers[0]);
  Serial.println(numbers[1]);
  Serial.println(numbers[2]);
  Serial.print("It's:");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print("of day:");
  Serial.print(now.day());
  Serial.print("of month:");
  Serial.print(now.month());
  Serial.print("of year:");
  Serial.println(now.year());
  Serial.println("passed time adjust");
}
