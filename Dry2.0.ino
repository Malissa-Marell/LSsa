 #include "CytronMotorDriver.h"


// Configure the motor driver.
CytronMD motor1(PWM_DIR, 14, 12);  // PWM 1 = Pin 14, DIR 1 = Pin 12.
CytronMD motor2(PWM_DIR, 25, 26); // PWM 2 = Pin 9, DIR 2 = Pin 10.

#define rainDigital 2

int cover = 0;

// The setup routine runs once when you press reset.
void setup() {
  Serial.begin(9600);
  pinMode(rainDigital, INPUT);
  int cover = 0;
}


// The loop routine runs over and over again forever.
void loop() {
  int rainDigitalVal = digitalRead(rainDigital);

  // 50% is 128
  // 100% is 255

  if(rainDigitalVal == 0) {

    if (cover == 0) {
      motor2.setSpeed(128);
      motor1.setSpeed(0);
      delay(5000);

      motor2.setSpeed(0);
      motor1.setSpeed(255);
      delay(5000);

      motor2.setSpeed(0);
      motor1.setSpeed(0);
      delay(5000);
      int cover = 1;
    }
    
  } else {
    
    if(cover == 1) {
      motor2.setSpeed(-128);
      motor1.setSpeed(0);
      delay(5000);

      motor2.setSpeed(0);
      motor1.setSpeed(-255);
      delay(5000);

      motor2.setSpeed(0);
      motor1.setSpeed(0);
      delay(5000);
      int cover = 0;
    }

  }
  
  Serial.print(rainDigitalVal);
  Serial.print("\t");
  delay(200);
}