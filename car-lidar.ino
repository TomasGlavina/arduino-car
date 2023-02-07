#include <LiquidCrystal.h>
#include <Wire.h>

#define Motor_L_dir_pin       7
#define Motor_R_dir_pin       8
#define Motor_L_pwm_pin       9
#define Motor_R_pwm_pin       10
#define joyX A8
#define joyY A9
#define trimmer A3
#define power A4

#define pulse_port_right 3
#define pulse_port_left 2
  #define _i2cAddress 0x60
  #define BEARING_Register 2 
  #define ONE_BYTE   1
  #define TWO_BYTES  2
  #define FOUR_BYTES 4
  #define SIX_BYTES  6

byte _byteHigh;
byte _byteLow;

int nReceived;
signed char roll;
signed char pitch;

int bearing;

int target = 260;

//test move to speed

//int speed = 60;

//end test move to speed

int number;
String direc;
const int lWheel = 2;
const int rWheel = 3;
float xMap, yMap, xValue, yValue;
const int buttonPin = 19;
volatile int buttonState;
int left_count, right_count = 0;
int dir;
float arrayOfSpeed[2];
String a;

LiquidCrystal lcd(37, 36, 35, 34, 33, 32);



void setup()
{
  Serial.begin(9600);
  Wire.begin();
  delay(200);
  Serial.print("JollyCooperation\n");
  pinMode(lWheel, INPUT);
  pinMode(rWheel, INPUT);
 
  pinMode(buttonPin, INPUT);
  lcd.begin(20, 4);     // Display size definition 20 char  4 row
  lcd.setCursor(0, 0);
  attachInterrupt(digitalPinToInterrupt(lWheel), lCount, FALLING);
  attachInterrupt(digitalPinToInterrupt(rWheel), rCount, FALLING);
}

void loop()
{
////// Direction
  xValue = analogRead(joyX);
  yValue = analogRead(joyY);
  ReadCompass();
  dir = value_to_dir(xValue);
  value_to_speed(xValue, yValue); 
  digitalWrite(Motor_R_dir_pin, dir);
  digitalWrite(Motor_L_dir_pin, dir);
  analogWrite(Motor_R_pwm_pin, arrayOfSpeed[0]);
  analogWrite(Motor_L_pwm_pin, arrayOfSpeed[1]);
  // lcd.setCursor(0,0); 
  // lcd.print("CMP= ");
  // lcd.print(bearing);
  // lcd.print("");
  // lcd.setCursor(0,1);

  // if(bearing  < 15 || bearing > 350 ){
  // direc ="N";
  // }
  // else if (bearing <45 ) {
  // direc = "NE";
  // }
  // else if (bearing <90) {
  // direc = "E";
  // }
  // else if (bearing <135 ) {
  // direc = "SE";
  // }
  // else if (bearing <180) {
  // direc = "S";
  // }
  // else if (bearing <225) {
  // direc = "SW";
  // }
  // else if (bearing <270) {
  // direc = "W";
  // }
  // else if (bearing <315) {
  // direc = "NW";
  // }
  // //lcd.print(bearing);
  // //lcd.print(" degrees");
  // lcd.print(direc);
  // Serial.print("CMP: ");
  // Serial.println(bearing);
  // delay(100);
  // lcd.clear();

  calibrate_to_cmp(bearing, target);
  //end code to turn to the compass

  //test serial event
  serialEvent();
  //end test serial event
}

void serialEvent() {
  while (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    Serial.println(message); 
    int pos_s = message.indexOf("Move");
    int value_to_turn = message.indexOf("Turn");
    int target_value = message.indexOf("Set target");
    int message_to_lcd = message.indexOf("Say");

    if (pos_s > -1){
      pos_s = message.indexOf(":");
      if (pos_s > -1){
        String stat = message.substring(pos_s + 1);

        if (stat.charAt(0) == '-') {
          number = stat.substring(1).toInt();
          move_backward(number, 250);
        }
        else {
          number = stat.toInt();
          move_forward(number, 250);
        }     

      }
    }
    else if (value_to_turn > -1) {
      value_to_turn = message.indexOf(":");
      if (value_to_turn > -1) {
        String stat = message.substring(value_to_turn + 1);
        if (stat.charAt(0) == '-') {
          number = stat.substring(1).toInt();
          turn_left(number, 250);
        }
        else {
          number = stat.toInt();
          turn_right(number, 250);
        }
      }
    }
    else{
      Serial.println("");
     
    }
    if (target_value > -1) {
      target_value = message.indexOf(":");
      if (target_value > 1) {
        String stat = message.substring(target_value + 1);
        if (stat.charAt(0) == '-') {
          target = 360 - stat.substring(1).toInt();
        }
        else {
          target = stat.substring(0).toInt();
        }
      }
    }

    if (message_to_lcd > -1) {
      message_to_lcd = message.indexOf(":");
      if (message_to_lcd > -1) {
        String stat = message.substring(message_to_lcd + 1);
        lcd.clear();
        lcd.print(stat);
        delay(1000);
        lcd.clear();
      }
    }
  }
}
//End test serial event

