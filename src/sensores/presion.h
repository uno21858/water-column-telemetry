//
// Created by Erick on 9/28/26.
//

#ifndef SERVICIOBECARIOSENSORESAGUA_PRESION_H
#define SERVICIOBECARIOSENSORESAGUA_PRESION_H

#include <Arduino.h>

/// Numero de sensores de presion conectados (2 ADS1115 x 4 canales).
constexpr size_t PRESION_N = 8;

/// Valor que marca un sensor cuyo chip no respondió.
constexpr int16_t PRESION_SIN_DATO = INT16_MIN;

/// Arranca el bus I2C y los ADS1115. Llamar una sola vez desde setup().
/// Regresa false si no respondió ningún chip.
bool presionInit();

/// Lee los 8 sensores en cuentas crudas del ADC, promediando varias muestras.
/// Los sensores de un chip ausente quedan en PRESION_SIN_DATO.
/// Regresa false solo si no respondió ningún chip.
bool presionLeer(int16_t destino[PRESION_N]);

#endif //SERVICIOBECARIOSENSORESAGUA_PRESION_H
