#ifndef MQTT_MESSAGE_HANDLER
#define MQTT_MESSAGE_HANDLER

#define CONNECT 1
#define CONNECT_ACKNOWLEDGEMENT 2
#define PUBLISH 3
#define PUBLISH_ACKNOWLEDGEMENT 4
#define PUBLISH_RECEIVED 5
#define PUBLISH_RELEASE 6
#define PUBLISH_COMPLETE 7
#define SUBSCRIBE 8
#define SUBSCRIBE_ACKNOWLEDGEMENT 9
#define UNSUBSCRIBE 10
#define UNSUBSCRIBE_ACKNOWLEDGEMENT 11
#define PING_REQUEST 12
#define PING_RESPONSE 13
#define DISCONNECT 14
#define QOS_0 0
#define QOS_1 1
// #define QOS_2 2 // unavailable
#define QOS_DEFAULT QOS_1

#define KEEP_ALIVE 60000
#define CLEAN_SESSION true
#define TIMEOUT 10000

#include <cstdlib> // for calloc()
#include "ByteArrayInputStream.h"
#include "ByteArrayOutputStream.h"

struct Message {
  uint8_t id;
  uint8_t type;
  uint8_t *header;
  uint16_t headerLen;
  uint8_t *payload;
  uint16_t payloadLen;
  uint8_t qos;
  uint8_t *topic;
  uint16_t topicLen;
  uint8_t *publication;
  uint16_t publicationLen;
};

class MqttMessageHandler {
  private:  
  uint16_t currentMessageId;

  void decodeUTF8(ByteArrayInputStream &inputStream, uint8_t *out, uint16_t len);
  void encodeUTF8(ByteArrayOutputStream &outputStream, uint8_t *in, uint16_t len);
  ByteArrayOutputStream encodeMBI(uint32_t number);
  uint32_t decodeMBI(ByteArrayInputStream &inputStream);
  uint8_t* charToUint8(const char* b, uint16_t len);

  public:
  MqttMessageHandler();
  Message readMessage(uint8_t *buffer, uint32_t bufferLen);
  Message getConnectMessage(const char *clientId, uint16_t clientIdLen);
  Message getPublishMessage(const char *topic, uint16_t topicLen, const char *message, uint16_t messageLen);
  Message getSubscribeMessage(const char *topic, uint16_t topicLen);
  Message getUnsubscribeMessage(const char *topic, uint16_t topicLen);
  Message getPublishMessageAck(uint16_t messageId);
  Message getPingMessage();

};

#endif