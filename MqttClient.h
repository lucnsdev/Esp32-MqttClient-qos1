#ifndef MQTT_H
#define MQTT_H

#include <WiFiClient.h>
#include "MqttMessageHandler.h"

class MqttClient {
private:
  WiFiClient client;
  MqttMessageHandler messageHandler;
  uint32_t lastPingTime;
  String clientId;
  bool isConnecting;

  void (*onPublicationReceived)(String, String);
  void (*onTopicSubscribeChanged)(bool);
  void (*onConnectionChanged)(bool);
  void (*onPublicationAcknowledgement)(uint16_t);

  void write(Message &message);
  String byteArrayToString(uint8_t *buf, uint16_t len);

public:
  MqttClient();
  void setClient(WiFiClient &client);

  void setOnTopicSubscribeChanged(void (*onTopicSubscribeChanged)(bool));
  void setOnPublicationReceived(void (*onPublicationReceived)(String, String));
  void setOnConnectionChanged(void (*onConnectionChanged)(bool));
  void setOnPublicationAcknowledgement(void (*onPublicationAcknowledgement)(uint16_t));
  void setClientId(String clientId);

  bool connecting();
  bool connected();
  void connect(String server, uint16_t port);
  uint16_t publish(String topic, String message);
  void subscribe(String topic);
  void unsubscribe(String topic);

  void loop();
};

#endif MQTT_H