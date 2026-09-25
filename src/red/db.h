//
// Created by Erick on 9/22/26.
//

#ifndef SERVICIOBECARIOSENSORESAGUA_DB_H
#define SERVICIOBECARIOSENSORESAGUA_DB_H

#pragma once

#include <Arduino.h>

#include "../sensores/caudalimetro.h"

/// Envia un snapshot de caudal a Supabase como un solo lote.
///
/// Todas las filas del lote quedan con el mismo timestamp, porque Postgres
/// asigna now() por transaccion y un POST con arreglo es una sola transaccion.
///
/// @param p Pulsos acumulados por tubo en el ultimo intervalo.
/// @return true si Supabase respondio 201.
bool dbEnviarCaudal(const uint32_t p[CAUDAL_N]);

#endif //SERVICIOBECARIOSENSORESAGUA_DB_H
