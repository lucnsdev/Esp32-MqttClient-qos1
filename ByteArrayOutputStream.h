#ifndef BYTE_ARRAY_OUTPUT_STREAM_H
#define BYTE_ARRAY_OUTPUT_STREAM_H

#define ORDER_LITTLE_ENDIAN 0
#define ORDER_BIG_ENDIAN 1

class ByteArrayOutputStream {
private:
  uint8_t* buffer;
  uint16_t capacity;
  uint16_t count;

  void ensureCapacity(uint16_t minCapacity) {
    if (minCapacity <= capacity) return;
    uint16_t newCapacity = capacity * 2;
    if (newCapacity < minCapacity) newCapacity = minCapacity;
    uint8_t* newBuffer = (uint8_t*)calloc(newCapacity, sizeof(uint8_t));
    if (count > 0) {
      for (uint16_t i = 0; i < count; i++) newBuffer[i] = buffer[i];
    }
    free(buffer);
    buffer = newBuffer;
    capacity = newCapacity;
  }

public:
  ByteArrayOutputStream(uint16_t initialCapacity = 32) {
    count = 0;
    capacity = initialCapacity;
    buffer = (uint8_t*)calloc(capacity, sizeof(uint8_t));
    //buffer = new uint8_t[capacity];
  }

  void write(uint8_t value) {
    ensureCapacity(count + 1);
    buffer[count++] = value;
  }

  void write(uint8_t* data, uint16_t len) {
    ensureCapacity(count + len);
    for (uint16_t i = 0; i < len; i++) {
      buffer[count++] = data[i];
    }
  }

  void writeShort(uint16_t v, uint8_t byteOrder = ORDER_LITTLE_ENDIAN) {
    if (byteOrder == ORDER_LITTLE_ENDIAN) {
      write((uint8_t)((v >> 8) & 0xFF));
      write((uint8_t)(v & 0xFF));
    } else {
      write((uint8_t)(v & 0xFF));
      write((uint8_t)((v >> 8) & 0xFF));
    }
  }

  void write(uint8_t* b, uint16_t off, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
      write(b[off + i]);
    }
  }

  uint16_t getSize() {
    return count;
  }

  void reset() {
    count = 0;
  }

  uint8_t* toByteArray() {
    return buffer;
  }

  void close() {
    free(buffer);
  }
};

#endif