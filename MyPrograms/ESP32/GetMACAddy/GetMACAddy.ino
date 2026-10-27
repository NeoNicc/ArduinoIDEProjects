#include <WiFi.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC ADDY: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  Serial.println(WiFi.macAddress());
}
