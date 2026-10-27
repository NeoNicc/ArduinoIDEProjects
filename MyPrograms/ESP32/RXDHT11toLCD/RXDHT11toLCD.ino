#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

typedef struct struct_message {
  float temp;
  int hum;
} struct_message;

struct_message myData;

void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData,sizeof(myData));

  if (myData.temp < 65) {
    lcd.setCursor(0, 0);
    lcd.print("IT'S FUCKIN COLD");
    lcd.setCursor(0, 1);
    lcd.print(myData.temp);
    lcd.print(" F  |  ");
    lcd.print(myData.hum);
    lcd.print("%");
  } else if (myData.temp > 80) {
    lcd.setCursor(0, 0);
    lcd.print("IT'S FUCKIN HOT");
    lcd.setCursor(0, 1);
    lcd.print(myData.temp);
    lcd.print(" F  |  ");
    lcd.print(myData.hum);
    lcd.print("%");
  } else if (myData.hum > 60) {
    lcd.setCursor(0, 0);
    lcd.print("FUCKIN HUMID");
    lcd.setCursor(0, 1);
    lcd.print(myData.temp);
    lcd.print(" F  |  ");
    lcd.print(myData.hum);
    lcd.print("%");
  } else if (myData.hum < 25) {
    lcd.setCursor(0, 0);
    lcd.print("DRY AS SHIT");
    lcd.setCursor(0, 1);
    lcd.print(myData.temp);
    lcd.print(" F  |  ");
    lcd.print(myData.hum);
    lcd.print("%");
  } else {
    //update lcd first row (temp)
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(myData.temp);
    lcd.print(" F");

    //update lcd second row (humidity)
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(myData.hum);
    lcd.print("%");
  }
}

void setup() {
  Serial.begin(115200);

  //initialize screen
  Wire.begin(26, 27);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Awaiting");
  lcd.setCursor(0, 1);
  lcd.print("Data");

  //set device as wifi station
  WiFi.mode(WIFI_STA);

  //initialize esp-now
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing esp-now");
    return;
  }

  //register callback function for receiving data
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {}