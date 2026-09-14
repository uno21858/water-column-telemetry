//
// Created by Erick on 9/13/26.
//


#include <cstdint>

#include "caudalimetro.h"

struct Caudalimetro {
    uint8_t pin;
    volatile uint32_t pulsos;
};

static Caudalimetro tubos[] = {
    {14, 0},  // tubo 1
    {12, 0},  // tubo 2
    {13, 0},  // tubo 3
    {2,  0},  // tubo 4
};

static_assert(std::size(tubos) == CAUDAL_N,
              "CAUDAL_N no coincide con el numero de tubos");

static void IRAM_ATTR isr0() { tubos[0].pulsos++; }
static void IRAM_ATTR isr1() { tubos[1].pulsos++; }
static void IRAM_ATTR isr2() { tubos[2].pulsos++; }
static void IRAM_ATTR isr3() { tubos[3].pulsos++; }

static void (*isrs[])() = {isr0, isr1, isr2, isr3};

void caudalimetroInit() {
    for (size_t i = 0; i < CAUDAL_N; i++) {
        pinMode(tubos[i].pin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(tubos[i].pin), isrs[i], FALLING);
    }
}


void caudalimetroSnapshot(uint32_t destino[CAUDAL_N]) {
    noInterrupts();
    for (size_t i = 0; i < CAUDAL_N; i++) {
        destino[i] = tubos[i].pulsos;
        tubos[i].pulsos = 0;
    }
    interrupts();
}