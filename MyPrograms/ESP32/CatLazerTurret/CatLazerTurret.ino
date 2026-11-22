#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_system.h>

const int ROTATE = 18;
const int PITCH = 19;

Servo rotServo;
Servo pitServo;

bool lastClickState = HIGH;

int targetX = 150;
int targetY = 90;
int lastX = 150;
int lastY = 90;

unsigned long stepCount = 0;
int speed = 30;
int step = 3;

enum Mode {AUTO, MANUAL};
Mode mode = AUTO;
unsigned long angleSwitchTimer = 0;

typedef struct struct_message {
  int xValue;
  int yValue;
  int click;
} struct_message;

struct_message incomingData;
volatile boolean newDataRec = false;
portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

void OnDataRecv (const esp_now_recv_info *info, const uint8_t *incomingDataPtr, int len) {
  if (incomingDataPtr == nullptr || len != sizeof(struct_message)) return;

  portENTER_CRITICAL(&dataMux);
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  newDataRec = true;
  portEXIT_CRITICAL(&dataMux);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  randomSeed(esp_random());

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  rotServo.attach(ROTATE);
  pitServo.attach(PITCH);

  delay(300);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing esp now");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  static struct_message latestInput = {90, 90, 0};
  bool newData = false;
  portENTER_CRITICAL(&dataMux);
  if (newDataRec) {
    latestInput = incomingData;
    newDataRec = false;
    newData = true;
  }
  portEXIT_CRITICAL(&dataMux);
  if (newData) {
    newData = false;

    if (latestInput.click == LOW && lastClickState == HIGH) {
      //happens when joystick is clicked
      if (mode == MANUAL) {
        mode = AUTO;
      } else if (mode == AUTO) {
        mode = MANUAL;
      }
    } 

    lastClickState = latestInput.click;

    if (mode == MANUAL) {
      int rawX = latestInput.xValue;
      int rawY = latestInput.yValue;
    

      int pitServoAngle = map(rawX, 4095, 0, 30, 160);
      pitServoAngle = constrain(pitServoAngle, 30, 160);
      targetX = pitServoAngle;

      int rotServoAngle = map(rawY, 4095, 0, 0, 180);
      rotServoAngle = constrain(rotServoAngle, 0, 180);
      targetY = rotServoAngle;
    } 
  }

  if (mode == AUTO) {
    unsigned long currentMillis = millis();

    if (currentMillis - angleSwitchTimer > random(800, 1501)) {
      angleSwitchTimer = currentMillis;

      targetX = random(140, 160);
      targetY = random(50, 130);
    }
  }

  smoothMove(targetX, targetY);
}

void smoothMove(int x, int y) {
  int currentX = lastX;
  int currentY = lastY;
  unsigned long currentMillis = millis();

  if (currentMillis - stepCount > speed) {
    stepCount = currentMillis;
    if (x > currentX) {
      if (x - currentX > 50) {
        currentX += step*3;
      } else if (x - currentX > 20) {
        currentX += step*2;
      } else if (x - currentX > 8) {
        currentX += step;
      } else {
        currentX += step/2;
      }
      if (currentX > x) {
        currentX = x;
      }
    } else if (x < currentX) {
      if (currentX - x > 50) {
        currentX -= step*3;
      } else if (currentX - x > 20) {
        currentX -= step*2;
      } else if (currentX - x > 8) {
        currentX -= step;
      } else {
        currentX -= step/2;
      }
      if (currentX < x) {
        currentX = x;
      }
    }
    if (y > currentY) {
      if (y - currentY > 50) {
        currentY += step*3;
      } else if (y - currentY > 20) {
        currentY += step*2;
      } else if (y - currentY > 8) {
        currentY += step;
      } else {
        currentY += step/2;
      }

      if (currentY > y) {
        currentY = y;
      }
    } else if (y < currentY) {
      if (currentY - y > 50) {
        currentY -= step*3;
      } else if (currentY - y > 20) {
        currentY -= step*2;
      } else if (currentY - y > 8) {
        currentY -= step;
      } else {
        currentY -= step/2;
      }

      if (currentY < y) {
        currentY = y;
      }
    }

    lastX = currentX;
    lastY = currentY;

    rotServo.write(currentY);
    pitServo.write(currentX);
  }
}