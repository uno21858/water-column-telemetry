//
// Created by Erick on 9/28/26.
//


#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include <iterator>

#include "presion.h"
/// Muestras que se promedian por lectura.
static constexpr int MUESTRAS = 8;

/// Direcciones I2C de los chips (pin ADDR a GND y a VDD).
static constexpr uint8_t DIRECCIONES[] = {0x48, 0x49};
static constexpr size_t CHIPS_N = std::size(DIRECCIONES);

struct Canal {
    uint8_t chip;
    uint8_t entrada;
};

static constexpr Canal CANALES[] = {
    {0, 0},  // p1
    {0, 1},  // p2
    {0, 2},  // p3
    {0, 3},  // p4
    {1, 0},  // p5
    {1, 1},  // p6
    {1, 2},  // p7
    {1, 3},  // p8
};

static_assert(std::size(CANALES) == PRESION_N,
              "PRESION_N no coincide con el numero de canales");

static Adafruit_ADS1115 chips[CHIPS_N];
static bool chipListo[CHIPS_N] = {false};

#ifdef SIMULAR_PRESION
/// Valor inventado que varía lento en el tiempo, para probar sin hardware.
static int16_t simular(size_t i) {
    return static_cast<int16_t>(8000 + 1000 * i + (millis() / 1000) % 500);
}
#endif

bool presionInit() {
    Wire.begin(4, 5);  // WeMos D1 R1: SDA = GPIO4, SCL = GPIO5

    bool alguno = false;
    for (size_t i = 0; i < CHIPS_N; i++) {
        chipListo[i] = chips[i].begin(DIRECCIONES[i]);
        if (chipListo[i]) {
            chips[i].setGain(GAIN_TWO);  // ±2.048 V; el default de la librería es ±6.144 V
            alguno = true;
        } else {
            Serial.printf("ADS1115 en 0x%02X no responde\n", DIRECCIONES[i]);
        }
    }
    return alguno;
}
bool presionLeer(int16_t destino[PRESION_N]) {
    bool alguno = false;

    for (size_t i = 0; i < PRESION_N; i++) {
#ifdef SIMULAR_PRESION
        destino[i] = simular(i);
        alguno = true;
#else
        const Canal &c = CANALES[i];
        if (!chipListo[c.chip]) {
            destino[i] = PRESION_SIN_DATO;
            continue;
        }
        int32_t suma = 0;
        for (int k = 0; k < MUESTRAS; k++) {
            suma += chips[c.chip].readADC_SingleEnded(c.entrada);
        }
        destino[i] = static_cast<int16_t>(suma / MUESTRAS);
        alguno = true;
#endif
    }

    return alguno;
}
