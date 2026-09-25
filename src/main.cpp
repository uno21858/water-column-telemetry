#include <Arduino.h>

#include "sensores/caudalimetro.h"
#include "red/red.h"

#include <Arduino.h>

#include "sensores/caudalimetro.h"
#include "red/db.h"
#include "red/red.h"

static constexpr uint32_t INTERVALO_MS = 60000;

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

    if (millis() - t0 >= INTERVALO_MS) {
        t0 = millis();

        uint32_t p[CAUDAL_N];
        caudalimetroSnapshot(p);

        imprimirCaudal(Serial, p);
        dbEnviarCaudal(p);
    }
}