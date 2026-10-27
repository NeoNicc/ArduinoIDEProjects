#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

uint8_t broadcastAddress[] = {0x04, 0xB2, 0x47, 0x53, 0xB5, 0x3C};

typedef struct struct_message {
  float temp;
  int hum;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void onDataSent (const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  //set device as WIFI station
  WiFi.mode(WIFI_STA);

  //Initialize ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error Initializing ESP_NOW");
    return;
  }

  //register the send callback
  esp_now_register_send_cb(onDataSent);

  //register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  delay(5000);

  myData.hum = dht.readHumidity();
  myData.temp = dht.readTemperature(true);

  if(isnan(myData.hum) || isnan(myData.temp)) {
    Serial.println("Failed to read from sensor!");
    return;
  }

  //send message over esp-now
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Data Sent Successfully");
  } else {
    Serial.println("Error sending data");
  }
}
