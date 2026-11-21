#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define IR_USE_AVR_TIMER2
#include <Servo.h>
#include <IRremote.h>

//Pinout
const int IR_SENSOR = 11;
const int DHT_PIN = 10;

//objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT11);
IRrecv irrecv(IR_SENSOR);
decode_results results;
Servo myServo;

int setTemp = 68;
float temp = 0.0;
int hum = 0;
int pos = 0;
int speed = 3;
unsigned long lastDHTRead = millis();
unsigned long lastScreenUpdate = millis();

void setup() {
  //serial connection for debugging
  Serial.begin(9600);

  //initialize temp sensor
  dht.begin();

  //initialize lcd screen
  Wire.begin();
  lcd.init();
  lcd.backlight();
  writeToScreen();

  //initialize IR rx
  irrecv.enableIRIn();
  myServo.write(pos);
  myServo.attach(9);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDHTRead > 5000) {
    lastDHTRead = millis();

    temp = dht.readTemperature(true);
    hum = dht.readHumidity();

    if (isnan(hum) || isnan(temp)) {
      return;
    }
  }
  writeToScreen();
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    newTemp();
    changeTemp();
    irrecv.resume();
  }
  changeTemp();
}

void writeToScreen() {
  unsigned long current = millis();
  if (current - lastScreenUpdate > 300) {
    lastScreenUpdate = current;
    lcd.setCursor(0, 0);
    lcd.print("Set: ");
    lcd.print(setTemp);
    lcd.print(" F");
    lcd.print("       ");
    lcd.setCursor(0, 1);
    lcd.print("Current: ");
    lcd.print(temp);
    lcd.print(" F");
  }
}

void newTemp() {
  switch(results.value) {
    case 0xFF629D: //up
      setTemp += 1;
      writeToScreen();
      break;
    case 0xFFA857:
      setTemp -= 1;
      writeToScreen();
      break;
    default: 
      break;
  }
}

void changeTemp() {
  if (temp > (float)setTemp + 1.0) {
    pos = 180;
    myServo.write(pos);
  } else if (temp < (float)setTemp - 1.0) {
    pos = 0;
    myServo.write(pos);
  } else {
    //do nothing
  }
}
