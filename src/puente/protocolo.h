#pragma once

#include <cstddef>

/// Valores por línea después del tiempo:
/// pH1..3, EC1..3, TDS1..3, F1..4
constexpr size_t TRAMA_N = 13;

enum class ResultadoLinea {
    ok,
    formato,  // no empieza con "DATA," o sobran/faltan campos
    valor,    // un campo no es número
};

const char *resultadoTexto(ResultadoLinea r);

/// Lee una línea del Mega con el formato
/// DATA,tiempo_ms,pH1,pH2,pH3,EC1,EC2,EC3,TDS1,TDS2,TDS3,F1,F2,F3,F4
///
/// La línea no trae checksum, así que se valida estricto: exactamente
/// TRAMA_N + 1 campos y todos numéricos. Es todo o nada: si algo falla,
/// valores no se toca.
///
/// @param linea   Línea completa, sin \r ni \n.
/// @param valores Donde se escriben los TRAMA_N valores (sin el tiempo).
ResultadoLinea lineaParsear(const char *linea, float valores[TRAMA_N]);
