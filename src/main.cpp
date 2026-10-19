#if !defined(MODO_CALIBRACION) && !defined(MODO_PUENTE)

#include <Arduino.h>

#include "red/db.h"
#include "red/red.h"
#include "sensores/caudalimetro.h"
#include "sensores/presion.h"

static constexpr uint32_t INTERVALO_MS = 30000;  // cada cuánto se sube a la DB
static constexpr uint32_t MONITOR_MS = 1000;     // cada cuánto se imprime en el monitor

// p son pulsos por minuto
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
    static uint32_t tMon = millis();
    static uint32_t tDb = millis();
    static uint32_t loteDb[CAUDAL_N] = {};  // pulsos acumulados para la DB

    uint32_t ahora = millis();
    if (ahora - tMon < MONITOR_MS) {
        return;
    }

    // Los pulsos de cada impresión se suman al lote de la DB, así no se pierde
    // ninguno aunque el snapshot reinicie los contadores.
    uint32_t caudal[CAUDAL_N];
    caudalimetroSnapshot(caudal);
    uint32_t transcurrido = ahora - tMon;
    tMon = ahora;

    for (size_t i = 0; i < CAUDAL_N; i++) {
        loteDb[i] += caudal[i];
        caudal[i] = static_cast<uint64_t>(caudal[i]) * 60000 / transcurrido;
    }

    int16_t presion[PRESION_N];
    if (presionLeer(presion)) {
        imprimirPresion(Serial, presion);
    }
    imprimirCaudal(Serial, caudal);

    if (ahora - tDb >= INTERVALO_MS) {
        // La vista sbagua.caudal espera pulsos por minuto
        uint32_t ms = ahora - tDb;
        for (size_t i = 0; i < CAUDAL_N; i++) {
            loteDb[i] = static_cast<uint64_t>(loteDb[i]) * 60000 / ms;
        }
        tDb = ahora;
        dbEnviarCaudal(loteDb);
        for (auto &p : loteDb) p = 0;
    }
}

#endif