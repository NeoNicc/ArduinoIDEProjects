#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

const int PWMA = 4;
const int AIN1 = 5;
const int AIN2 = 18;
const int STBY = 23;
const int SRVO = 21;

Servo steeringServo;

const int DEADZONE = 150;
const int CENTER_VAL = 1870;
const int YCENTER_VAL = 1920;

typedef struct struct_message {
  int xValue;
  int yValue;
} struct_message;

struct_message incomingData;
volatile boolean newDataRec = false;

void OnDataRecv (const esp_now_recv_info *info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  newDataRec = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  steeringServo.setPeriodHertz(50);
  steeringServo.attach(SRVO, 500, 2400);
  steeringServo.write(90);

  digitalWrite(STBY, HIGH);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing esp now");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (newDataRec) {
    newDataRec = false;
    //dc motor
    int rawX = incomingData.xValue;
    int displacement = rawX - CENTER_VAL;

    if (abs(displacement) < DEADZONE) {
      brake();
    } else if (displacement > 0) {
      int speed = map(displacement, DEADZONE, (4095 - CENTER_VAL), 0, 255);
      speed = constrain(speed, 0, 255);
      forwards(speed);
    } else {
      int speed = map(abs(displacement), DEADZONE, CENTER_VAL, 0, 255);
      speed = constrain(speed, 0, 255);
      reverse(speed);
    }

    //Servo
    int rawY = incomingData.yValue;
    int yDisplacement = rawY - YCENTER_VAL;

    int servoAngle = map(rawY, 100, 3750, 40, 160);

    if (abs(yDisplacement) < DEADZONE) {
      steeringServo.write(90);
    } else {
      servoAngle = constrain(servoAngle, 40, 160);
      steeringServo.write(servoAngle);
    }

    Serial.printf("Raw X: %d | Displacement: %d\nRaw y: %d | Y Displacement: %d\n", rawX, displacement, rawY, yDisplacement);
  }
}

void forwards(int speed) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
}

void brake() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
}

void reverse(int speed) {
  digitalWrite(AIN2, HIGH);
  digitalWrite(AIN1, LOW);
  analogWrite(PWMA, speed);
}