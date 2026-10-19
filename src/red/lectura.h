#pragma once

#include <cstdint>

/// Una fila de sbagua.lecturas: qué sensor y su valor.
struct Lectura {
    uint16_t sensor_id;  // smallint en Postgres
    float valor;         // double precision en Postgres
};
