// Modo puente: la WeMos no lee sensores. Recibe por serial la línea DATA que
// el Mega manda cada ~2 s, la imprime al monitor y cada 30 s sube a Supabase
// el promedio de las líneas válidas de ese intervalo.
//
// Cableado: TX2 del Mega -> divisor 2.2k/3.3k -> GPIO4. GPIO5 -> RX2. GND común.
//
// pio run -e puente -t upload && pio device monitor

#ifdef MODO_PUENTE

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "puente/protocolo.h"
#include "red/db.h"
#include "red/red.h"

static constexpr int8_t PIN_RX = 4;
static constexpr int8_t PIN_TX = 5;
static constexpr uint32_t BAUD_MEGA = 9600;      // igual que Serial2.begin() en el Mega
static constexpr uint32_t INTERVALO_MS = 30000;  // cada cuánto se sube a la DB

// sensor_id de cada valor de la línea, en el mismo orden:
// pH1..3, EC1..3, TDS1..3, F1..4. Tienen que existir en sbagua.sensores
// (db/esquema.sql): si uno no existe, Supabase rechaza el lote completo.
static constexpr uint16_t SENSORES[TRAMA_N] = {
    5, 6, 7,        // pH  A0, A1, A2
    8, 9, 10,       // EC  A4, A5, A6
    11, 12, 13,     // TDS A4, A5, A6
    1, 2, 3, 4,     // flujo D2, D3, D19, D18
};

static SoftwareSerial mega;

static char linea[200];
static size_t largo = 0;

static float suma[TRAMA_N] = {};
static uint16_t muestras = 0;

static void procesar() {
    float v[TRAMA_N];
    ResultadoLinea r = lineaParsear(linea, v);
    if (r != ResultadoLinea::ok) {
        Serial.printf("descartada (%s): %s\n", resultadoTexto(r), linea);
        return;
    }

    Serial.println(linea);
    for (size_t i = 0; i < TRAMA_N; i++) {
        suma[i] += v[i];
    }
    muestras++;
}

static void subir() {
    if (muestras == 0) {
        Serial.println("sin lineas validas del Mega en este intervalo");
        return;
    }

    Lectura lote[TRAMA_N];
    for (size_t i = 0; i < TRAMA_N; i++) {
        lote[i] = {SENSORES[i], suma[i] / muestras};
        suma[i] = 0;
    }
    Serial.printf("subiendo promedio de %u lineas\n", muestras);
    muestras = 0;

    if (!dbEnviar(lote, TRAMA_N)) {
        Serial.println("lote no subido");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Buffer de 256: cabe una línea completa aunque el loop esté ocupado en un POST
    mega.begin(BAUD_MEGA, SWSERIAL_8N1, PIN_RX, PIN_TX, false, 256);
    redInit();

    Serial.println("Puente listo, esperando al Mega");
}

void loop() {
    static uint32_t tDb = millis();

    if (mega.overflow()) {
        Serial.println("buffer serial lleno, se perdieron bytes");
    }

    while (mega.available() > 0) {
        char c = static_cast<char>(mega.read());
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            linea[largo] = '\0';
            if (largo > 0) {
                procesar();
            }
            largo = 0;
        } else if (largo < sizeof(linea) - 1) {
            linea[largo++] = c;
        } else {
            // Línea demasiado larga: se tira lo acumulado y se resincroniza
            // en el siguiente '\n' (la línea a medias falla en el parser)
            largo = 0;
        }
    }

    if (millis() - tDb >= INTERVALO_MS) {
        tDb = millis();
        subir();
    }
}

#endif
