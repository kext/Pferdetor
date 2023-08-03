#pragma once

template<class I2c>
class Ssd1306 {
    static constexpr int ADDRESS = 0x3c;
    static constexpr int COMMAND = 0x00;
    static constexpr int DATA = 0x40;
    I2c &i2c;
  public:
    Ssd1306(I2c &i2c) : i2c(i2c) {}
    bool init() {
      static unsigned char const init_sequence[] = {
        0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40,
        0x8d, 0x14, 0x20, 0x00, 0xa1, 0xc8, 0xda, 0x12,
        0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6,
        0x21, 0x00, 0x7f, 0x22, 0x00, 0x07, 0x2e, 0xaf
      };
      i2c.init();
      for (int i = 0; i < 100; ++i) {
        if (i2c.write_command(ADDRESS, COMMAND, init_sequence, sizeof(init_sequence))) return true;
      }
      return false;
    }
    void display(const unsigned char *buffer) {
      i2c.write_command(ADDRESS, DATA, buffer, 1024);
    }
};