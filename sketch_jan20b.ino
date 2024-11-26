#include <Servo.h>

Servo myServo;
int ServoPin = 9;

int ServoPos;

const int closedPos =180;
const int openPos =0;

const int trigPin =10;
const int echoPin =12;
float travelTime;
float distance;

void setup() {
  // put your setup code here, to run once:
  myServo.attach(ServoPin);
  
  ServoPos = closedPos;
  myServo.write(ServoPos);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
} 

void loop() {
  // put your main code here, to run repeatedly:
  sendSoundWave();
  travelTime = pulseIn(echoPin, HIGH);
  distance = 0.034 * travelTime/2;
  if(distance < 20){
    openTrash();
    delay(5000);
  }
  else{
    closeTrash();
  }
}

void openTrash() {
  while( ServoPos> openPos) {
    ServoPos--;
    myServo.write(ServoPos);
    delay(70);
  }
}

void closeTrash() {
  while( ServoPos> closedPos) {
    ServoPos++;
    myServo.write(ServoPos);
    delay(70);
  }
}

void sendSoundWave() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(10);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
}
