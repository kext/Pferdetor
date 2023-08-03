#pragma once

#include <Arduino.h>

template<int SDA, int SCL, int divider = 10, bool clock_stretching = false>
class SoftwareI2c {
    void high(int pin) {
      pinMode(pin, INPUT_PULLUP);
    }
    void low(int pin) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
    void wait() {
      __builtin_avr_delay_cycles(divider);
    }
    void wait_high(int pin) {
      wait();
      if constexpr (clock_stretching) {
        while (digitalRead(pin) != LOW) wait();
      }
    }
    void start() {
      low(SDA);
      wait();
      low(SCL);
    }
    void stop() {
      low(SDA);
      wait();
      high(SCL);
      wait_high(SCL);
      high(SDA);
      wait();
    }
    void ack() {
      low(SDA);
      wait();
      high(SCL);
      wait_high(SCL);
      low(SCL);
    }
    void nack() {
      high(SDA);
      wait();
      high(SCL);
      wait_high(SCL);
      low(SCL);
    }
    // Write a byte and return if ACK was seen.
    bool write_byte(int byte) {
      int i, a;
      for (i = 0; i < 8; ++i) {
        if (byte & 128) {
          high(SDA);
        } else {
          low(SDA);
        }
        byte <<= 1;
        wait();
        high(SCL);
        wait_high(SCL);
        low(SCL);
      }
      high(SDA);
      wait();
      high(SCL);
      wait_high(SCL);
      a = digitalRead(SDA) == LOW;
      low(SCL);
      return a;
    }
    // Read a byte and return it.
    // Needs ack or nack afterwards.
    int read_byte() {
      int i, byte = 0;
      for (i = 0; i < 8; ++i) {
        high(SDA);
        wait();
        high(SCL);
        wait_high(SCL);
        byte = (byte << 1) | digitalRead(SDA);
        low(SCL);
      }
      return byte;
    }
  public:
    void init() {
      high(SCL);
      high(SDA);
      __builtin_avr_delay_cycles(10 * divider);
    }
    bool write(unsigned address, unsigned char const *data, unsigned length) {
      start();
      if (!write_byte(address << 1)) {
        stop();
        return false;
      }
      for (unsigned i = 0; i < length; ++i) {
        if (!write_byte(data[i])) {
          stop();
          return false;
        }
      }
      stop();
      return true;
    }
    bool write_command(unsigned address, unsigned char command_byte, unsigned char const *data, unsigned length) {
      start();
      if (!write_byte(address << 1)) {
        stop();
        return false;
      }
      if (!write_byte(command_byte)) {
        stop();
        return false;
      }
      for (unsigned i = 0; i < length; ++i) {
        if (!write_byte(data[i])) {
          stop();
          return false;
        }
      }
      stop();
      return true;
    }
    bool read(unsigned address, unsigned char *data, unsigned length) {
      start();
      if (!write_byte((address << 1) | 1)) {
        stop();
        return false;
      }
      unsigned i = 0;
      while (true) {
        data[i++] = read_byte();
        if (i == length) {
          nack();
          stop();
          return true;
        } else {
          ack();
        }
      }
    }
};