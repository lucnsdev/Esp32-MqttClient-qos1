#include "MqttClient.h"

MqttClient::MqttClient() {
  lastPingTime = 0;
  isConnecting = false;
}

void MqttClient::setClient(WiFiClient &client) {
  this->client = client;
}

void MqttClient::setClientId(String clientId) {
  this->clientId = clientId;
}

void MqttClient::setOnTopicSubscribeChanged(void (*callback)(bool)) {
  onTopicSubscribeChanged = callback;
}

void MqttClient::setOnPublicationReceived(void (*callback)(String, String)) {
  onPublicationReceived = callback;
}

void MqttClient::setOnConnectionChanged(void (*callback)(bool)) {
  onConnectionChanged = callback;
}

void MqttClient::setOnPublicationAcknowledgement(void (*callback)(uint16_t)) {
  onPublicationAcknowledgement = callback;
}

bool MqttClient::connecting() {
  return isConnecting;
}

bool MqttClient::connected() {
  uint8_t a = client.connected();
  if (a) return true;
  client.flush();
  client.stop();
  return false;
}

void MqttClient::connect(String server, uint16_t port) {
  if (isConnecting) return;
  isConnecting = true;

  int response = client.connect(server.c_str(), port, TIMEOUT);
  if (!response) {
    isConnecting = false;
    return;
  }
  lastPingTime = millis();
  Message message = messageHandler.getConnectMessage(clientId.c_str(), clientId.length());
  write(message);
}

uint16_t MqttClient::publish(String topic, String publication) {
  if (!connected()) return 0;
  lastPingTime = millis();
  Message message = messageHandler.getPublishMessage(topic.c_str(), topic.length(), publication.c_str(), publication.length());
  write(message);
  return message.id;
}
void MqttClient::subscribe(String topic) {
  if (!connected()) return;
  lastPingTime = millis();
  Message message = messageHandler.getSubscribeMessage(topic.c_str(), topic.length());
  write(message);
}

void MqttClient::unsubscribe(String topic) {
  if (!connected()) return;
  lastPingTime = millis();
  Message message = messageHandler.getUnsubscribeMessage(topic.c_str(), topic.length());
  write(message);
}

void MqttClient::loop() {
  if (!connecting()) {
	  if (!connected()) {
		if (onConnectionChanged) onConnectionChanged(false);
		return;
	  }
  }

  if (millis() - lastPingTime >= KEEP_ALIVE) {
    lastPingTime = millis();
    Message message = messageHandler.getPingMessage();
    write(message);
  }
  if (client.available() <= 0) return;

  uint32_t available = client.available();
  uint8_t *buffer = (uint8_t *)calloc(available, sizeof(uint8_t));
  int32_t read = client.read(buffer, available);
  if (read < 0) {
    free(buffer);
    return;
  }

  Message message = messageHandler.readMessage(buffer, read);
  switch (message.type) {
	case PING_RESPONSE:
	  //Serial.println("Ping response");
	  break;
    case CONNECT_ACKNOWLEDGEMENT:
      isConnecting = false;
      if (onConnectionChanged) onConnectionChanged(true);
      break;
    case DISCONNECT:
      isConnecting = false;
      if (onConnectionChanged) onConnectionChanged(false);
      break;
    case PUBLISH_ACKNOWLEDGEMENT:
      if (onPublicationAcknowledgement) onPublicationAcknowledgement(message.id);
      break;
    case SUBSCRIBE_ACKNOWLEDGEMENT:
      if (onTopicSubscribeChanged) onTopicSubscribeChanged(true);
      break;
    case UNSUBSCRIBE_ACKNOWLEDGEMENT:
      if (onTopicSubscribeChanged) onTopicSubscribeChanged(false);
      break;
    case PUBLISH:
      String t = byteArrayToString(message.topic, message.topicLen);
      String p = byteArrayToString(message.publication, message.publicationLen);
      if (message.qos > 0) {
        message = messageHandler.getPublishMessageAck(message.id);
        write(message);
      }
      if (onPublicationReceived) onPublicationReceived(t, p);
      break;
  }
}

void MqttClient::write(Message &message) {	
  client.write(message.header, message.headerLen);
  if (message.payloadLen > 0) client.write(message.payload, message.payloadLen);
  //if (!connected() && onConnectionChanged) onConnectionChanged(false);
}

String MqttClient::byteArrayToString(uint8_t *buf, uint16_t len) {
  String s;
  for (uint16_t i = 0; i < len; i++) {
    s += (char)buf[i];
  }
  return s;
}