int value_to_dir(int value){
  //1023 0 518
  int result;
  if (value > 550) {
    result = 0;
  } else if (value < 500) {
    result = 1;
  } 
  return result;
}

void value_to_speed(float xValue, float yValue) {
  if (xValue < 770 && xValue > 500) {
    if (yValue > 500) {
      arrayOfSpeed[1] = map(yValue, 550, 1023, 10, 250);
      arrayOfSpeed[0] = 0;
    } else if (yValue < 485) {
      arrayOfSpeed[0] = map(yValue, 500, 0, 10, 250);
      arrayOfSpeed[1] = 0;    
    } else if((485 < yValue <500) && (500 < xValue < 600))  {
      arrayOfSpeed[1] = 0;
      arrayOfSpeed[0] = 0;
    }
  } else {
    if (xValue > 770) {
      arrayOfSpeed[0] = arrayOfSpeed[1] = map(xValue, 770, 1023, 10, 250);
    } else if(xValue < 500) {
      arrayOfSpeed[0] = arrayOfSpeed[1] = map(xValue, 500, 0, 10, 250);
    } else {
      arrayOfSpeed[0] = arrayOfSpeed[1] = 0;
    }
  }

}

void ReadCompass()
{
  // Begin communication with CMPS14
  Wire.beginTransmission(_i2cAddress);

  // Tell register you want some data
  Wire.write(BEARING_Register);

  // End the transmission
  int nackCatcher = Wire.endTransmission();

  // Return if we have a connection problem 
  if(nackCatcher != 0){bearing = 0; pitch = 0;  roll = 0; return;}

  // Request 4 bytes from CMPS14
  nReceived = Wire.requestFrom(_i2cAddress , FOUR_BYTES);

  // Something has gone wrong
  if (nReceived != FOUR_BYTES) {bearing = 0; pitch = 0;  roll = 0; return;}

  // Read the values
  _byteHigh = Wire.read(); _byteLow = Wire.read();
  bearing = ((_byteHigh<<8) + _byteLow) / 10;

  // Read the values
  pitch = Wire.read();

  // Read the values


}

float ad_volts(int input)
{
  int meas = analogRead(input); //Reading analog input
  float volts;
  volts = meas*5.0;
  volts = volts/1023.0; // Transform it into voltage

  return volts;
}

void move_forward(int cms, int speed) {
  digitalWrite(Motor_R_dir_pin, 0);
  digitalWrite(Motor_L_dir_pin, 0);  
  while (right_count < 9*cms) {
    analogWrite(Motor_L_pwm_pin, speed);
    analogWrite(Motor_R_pwm_pin, speed);
  }
  right_count = 0;
  digitalWrite(Motor_L_pwm_pin, 0);
  digitalWrite(Motor_R_pwm_pin, 0);
  
}

void move_backward(int cms, int speed) {
  digitalWrite(Motor_R_dir_pin, 1);
  digitalWrite(Motor_L_dir_pin, 1);
  while (right_count < 9*cms) {
    analogWrite(Motor_L_pwm_pin, speed);
    analogWrite(Motor_R_pwm_pin, speed);
  }
  right_count = 0;
  digitalWrite(Motor_L_pwm_pin, 0);
  digitalWrite(Motor_R_pwm_pin, 0);
}

//turn to cmp

void turn_left(int angle, int speed) {
  float radius = angle * PI * 27 / 360 / 2;
  digitalWrite(Motor_R_dir_pin, 0);
  digitalWrite(Motor_L_dir_pin, 1);
  while (left_count < 9*radius) {
    analogWrite(Motor_L_pwm_pin, speed);
    analogWrite(Motor_R_pwm_pin, speed);
  }
  left_count = 0;
  digitalWrite(Motor_L_pwm_pin, 0);
  digitalWrite(Motor_R_pwm_pin, 0);
}

void turn_right(int angle, int speed) {
  float radius = angle * PI * 27 / 360 / 2;
  digitalWrite(Motor_R_dir_pin, 1);
  digitalWrite(Motor_L_dir_pin, 0);
  while (right_count < 9*radius) {
    analogWrite(Motor_L_pwm_pin, speed);
    analogWrite(Motor_R_pwm_pin, speed);
  }
  right_count = 0;
  digitalWrite(Motor_L_pwm_pin, 0);
  digitalWrite(Motor_R_pwm_pin, 0);
}

void calibrate_to_cmp(int direction, int target) {
  if (direction > target) direction -= target;
  else direction = 360 + direction - target;
  target = 0;
  int lVar = 357, rVar = 3;
  if (direction > lVar || direction < rVar) return;
  if (direction > 180) turn_right(2, 60);
  else turn_left(2, 60);
}

//end turn to cmp

void lCount(){
  left_count++;
}

void rCount(){
  right_count++;
}
