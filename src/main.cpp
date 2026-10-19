#ifndef MODO_CALIBRACION

#include <Arduino.h>

#include "red/db.h"
#include "red/red.h"
#include "sensores/caudalimetro.h"
#include "sensores/presion.h"

static constexpr uint32_t INTERVALO_MS = 60000;

static void imprimirCaudal(Print &salida, const uint32_t p[CAUDAL_N]) {
    for (size_t i = 0; i < CAUDAL_N; i++) {
        salida.printf("tubo%u=%u ", i + 1, p[i]);
    }
    salida.println();
}

static void imprimirPresion(Print &salida, const int16_t p[PRESION_N]) {
    for (size_t i = 0; i < PRESION_N; i++) {
        if (p[i] == PRESION_SIN_DATO) {
            salida.printf("p%u=-- ", i + 1);
        } else {
            salida.printf("p%u=%d ", i + 1, p[i]);
        }
    }
    salida.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    caudalimetroInit();
    presionInit();
    redInit();
}

void loop() {
    static uint32_t t0 = 0;

    if (millis() - t0 >= INTERVALO_MS) {
        t0 = millis();

        uint32_t caudal[CAUDAL_N];
        caudalimetroSnapshot(caudal);

        int16_t presion[PRESION_N];
        if (presionLeer(presion)) {
            imprimirPresion(Serial, presion);
        }

        imprimirCaudal(Serial, caudal);
        dbEnviarCaudal(caudal);
    }
}

#endif