#include <WiFi.h>
#include <esp_now.h>

const int PWMA = 18;
const int AIN1 = 19;
const int AIN2 = 23;
const int PWMB = 32;
const int BIN1 = 33;
const int BIN2 = 27;
const int STBY = 26;

const int BUZZER = 22;

// LEDC PWM configuration (20 kHz silent frequency, 10-bit resolution)
const int PWM_FREQ = 20000;
const int PWM_RES = 10;
const int MAX_PWM = 800;
const int MIN_PWM = 180;      // Minimum PWM to overcome motor/tread static friction (tune 150-250)
const int MAX_STEER = 350;    // Gentler steering at speed so tank doesn't bog down

// Calibrated joystick limits (adjust after checking Serial Monitor!)
const int DEADZONE = 150;
const int XCENTER_VAL = 1875;
const int YCENTER_VAL = 1920;
const int XMIN_VAL = -1875;     // Typical ESP32 ADC bottom reading
const int XMAX_VAL = 2220;    // Typical ESP32 ADC top reading
const int YMIN_VAL = -1920;
const int YMAX_VAL = 2175;

int accelL;
int accelR;

int currentSpeedR = 0;
int currentSpeedL = 0;
unsigned long lastUpdatedR = 0;
unsigned long lastUpdatedL = 0;
const int ACCEL_RATE = 140;

unsigned long serialTimer = 0;

//honk variables
enum HonkState {IDLE, HONK, PAUSE, HONKHONK};
HonkState buzzerState = IDLE;
unsigned long buzzerTimer = 0;
bool lastClickState = HIGH;

// ESP-NOW data structure
typedef struct struct_message {
  int xValue;
  int yValue;
  int click;
} struct_message;

struct_message incomingData;
volatile boolean newDataRec = false;

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  newDataRec = true;
}

// Helper: Maps displacement with an quadratic curve for smooth low-speed control
int calculateCurvedSpeed(int displacement, int deadzone, int maxDisp, int minOut, int maxOut) {
  float norm = (float)(displacement - deadzone) / (float)(maxDisp - deadzone);
  norm = constrain(norm, 0.0f, 1.0f);
  float curved = norm * norm; // Quadratic curve: fine control near center, full power at edges
  return minOut + (int)(curved * (maxOut - minOut));
}

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  ledcAttach(PWMA, PWM_FREQ, PWM_RES);
  ledcAttach(PWMB, PWM_FREQ, PWM_RES);

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
    int rawX = incomingData.xValue;
    int rawY = incomingData.yValue;
    int xDisplacement = rawX - XCENTER_VAL;
    int yDisplacement = rawY - YCENTER_VAL;

    accelL = 0;
    accelR = 0;

    // Symmetric steering offsets using calibrated Y limits
    if (yDisplacement > DEADZONE) {
      accelR = map(yDisplacement, DEADZONE, YMAX_VAL, 0, MAX_STEER);
      accelR = constrain(accelR, 0, MAX_STEER);
      accelL = -accelR;
    } else if (yDisplacement < -DEADZONE) {
      accelL = map(abs(yDisplacement), DEADZONE, -YMIN_VAL, 0, MAX_STEER);
      accelL = constrain(accelL, 0, MAX_STEER);
      accelR = -accelL;
    }

    // Throttle & steering mix
    if (abs(xDisplacement) < DEADZONE) {
      if (abs(yDisplacement) > DEADZONE) {
        leftTread(accelR*2);
        rightTread(accelL*2);
      } else {
        brake();
        currentSpeedL = 0;
        currentSpeedR = 0;
        
        printDebugMessage("Stopped", rawX, rawY, 0, 0);
      }
    } else {
      int speed = calculateCurvedSpeed(abs(xDisplacement), DEADZONE, XMAX_VAL, MIN_PWM, MAX_PWM);
      if (xDisplacement < 0) {
        speed = -speed;
      }

      int finalR;
      int finalL;
      if (speed < 0) {
        finalR = constrain(speed - accelL, -MAX_PWM, MAX_PWM);
        finalL = constrain(speed - accelR, -MAX_PWM, MAX_PWM);
      } else {
        finalR = constrain(speed + accelL, -MAX_PWM, MAX_PWM);
        finalL = constrain(speed + accelR, -MAX_PWM, MAX_PWM);
      }
      

      rightTread(finalR);
      leftTread(finalL);
      printDebugMessage("Moving", rawX, rawY, finalL, finalR);
    }
  }

  //trigger honk
  if (incomingData.click == LOW && lastClickState == HIGH && buzzerState == IDLE && millis() > 300) {
    buzzerState = HONK;
    tone(BUZZER, 2000);
    buzzerTimer = millis();
  }
  lastClickState = incomingData.click;

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

void rightTread(int speed) {
  int targetSpeed = constrain(speed, -MAX_PWM, MAX_PWM);

  const unsigned long STEP_TIME = 20;
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdatedR >= STEP_TIME) {
    lastUpdatedR = currentMillis;

    if (currentSpeedR < targetSpeed) {
      currentSpeedR += ACCEL_RATE;
      if (currentSpeedR > targetSpeed) {
        currentSpeedR = targetSpeed;
      }
    } else if (currentSpeedR > targetSpeed) {
      currentSpeedR -= ACCEL_RATE;
      if (currentSpeedR < targetSpeed) {
        currentSpeedR = targetSpeed;
      }
    }
  }

  if (currentSpeedR > 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    ledcWrite(PWMA, currentSpeedR);
  } else if (currentSpeedR < 0) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    ledcWrite(PWMA, abs(currentSpeedR));
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    ledcWrite(PWMA, 0);
  }
}

void leftTread(int speed) {
  int targetSpeed = constrain(speed, -MAX_PWM, MAX_PWM);

  const unsigned long STEP_TIME = 20;
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdatedL >= STEP_TIME) {
    lastUpdatedL = currentMillis;

    if (currentSpeedL < targetSpeed) {
      currentSpeedL += ACCEL_RATE;
      if (currentSpeedL > targetSpeed) {
        currentSpeedL = targetSpeed;
      }
    } else if (currentSpeedL > targetSpeed) {
      currentSpeedL -= ACCEL_RATE;
      if (currentSpeedL < targetSpeed) {
        currentSpeedL = targetSpeed;
      }
    }
  }

  if (currentSpeedL > 0) {
    digitalWrite(BIN2, HIGH);
    digitalWrite(BIN1, LOW);
    ledcWrite(PWMB, currentSpeedL);
  } else if (currentSpeedL < 0) {
    digitalWrite(BIN2, LOW);
    digitalWrite(BIN1, HIGH);
    ledcWrite(PWMB, abs(currentSpeedL));
  } else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
    ledcWrite(PWMB, 0);
  }
}

void brake() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  ledcWrite(PWMA, 0);
  ledcWrite(PWMB, 0);
  currentSpeedL = 0;
  currentSpeedR = 0;
}

void printDebugMessage(const char* dir, int rawX, int rawY, int pwmL, int pwmR) {
  unsigned long current = millis();
  if (current - serialTimer > 300) {
    Serial.printf("%s | rawX: %d, rawY: %d | PWM L: %d, R: %d\n", dir, rawX, rawY, pwmL, pwmR);
    serialTimer = current;
  }
}