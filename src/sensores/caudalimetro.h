//
// Created by Erick on 9/13/26.
//

#ifndef SERVICIOBECARIOSENSORESAGUA_CAUDIMETRO_H
#define SERVICIOBECARIOSENSORESAGUA_CAUDIMETRO_H

#include <Arduino.h>

/// Numero de caudalimetros conectados.
constexpr size_t CAUDAL_N = 4;

/// Configura los pines de los caudalimetros y engancha sus interrupciones.
/// Llamar una sola vez desde setup().
void caudalimetroInit();

/// Copia los pulsos acumulados de cada caudalimetro y reinicia los contadores.
///
/// Cada llamada devuelve los pulsos contados desde la llamada anterior, por lo
/// que debe invocarse una sola vez por ciclo de medicion: una segunda llamada
/// seguida leeria casi cero.
///
/// La lectura y el reinicio se hacen con las interrupciones desactivadas para
/// que sean indivisibles. Sin esa proteccion, un pulso que llegue entre ambas
/// operaciones se perderia sin dejar rastro.
///
/// @param destino Arreglo de CAUDAL_N elementos donde se depositan los pulsos,
///                indexado por tubo (destino[0] es el tubo 1).
void caudalimetroSnapshot(uint32_t destino[CAUDAL_N]);


#endif //SERVICIOBECARIOSENSORESAGUA_CAUDIMETRO_H
