#include <LiquidCrystal.h>
#include <Wire.h>
#include <LIDARLite_v4LED.h>

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

float target = 160;

//test move to speed

//int speed = 60;

//end test move to speed

float distance = 20;
float number;
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

LIDARLite_v4LED lidar;

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

  //LIDAR
  if (lidar.begin() == false) {
    Serial.println("Device did not acknowledge! Freezing.");
    while(1);
  }
  Serial.println("LIDAR acknowledged!");  
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
   lcd.setCursor(0,0); 
   lcd.print("CMP= ");
   lcd.print(bearing);
   lcd.print("");
   lcd.setCursor(0,1);

  if(bearing  < 15 || bearing > 350 ){
  direc ="N ";
  }
  else if (bearing <45 ) {
  direc = "NE";
  }
  else if (bearing <90) {
  direc = "E ";
  }
  else if (bearing <135 ) {
  direc = "SE";
  }
  else if (bearing <180) {
  direc = "S ";
  }
  else if (bearing <225) {
  direc = "SW";
  }
  else if (bearing <270) {
  direc = "W ";
  }
  else if (bearing <315) {
  direc = "NW";
  }

   lcd.print(direc);
   Serial.print("CMP: ");
   Serial.println(bearing);

  calibrate_to_cmp(bearing, target);
  //end code to turn to the compass

  //test serial event
  //serialEvent();
  //end test serial event
  //lcd.setCursor(0, 0);
  //lcd.print("Adjusted distance ");
  //lcd.print((int)distance);
  //Serial.print("Distance: ");
  //Serial.println(lidar.getDistance());
  //move_to_object();
  //printArea();
 //alibrate_to_cmp(bearing, 100);
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
          number = stat.substring(1).toFloat();
          move_backward(number, 250);
        }
        else {
          number = stat.toFloat();
          move_forward(number, 250);
        }     

      }
    }
    else if (value_to_turn > -1) {
      value_to_turn = message.indexOf(":");
      if (value_to_turn > -1) {
        String stat = message.substring(value_to_turn + 1);
        if (stat.charAt(0) == '-') {
          number = stat.substring(1).toFloat();
          turn_left(number, 250);
        }
        else {
          number = stat.toFloat();
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
          target = 360 - stat.substring(1).toFloat();
        }
        else {
          target = stat.substring(0).toFloat();
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

void move_forward(float cms, int speed) {
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

void move_backward(float cms, int speed) {
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

void turn_left(float angle, int speed) {
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

void turn_right(float angle, int speed) {
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

void calibrate_to_cmp(int direction, float target) {
  if (direction > target) direction -= target;
  else direction = 360 + direction - target;
  int lVar = target - 3, rVar = target + 3;
  Serial.print(direction);
  if (direction > lVar || direction < rVar) return;
  if (direction > 180) turn_right(5, 60);
  else turn_left(5, 60);
}

//end turn to cmp

//LIDAR

void printArea() {
  float x_side = lidar.getDistance();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("X = ");
  lcd.setCursor(4,0);
  lcd.print(x_side);
  delay(5000);
  
  x_side = (x_side+lidar.getDistance()) / 100;
  lcd.setCursor(4,0);
  lcd.print(x_side);
  delay(5000);
  
  float y_side = lidar.getDistance();
  lcd.setCursor(0,1);
  lcd.print("Y = ");
  lcd.setCursor(4,1);
  lcd.print(y_side);
  delay(5000);
  y_side = (y_side+lidar.getDistance()) / 100;
  lcd.setCursor(4,1);
  lcd.print(y_side);

  delay(5000);
  float z_side = lidar.getDistance();
  lcd.setCursor(0,2);
  lcd.print("Z = ");
  lcd.setCursor(4,2);
  lcd.print(z_side);
    
  z_side = (z_side+lidar.getDistance()) / 100;
  lcd.setCursor(4,2);
  lcd.print(z_side);
  delay(5000);

  float area = x_side * y_side;
  float volum = x_side * y_side * z_side;
  lcd.setCursor(0,3);
  lcd.print("Area:");
  lcd.setCursor(5, 3);
  lcd.print(area);
  lcd.setCursor(12, 3);
  lcd.print("V:");
  lcd.setCursor(15, 3);
  lcd.print(volum);
  delay(5000);
  
}

void move_to_object() {
  float current_distance = lidar.getDistance();
  
  lcd.setCursor(0, 1);
  lcd.print("Measured distance ");
  lcd.print((int)current_distance);
  lcd.setCursor(0, 2);
  if(distance > current_distance) {
    move_backward(((distance - current_distance) < 5 ? (distance - current_distance) : 10), 70);
    lcd.print("Motor status: RUN FW ");
  }
  else if(distance < current_distance) {
    move_forward(((current_distance - distance) < 5 ? (current_distance - distance) : 10), 70);
    lcd.print("Motor status: RUN RET");
  }
  else {
    turn_left(90, 70);
    lcd.print("Motor status: RUN L    ");  
  }
}

//end LIDAR


void lCount(){
  left_count++;
}

void rCount(){
  right_count++;
}
