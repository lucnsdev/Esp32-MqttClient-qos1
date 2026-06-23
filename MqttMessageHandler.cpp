#include "MqttMessageHandler.h"

MqttMessageHandler::MqttMessageHandler() {
}

Message MqttMessageHandler::readMessage(uint8_t *buffer, uint32_t bufferLen) {
  ByteArrayInputStream in(buffer, bufferLen);
  Message message;
  uint8_t first = in.read();
  message.type = first >> 4;
  uint8_t info = first &= 0x0f;
  message.qos = (info >> 1) & 0x03;
  //isRetained = (info & 0x01) == 0x01;
  //isDuplicated = (info & 0x08) == 0x08;

  uint32_t remaining = decodeMBI(in);
  switch (message.type) {
    case PUBLISH_ACKNOWLEDGEMENT:
      message.id = in.readShort(ORDER_BIG_ENDIAN);
      break;
    case PUBLISH:
      message.topicLen = in.readShort();
      message.topic = (uint8_t *)calloc(message.topicLen, sizeof(uint8_t));
      in.read(message.topic, message.topicLen);
      switch (message.qos) {
        case QOS_0:
          message.publicationLen = in.available();
          message.publication = (uint8_t *)calloc(message.publicationLen, sizeof(uint8_t));
          in.read(message.publication, message.publicationLen);
          break;
        case QOS_1:
          message.id = in.readShort();
          message.publicationLen = in.available();
          message.publication = (uint8_t *)calloc(message.publicationLen, sizeof(uint8_t));
          in.read(message.publication, message.publicationLen);
          break;
      }
      break;
  }
  in.close();
  return message;
}

Message MqttMessageHandler::getConnectMessage(const char *clientId, uint16_t clientIdLen) {
  uint8_t *cId = charToUint8(clientId, clientIdLen);
  ByteArrayOutputStream payload;
  payload.writeShort((uint16_t)clientIdLen);
  payload.write(cId, clientIdLen);
  free(cId);

  ByteArrayOutputStream variableHeader;         // variable header has 8 fixed bytes
  uint8_t mqtt[] = { 0x4D, 0x51, 0x54, 0x54 };  // M, Q, T, T in hex
  encodeUTF8(variableHeader, mqtt, 4);
  variableHeader.write((uint8_t)4);                        // mqtt version 4 = v3.1.1
  variableHeader.write((uint8_t)(CLEAN_SESSION ? 2 : 0));  // Connect flags
  variableHeader.writeShort((uint16_t)KEEP_ALIVE);         // Interval between ping messages

  uint32_t remainingLen = variableHeader.getSize() + payload.getSize();
  ByteArrayOutputStream mbi = encodeMBI(remainingLen);

  uint8_t messageInfo = 0;
  ByteArrayOutputStream header;
  header.write((uint8_t)(((CONNECT & 0x0f) << 4) ^ (messageInfo & 0x0f)));  // message type
  header.write(mbi.toByteArray(), mbi.getSize());                           // encoded string variable header length + payload length
  header.write(variableHeader.toByteArray(), variableHeader.getSize());

  currentMessageId = 0;
  Message message;
  message.id = currentMessageId++;
  message.type = CONNECT;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payload = payload.toByteArray();
  message.payloadLen = payload.getSize();
  return message;
}

Message MqttMessageHandler::getPublishMessage(const char *topic, uint16_t topicLen, const char *publication, uint16_t publicationLen) {
  uint8_t *t = charToUint8(topic, topicLen);
  uint8_t *payload = charToUint8(publication, publicationLen);

  ByteArrayOutputStream variableHeader;
  encodeUTF8(variableHeader, t, topicLen);
  if (QOS_DEFAULT > 0) variableHeader.writeShort(currentMessageId);

  uint32_t remainingLen = variableHeader.getSize() + publicationLen;
  ByteArrayOutputStream mbi = encodeMBI(remainingLen);

  uint8_t messageInfo = QOS_DEFAULT << 1;
  // if (isRetained) info |= 0x01;
  // if (isDuplicated) info |= 0x08;
  ByteArrayOutputStream header;
  header.write((uint8_t)(((PUBLISH & 0x0f) << 4) ^ (messageInfo & 0x0f)));
  header.write(mbi.toByteArray(), mbi.getSize());
  header.write(variableHeader.toByteArray(), variableHeader.getSize());

  Message message;
  message.id = currentMessageId++;
  message.type = PUBLISH;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payload = payload;
  message.payloadLen = publicationLen;
  return message;
}

