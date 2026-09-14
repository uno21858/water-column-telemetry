#include <Arduino.h>

#include "caudalimetro.h"
#include "red.h"

static void imprimirCaudal(Print &salida, const uint32_t p[CAUDAL_N]) {
    for (size_t i = 0; i < CAUDAL_N; i++) {
        salida.printf("tubo%u=%u ", i + 1, p[i]);
    }
    salida.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    caudalimetroInit();
    redInit();
}

void loop() {
    static uint32_t t0 = 0;

    if (millis() - t0 >= 1000) {
        uint32_t p[CAUDAL_N];
        caudalimetroSnapshot(p);

        imprimirCaudal(Serial, p);

        t0 = millis();
    }
}