#include "BluetoothA2DPSource.h"
#include "mwahaha.h"

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LED_PIN = 19;

BluetoothA2DPSource a2dp_source;
const char* bt_speaker_name = "JBL Flip 5";

bool triggered = false;
unsigned long lastTriggerTime = 0;
const long cooldownTime = 30000;
int audio_index = 0;

// --- Timers for Background Bluetooth Tasks ---
unsigned long lastPingTime = 0;
unsigned long ledOnTime = 0;
bool ledActive = false;

int32_t get_sound_data(Frame *data, int32_t len) {
  if (triggered) {
    for (int i = 0; i < len; i++) {
      if (audio_index < sampleCount) {
        int8_t raw_sample = samples[audio_index];
        int16_t scaled_sample = raw_sample * 250;

        data[i].channel1 = scaled_sample;
        data[i].channel2 = scaled_sample;
        audio_index++;
      } else {
        data[i].channel1 = 0;
        data[i].channel2 = 0;
      }
    }
    if (audio_index >= sampleCount) {
      audio_index = 0;
      triggered = false;
    }
  } else {
    for (int i = 0; i < len; i++) {
      data[i].channel1 = 0;
      data[i].channel2 = 0;
    }
  }
  return len;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Initializing BT A2DP source...");
  a2dp_source.start(bt_speaker_name, get_sound_data);
  Serial.println("Ready! Scanning for Target Speaker...");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. ASYNCHRONOUS SENSOR CHECK (Pings every 150ms to allow Bluetooth background work)
  if (currentMillis - lastPingTime >= 150) {
    lastPingTime = currentMillis;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Timeout restricted to 15000us (~2.5 meters max range) to speed up performance
    long duration = pulseIn(ECHO_PIN, HIGH, 15000); 
    long distanceCm = duration * 0.034 / 2;

    if (distanceCm > 0 && distanceCm < 100 && !triggered && (currentMillis - lastTriggerTime > cooldownTime)) {
      triggered = true;
      audio_index = 0;
      lastTriggerTime = currentMillis;

      // Turn on LED and record when it turned on
      digitalWrite(LED_PIN, HIGH);
      ledOnTime = currentMillis;
      ledActive = true;
      Serial.println("Boo! Halloween Trick Triggered.");
    }
  }

  // 2. FIXED LED NON-BLOCKING TURN-OFF TIMER
  if (ledActive && (currentMillis - ledOnTime >= 300)) {
    digitalWrite(LED_PIN, LOW);
    ledActive = false; // Turn off tracker
  }
}