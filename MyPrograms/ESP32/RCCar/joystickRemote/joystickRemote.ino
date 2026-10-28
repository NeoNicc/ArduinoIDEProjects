#include <WiFi.h>
#include <esp_now.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const int JOY_X_PIN = 34;
const int JOY_Y_PIN = 35;

typedef struct struct_message {
  int xValue;
  int yValue;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing esp-now");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  myData.xValue = analogRead(JOY_X_PIN);
  myData.yValue = analogRead(JOY_Y_PIN);

  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  delay(30);
}
