#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

const int PWMA = 4;
const int AIN1 = 5;
const int AIN2 = 18;
const int STBY = 23;
const int SRVO = 21;

const int BUZZER = 22;

//driving variables
Servo steeringServo;

const int DEADZONE = 150;
const int CENTER_VAL = 1870;
const int YCENTER_VAL = 1920;

//honk variables
enum HonkState {IDLE, HONK, PAUSE, HONKHONK};
HonkState buzzerState = IDLE;
unsigned long buzzerTimer = 0;
bool lastClickState = HIGH;

//esp-now data structure
typedef struct struct_message {
  int xValue;
  int yValue;
  int click;
} struct_message;

struct_message incomingData;
volatile boolean newDataRec = false; //sentinel

void OnDataRecv (const esp_now_recv_info *info, const uint8_t *incomingDataPtr, int len) {
  //retrieve incoming data
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  newDataRec = true;
}

void setup() {
  // set up serial connection for debugging
  Serial.begin(115200);

  //specifiy pinout
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  //servo setup
  steeringServo.setPeriodHertz(50);
  steeringServo.attach(SRVO, 500, 2400);
  steeringServo.write(90);

  //driver requires STBY to be HIGH
  digitalWrite(STBY, HIGH);

  //esp-now related setup
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing esp now");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  //if our sentinel returns true take action
  //because we received data
  if (newDataRec) {
    newDataRec = false; //sentinel reset
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

    //debugging line below
    //Serial.printf("Raw X: %d | Displacement: %d\nRaw y: %d | Y Displacement: %d\n", rawX, displacement, rawY, yDisplacement);

    //trigger honk
    if (incomingData.click == LOW && lastClickState == HIGH && buzzerState == IDLE) {
      buzzerState = HONK;
      tone(BUZZER, 2000);
      buzzerTimer = millis();
    }
    lastClickState = incomingData.click;
  }

  //honking state machine
  unsigned long currentMillis = millis();

  switch(buzzerState) {
    case HONK:
      if (currentMillis - buzzerTimer >= 200) {
        noTone(BUZZER);
        buzzerState = PAUSE;
        buzzerTimer = currentMillis;
      }
      break;
    case PAUSE:
      if (currentMillis - buzzerTimer >= 150) {
        tone(BUZZER, 2000);
        buzzerState = HONKHONK;
        buzzerTimer = currentMillis;
      }
      break;
    case HONKHONK:
      if (currentMillis - buzzerTimer >= 700) {
        noTone(BUZZER);
        buzzerState = IDLE;
      }
      break;
    case IDLE:
    default:
      //waits for button press
      break;
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