Message MqttMessageHandler::getSubscribeMessage(const char *topic, uint16_t topicLen) {
  uint8_t *t = charToUint8(topic, topicLen);
  ByteArrayOutputStream payload;
  encodeUTF8(payload, t, topicLen);
  payload.write((uint8_t)QOS_DEFAULT);
  free(t);

  ByteArrayOutputStream variableHeader;
  variableHeader.writeShort((uint16_t)currentMessageId);

  uint32_t remainingLen = variableHeader.getSize() + payload.getSize();
  ByteArrayOutputStream mbi = encodeMBI(remainingLen);

  //uint8_t messageInfo = (2 | (isDuplicated ? 8 : 0));
  uint8_t messageInfo = 2;
  ByteArrayOutputStream header;
  header.write((uint8_t)(((SUBSCRIBE & 0x0f) << 4) ^ (messageInfo & 0x0f)));
  header.write(mbi.toByteArray(), mbi.getSize());
  header.write(variableHeader.toByteArray(), variableHeader.getSize());

  Message message;
  message.id = currentMessageId++;
  message.type = SUBSCRIBE;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payload = payload.toByteArray();
  message.payloadLen = payload.getSize();
  return message;
}

Message MqttMessageHandler::getUnsubscribeMessage(const char *topic, uint16_t topicLen) {
  uint8_t *t = charToUint8(topic, topicLen);
  ByteArrayOutputStream payload;
  encodeUTF8(payload, t, topicLen);
  free(t);

  ByteArrayOutputStream variableHeader;
  variableHeader.writeShort((uint16_t)currentMessageId);

  uint32_t remainingLen = variableHeader.getSize() + payload.getSize();
  ByteArrayOutputStream mbi = encodeMBI(remainingLen);

  //uint8_t messageInfo = (2 | (isDuplicated ? 8 : 0));
  uint8_t messageInfo = 2;
  ByteArrayOutputStream header;
  header.write((uint8_t)(((UNSUBSCRIBE & 0x0f) << 4) ^ (messageInfo & 0x0f)));
  header.write(mbi.toByteArray(), mbi.getSize());
  header.write(variableHeader.toByteArray(), variableHeader.getSize());

  Message message;
  message.id = currentMessageId++;
  message.type = UNSUBSCRIBE;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payload = payload.toByteArray();
  message.payloadLen = payload.getSize();
  return message;
}

Message MqttMessageHandler::getPublishMessageAck(uint16_t messageId) {
  ByteArrayOutputStream variableHeader;
  variableHeader.writeShort(messageId, ORDER_BIG_ENDIAN);

  ByteArrayOutputStream mbi = encodeMBI(2);
  ByteArrayOutputStream header;
  uint8_t messageInfo = 0;
  header.write((uint8_t)(((PUBLISH_ACKNOWLEDGEMENT & 0x0f) << 4) ^ (messageInfo & 0x0f)));
  header.write(mbi.toByteArray(), mbi.getSize());
  header.write(variableHeader.toByteArray(), variableHeader.getSize());

  Message message;
  message.type = PUBLISH_ACKNOWLEDGEMENT;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payloadLen = 0;
  return message;
}

Message MqttMessageHandler::getPingMessage() {
  ByteArrayOutputStream header;
  uint8_t messageInfo = 0;
  header.write((uint8_t)(((PING_REQUEST & 0x0f) << 4) ^ (messageInfo & 0x0f)));
  header.write((uint8_t)0);

  Message message;
  message.type = PING_REQUEST;
  message.header = header.toByteArray();
  message.headerLen = header.getSize();
  message.payloadLen = 0;
  return message;
}

void MqttMessageHandler::decodeUTF8(ByteArrayInputStream &inputStream, uint8_t *out, uint16_t len) {
  inputStream.read(out, len);
}

void MqttMessageHandler::encodeUTF8(ByteArrayOutputStream &outputStream, uint8_t *in, uint16_t len) {
  outputStream.write((uint8_t)((len >> 8) & 0xFF));
  outputStream.write((uint8_t)((len >> 0) & 0xFF));
  outputStream.write(in, 0, len);
}

ByteArrayOutputStream MqttMessageHandler::encodeMBI(uint32_t number) {
  uint8_t numBytes = 0;
  uint32_t no = number;
  ByteArrayOutputStream outputStream;
  do {
    uint8_t digit = (uint8_t)(no % 128);
    no = no / 128;
    if (no > 0) {
      digit |= (uint8_t)0x80;
    }
    outputStream.write(digit);
    numBytes++;
  } while ((no > 0) && (numBytes < 4));
  return outputStream;
}

uint32_t MqttMessageHandler::decodeMBI(ByteArrayInputStream &inputStream) {
  uint8_t digit;
  uint32_t msgLength = 0;
  uint32_t multiplier = 1;
  do {
    digit = inputStream.read();
    msgLength += ((digit & 0x7F) * multiplier);
    multiplier *= 128;
  } while ((digit & 0x80) != 0);
  return msgLength;
}

uint8_t *MqttMessageHandler::charToUint8(const char *b, uint16_t len) {
  uint8_t *buffer = (uint8_t *)calloc(len, sizeof(uint8_t));
  for (uint16_t i = 0; i < len; i++) buffer[i] = (uint8_t)b[i];
  return buffer;
}
