#ifndef BYTE_ARRAY_INPUT_STREAM_H
#define BYTE_ARRAY_INPUT_STREAM_H

#define ORDER_LITTLE_ENDIAN 0
#define ORDER_BIG_ENDIAN 1

class ByteArrayInputStream {
private:
  uint16_t counter;
  uint16_t len;
  uint8_t *buffer;

public:
  ByteArrayInputStream(uint8_t *in, uint16_t len) {
    this->buffer = in;
    this->counter = 0;
    this->len = len;
  }

  uint16_t available() {
    return len - counter;
  }

  uint8_t read() {
    return buffer[counter++];
  }

  void read(uint8_t *b, uint16_t dLen) {
    for (uint16_t i = 0; i < dLen; i++) {
      b[i] = read();
    }
  }

  uint16_t readShort(int8_t byteOrder = ORDER_LITTLE_ENDIAN) {
    uint8_t a = read();
    uint8_t b = read();
    return byteOrder == ORDER_LITTLE_ENDIAN ? ((a & 0xFF) << 8) | (b & 0xFF) : ((b & 0xFF) << 8) | (a & 0xFF);
  }

  void reset() {
    counter = 0;
  }

  void close() {
    free(buffer);
  }
};

#endif
