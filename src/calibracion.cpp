//
// Created by Erick on 9/28/26.
//

// Correr modo calibracion
// pio run -e calibracion
// pio device monitor

#ifdef MODO_CALIBRACION

#include <Arduino.h>

#include "sensores/caudalimetro.h"

// Modo calibración: sin WiFi ni base de datos.
// Acumula los pulsos de cada tubo. Manda 'r' por el monitor para reiniciar.

static uint32_t totales[CAUDAL_N] = {0};

void setup() {
    Serial.begin(115200);
    delay(1000);
    caudalimetroInit();
    Serial.println("\nMODO CALIBRACION. Manda 'r' para poner los totales en cero.");
}

void loop() {
    static uint32_t t0 = 0;

    if (Serial.available() > 0 && Serial.read() == 'r') {
        uint32_t descartar[CAUDAL_N];
        caudalimetroSnapshot(descartar);
        for (size_t i = 0; i < CAUDAL_N; i++) {
            totales[i] = 0;
        }
        Serial.println("Totales en cero.");
    }

    if (millis() - t0 >= 1000) {
        t0 = millis();

        uint32_t p[CAUDAL_N];
        caudalimetroSnapshot(p);

        for (size_t i = 0; i < CAUDAL_N; i++) {
            totales[i] += p[i];
            Serial.printf("tubo%u total=%u (%u/s)  ", i + 1, totales[i], p[i]);
        }
        Serial.println();
    }
}

#endif