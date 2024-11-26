#define rainAnalog 35
#define rainDigital 39
#define DO_PIN 15

#define PIN_IN1  27 // ESP32 pin GPIO27 connected to the IN1 pin L298N
#define PIN_IN2  26 // ESP32 pin GPIO26 connected to the IN2 pin L298N
#define PIN_ENA  14

#define PIN_IN3  18 // ESP32 pin GPIO27 connected to the IN3 pin L298N
#define PIN_IN4  19 // ESP32 pin GPIO26 connected to the IN4 pin L298N
#define PIN_ENB  5

#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6kShyoEuc"
#define BLYNK_TEMPLATE_NAME "DrysusRack"
#define BLYNK_AUTH_TOKEN "vO9NLNh_NHTOsyYTtueX8Td0tRI9Vu89"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//#include <WiFiManager.h> 

int sensorVal;
bool cover = false;
bool vertical = false;

BlynkTimer timer;

char auth[] = "vO9NLNh_NHTOsyYTtueX8Td0tRI9Vu89";
char ssid[] = "Mel";
char pass[] = "112223333";

String get_wifi_status(int status){
    switch(status){
        case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
        case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
        case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
        case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
        case WL_CONNECTED:
        return "WL_CONNECTED";
        case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    }
}

void myTimer() 
{
  // This function describes what will happen with each timer tick
  // e.g. writing sensor value to datastream V5
  //Blynk.virtualWrite(V0, sensorVal);  
}

void setup() {
  Serial.begin(115200);
  
  delay(10);
  WiFi.mode(WIFI_STA);
  int status = WL_IDLE_STATUS;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(get_wifi_status(status));

  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  int timeout_counter = 0;

    while(WiFi.status() != WL_CONNECTED){
        //Serial.print(".");
        Serial.println(get_wifi_status(status));
        delay(600);
        timeout_counter++;
        if(timeout_counter >= WL_CONNECTION_LOST*5){
        ESP.restart();
        }
    }

  Serial.println("WiFi connected");  

  Blynk.begin(auth, ssid, pass);

  // Setting interval to send data to Blynk Cloud to 1000ms. 
  // It means that data will be sent every second
  timer.setInterval(3000, myTimer);
  
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

}
void loop() {

  //sensorVal = digitalRead(DO_PIN);
  timer.run();
  Blynk.run();

  int rainAnalogVal = analogRead(rainAnalog);
  int rainDigitalVal = digitalRead(rainDigital);
  int lightState = digitalRead(DO_PIN);

  if (rainAnalogVal == 0) {
    
    Serial.println("Basah");
    Blynk.virtualWrite(V2, "Wet");

    if (cover == false) {

      digitalWrite(PIN_IN1, HIGH); // control the motor's direction in clockwise
      digitalWrite(PIN_IN2, LOW);

      for (int speed = 0; speed <= 255; speed++) {
        analogWrite(PIN_ENA, speed); // speed up
        delay(10);
      }
      cover = true;
      delay(2000);

      // down
      digitalWrite(PIN_IN3, HIGH);   // control the motor's direction in anti-clockwise
      digitalWrite(PIN_IN4, LOW);

      for (int speed = 0; speed <= 255; speed++) {
        analogWrite(PIN_ENB, speed); // speed up
        delay(10);
      }
      vertical = false;
      delay(2000);
    }

    digitalWrite(PIN_IN1, LOW); // stop
    digitalWrite(PIN_IN2, LOW);

    digitalWrite(PIN_IN3, LOW); // stop
    digitalWrite(PIN_IN4, LOW);

  } else {
    Serial.println("Kering");
    Blynk.virtualWrite(V2, "Dry");

    if (cover == true) {
      // for cover
      digitalWrite(PIN_IN1, LOW);   // control the motor's direction in anti-clockwise
      digitalWrite(PIN_IN2, HIGH);

      for (int speed = 0; speed <= 255; speed++) {
        analogWrite(PIN_ENA, speed); // speed up
        delay(10);
      }
      cover = false;
      delay(2000);

      // up
      digitalWrite(PIN_IN3, LOW);   // control the motor's direction in anti-clockwise
      digitalWrite(PIN_IN4, HIGH);

      for (int speed = 0; speed <= 255; speed++) {
        analogWrite(PIN_ENB, speed); // speed up
        delay(10);
      }
      vertical = true;
      delay(2000);
    }

    digitalWrite(PIN_IN1, LOW); // stop motor A
    digitalWrite(PIN_IN2, LOW);

    digitalWrite(PIN_IN3, LOW); // stop motor B
    digitalWrite(PIN_IN4, LOW);

    }

  //Serial.print(rainAnalogVal);
  Serial.print("\t");
  //Serial.println(rainDigitalVal);
  delay(3000);
}