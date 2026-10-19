//
// Created by Erick on 9/22/26.
//

#ifndef SERVICIOBECARIOSENSORESAGUA_DB_H
#define SERVICIOBECARIOSENSORESAGUA_DB_H

#pragma once

#include <Arduino.h>

#include "lectura.h"
#include "../sensores/caudalimetro.h"

/// Envia un lote de lecturas a Supabase en un solo POST.
///
/// Todas las filas del lote quedan con el mismo timestamp, porque Postgres
/// asigna now() por transaccion y un POST con arreglo es una sola transaccion.
/// Por lo mismo es todo o nada: si una fila falla (por ejemplo un sensor_id
/// que no existe), no se inserta ninguna.
///
/// @return true si Supabase respondio 201.
bool dbEnviar(const Lectura *lecturas, size_t n);

/// Envia un snapshot de caudal de las torres como un solo lote.
///
/// @param p Pulsos acumulados por tubo en el ultimo intervalo.
/// @return true si Supabase respondio 201.
bool dbEnviarCaudal(const uint32_t p[CAUDAL_N]);

#endif //SERVICIOBECARIOSENSORESAGUA_DB_H
