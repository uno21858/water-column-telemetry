#include "protocolo.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

const char *resultadoTexto(ResultadoLinea r) {
    switch (r) {
        case ResultadoLinea::ok:      return "ok";
        case ResultadoLinea::formato: return "formato";
        case ResultadoLinea::valor:   return "valor";
    }
    return "?";
}

ResultadoLinea lineaParsear(const char *linea, float valores[TRAMA_N]) {
    static constexpr char PREFIJO[] = "DATA,";
    if (strncmp(linea, PREFIJO, sizeof(PREFIJO) - 1) != 0) {
        return ResultadoLinea::formato;
    }

    float tmp[TRAMA_N + 1];  // [0] es tiempo_ms, se descarta
    const char *p = linea + sizeof(PREFIJO) - 1;

    for (size_t i = 0; i <= TRAMA_N; i++) {
        char *fin = nullptr;
        float v = strtof(p, &fin);
        if (fin == p || !std::isfinite(v)) {
            return ResultadoLinea::valor;
        }
        tmp[i] = v;

        // Después de cada número va una coma, menos en el último
        char esperado = i < TRAMA_N ? ',' : '\0';
        if (*fin != esperado) {
            return ResultadoLinea::formato;
        }
        p = fin + 1;
    }

    memcpy(valores, tmp + 1, sizeof(float) * TRAMA_N);
    return ResultadoLinea::ok;
}
