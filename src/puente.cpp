// Modo puente: la WeMos no lee sensores. Recibe por serial la línea que arma
// el Mega cada minuto, la valida y la sube a Supabase como un solo lote.
//
// Cableado: TX2 del Mega -> divisor 2.2k/3.3k -> GPIO4. GPIO5 -> RX2. GND común.
//
// pio run -e puente && pio device monitor

#ifdef MODO_PUENTE

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <iterator>

#include "puente/protocolo.h"
#include "red/db.h"
#include "red/red.h"

static constexpr int8_t PIN_RX = 4;
static constexpr int8_t PIN_TX = 5;
static constexpr uint32_t BAUD_MEGA = 9600;  // igual que BAUD_WEMOS en el Mega

// sensor_id de cada posición, en el orden en que el Mega recorre sus pines.
// Tienen que existir en sbagua.sensores: si uno no existe, Supabase rechaza
// el lote completo.
static constexpr uint16_t SENSORES_PH[] = {101, 102, 103};   // A0, A1, A2
static constexpr uint16_t SENSORES_TDS[] = {104, 105, 106};  // A4, A5, A6
static constexpr uint16_t SENSORES_CAUDAL[] = {107, 108};    // pines 2 y 3

static constexpr Grupo GRUPOS[] = {
    {'H', SENSORES_PH, std::size(SENSORES_PH)},
    {'T', SENSORES_TDS, std::size(SENSORES_TDS)},
    {'C', SENSORES_CAUDAL, std::size(SENSORES_CAUDAL)},
};

static constexpr size_t LECTURAS_MAX =
    std::size(SENSORES_PH) + std::size(SENSORES_TDS) + std::size(SENSORES_CAUDAL);

static SoftwareSerial mega;

static char linea[160];
static size_t largo = 0;
static bool dentro = false;  // true entre el '$' y el fin de línea

static void procesar() {
    Lectura lote[LECTURAS_MAX];
    size_t n = 0;

    ResultadoLinea r = lineaParsear(linea, GRUPOS, std::size(GRUPOS),
                                    lote, std::size(lote), n);
    if (r != ResultadoLinea::ok) {
        Serial.printf("descartada (%s): %s\n", resultadoTexto(r), linea);
        return;
    }

    Serial.printf("ok, %u lecturas: %s\n", static_cast<unsigned>(n), linea);
    if (n > 0 && !dbEnviar(lote, n)) {
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
    if (mega.overflow()) {
        Serial.println("buffer serial lleno, se perdieron bytes");
    }

    while (mega.available() > 0) {
        char c = static_cast<char>(mega.read());

        // Un '$' siempre empieza línea nueva: resincroniza después de basura
        if (c == '$') {
            largo = 0;
            dentro = true;
        }
        if (!dentro || c == '\r') {
            continue;
        }

        if (c == '\n') {
            linea[largo] = '\0';
            dentro = false;
            procesar();
        } else if (largo < sizeof(linea) - 1) {
            linea[largo++] = c;
        } else {
            dentro = false;
            Serial.println("linea demasiado larga, descartada");
        }
    }
}

#endif
