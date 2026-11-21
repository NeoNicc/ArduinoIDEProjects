#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

const int PWMA = 18;
const int AIN1 = 19;
const int AIN2 = 23;
const int PWMB = 32;
const int BIN1 = 33;
const int BIN2 = 27;
const int STBY = 26;

const int BUZZER = 22;

const int DEADZONE = 150;
const int XCENTER_VAL = 1900;
const int YCENTER_VAL = 1900;
int accelL;
int accelR;

int serialTimer = millis();
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
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(BUZZER, OUTPUT);

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
    int rawY = incomingData.yValue;
    int xDisplacement = rawX - XCENTER_VAL;
    int yDisplacement = rawY - YCENTER_VAL;

    if (abs(xDisplacement) < DEADZONE) {
      brake();
    } else if (xDisplacement > 0) {
      int speed = map(xDisplacement, DEADZONE, (4095 - XCENTER_VAL), 0, 255);
      speed = constrain(speed, 0, 255);
        if (yDisplacement > 149) {
          accelR = map(yDisplacement, DEADZONE, (4095 - YCENTER_VAL), 0, 125);
          accelR = constrain(accelR, 0, 125);
          accelL = accelR - (2*accelR);
        } else if (yDisplacement < -149) {
          accelL = map(abs(yDisplacement), DEADZONE, (4095 - YCENTER_VAL), 0, 125);
          accelL = constrain(accelL, 0, 125);
          accelR = accelL - (2*accelL);
        }
      forwardsR(speed + accelL);
      forwardsL(speed + accelR);
      printFWMessage(millis());
    } else {
      int speed = map(abs(xDisplacement), DEADZONE, (4095 - XCENTER_VAL), 0, 255);
      speed = constrain(speed, 0, 255);
      if (yDisplacement > 149) {
          accelR = map(yDisplacement, DEADZONE, (4095 - YCENTER_VAL), 0, 125);
          accelR = constrain(accelR, 0, 125);
          accelL = accelR - (2*accelR);
        } else if (yDisplacement < -149) {
          accelL = map(abs(yDisplacement), DEADZONE, (4095 - YCENTER_VAL), 0, 125);
          accelL = constrain(accelL, 0, 125);
          accelR = accelL - (2*accelL);
        }
      reverseR(speed + accelL);
      reverseL(speed + accelR);
      printRVMessage(millis());
    }

    //debugging line below
    //Serial.printf("Raw X: %d | Displacement: %d\nRaw y: %d | Y Displacement: %d\n", rawX, xDisplacement, rawY, yDisplacement);

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

void forwardsR(int speed) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
}

void forwardsL(int speed) {
  digitalWrite(BIN2, HIGH);
  digitalWrite(BIN1, LOW);
  analogWrite(PWMB, speed);
}

void brake() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN1, LOW);
}

void reverseR(int speed) {
  digitalWrite(AIN2, HIGH);
  digitalWrite(AIN1, LOW);
  analogWrite(PWMA, speed);
}

void reverseL(int speed) {
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
}

void printFWMessage(unsigned long current) {
  if(current - serialTimer > 300) {
    Serial.println("Forwards:");
    Serial.print("accelL: ");
    Serial.println(accelL);
    Serial.print("accelR: ");
    Serial.println(accelR);
    serialTimer = current;
  }
}

void printRVMessage(unsigned long current) {
  if(current - serialTimer > 300) {
    Serial.println("Reverse:");
    Serial.print("accelR: ");
    Serial.println(accelR);
    Serial.print("accelL: ");
    Serial.print(accelL);
    serialTimer = current;
  }
}