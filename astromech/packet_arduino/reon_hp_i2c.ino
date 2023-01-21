#include "reon_hp_i2c.h"

void setup() {
    Wire.begin();
    Wire.onReceive(receiveEvent);
}

void loop() {
    delay(100);
}

void receiveEvent(int how_many) {
    while(1 < Wire.available()) {
        Serial.print(Wire.read());
    }
}