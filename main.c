#include "uart.h"

void setup() {
    Serial.begin(115200);          // for printing the result to your PC
    delay(1000);

    uart2_config_t cfg = { 115200, 8, 1, 0 };
    uart2_init(&cfg);

    uart2_send_byte('A');

    uint8_t rx = 0;
    int tries = 100000;
    while (!uart2_receive_byte(&rx) && --tries) { }

    if (tries && rx == 'A') Serial.println("PASS");
    else                    Serial.println("FAIL");
}

void loop() { }