#include "ssd1306.h"
#include "software-i2c.h"
#include "display.h"
#include "awakening.h"

static Display<128, 64> display;
static SoftwareI2c<20, 21> i2c;
static Ssd1306<decltype(i2c)> ssd{i2c};

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  ssd.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  display.clear();
  Awakening::text_centered(display, "Hier könnte Ihre", 0, 2, 128);
  Awakening::text_centered(display, "Werbung stehen!", 0, 14, 128);
  Awakening::text_centered(display, "Comment ça va?", 0, 26, 128);
  Awakening::text_centered(display, "120kΩ 1769", 0, 40, 128);
  //display.line(0, 0, 128, 64);
  ssd.display(display.data());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
