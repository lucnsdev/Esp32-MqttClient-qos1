#include <WiFi.h>
#include <LogicChanger.h>
#include <Delay.h>
#include "MqttClient.h"

String SSID = "brisa-4196852";
String PASSWORD = "deitczi8";
String BROKER = "broker.emqx.io";
String CLIENT_ID = "lucns-esp32";
String MQTT_TOPIC_ESP32 = "lucns/esp32";
String MQTT_TOPIC_ANDROID = "lucns/android";

MqttClient mqtt;
LogicChanger led;
Delay delaySend(INTERVAL_10S);

void onLogicChanged(bool enabled) {
  if (enabled) {
    if (mqtt.connecting()) neopixelWrite(RGB_BUILTIN, 16, 8, 0);
    else if (mqtt.connected()) neopixelWrite(RGB_BUILTIN, 0, 8, 0);
    else neopixelWrite(RGB_BUILTIN, 8, 8, 0);
  } else {
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  }
}

void onPublicationReceived(String topic, String message) {
  Serial.println("From: " + topic + ". Received: " + message);
}

void onPublicationAcknowledgement(uint16_t messageId) {
  Serial.print("Acknowledgement: ");
  Serial.println(messageId);
  neopixelWrite(RGB_BUILTIN, 0, 0, 8);
  delay(500);
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
}

void onConnectionChanged(bool connected) {
  Serial.println(connected ? "MQTT connected" : "MQTT disconnected!");
  delaySend.cancel();
  if (connected) {
    delay(100);
    Serial.println("Subscribing...");
    mqtt.subscribe(MQTT_TOPIC_ESP32);
  }
}

void onTopicSubscribeChanged(bool subscribed) {
  Serial.println(subscribed ? "Subscribed" : "Unsubscribed");
  if (subscribed) delaySend.reset();
}

void connectOnWifi() {
  neopixelWrite(RGB_BUILTIN, 8, 0, 0);
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID.c_str(), PASSWORD.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected on ");
  Serial.println(WiFi.localIP());
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
}

void setup() {
  neopixelWrite(RGB_BUILTIN, 8, 8, 8);
  Serial.begin(115200);
  while (!Serial) {}  // wait USB atach
  Serial.println("Send a character to initialize...");
  while (!Serial.available()) {}
  while (Serial.available()) {
    Serial.read();
    delayMicroseconds(1000);
  }
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);

  delaySend.cancel();
  led.setCallback(onLogicChanged);
  led.setTimers(100, 900);

  mqtt.setClientId(CLIENT_ID);
  mqtt.setOnPublicationReceived(onPublicationReceived);
  mqtt.setOnTopicSubscribeChanged(onTopicSubscribeChanged);
  mqtt.setOnConnectionChanged(onConnectionChanged);
  mqtt.setOnPublicationAcknowledgement(onPublicationAcknowledgement);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectOnWifi();
    WiFiClient client;
    mqtt.setClient(client);
  }
  if (!mqtt.connected() && !mqtt.connecting()) {
    Serial.println("Mqtt connecting...");
    mqtt.connect(BROKER, 1883);
  }
  mqtt.loop();
  if (delaySend.gate()) {
    delaySend.reset();
    Serial.print("Publishing...");
    uint8_t id = mqtt.publish(MQTT_TOPIC_ANDROID, "Lucas " + String(millis()));
    Serial.print("message id: ");
    Serial.println(id);
  }
  led.compute();
}
