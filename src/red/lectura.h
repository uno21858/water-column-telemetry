#pragma once

#include <cstdint>

/// Una fila de sbagua.lecturas: qué sensor y su valor crudo.
struct Lectura {
    uint16_t sensor_id;  // smallint en Postgres
    int32_t valor;       // integer en Postgres
};
