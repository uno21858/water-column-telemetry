#include "protocolo.h"

#include <cstring>

namespace {

/// Un campo de la línea, de ini hasta fin sin incluirlo. No copia nada.
struct Campo {
    const char *ini;
    const char *fin;
};

int hexValor(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/// Toma el siguiente campo separado por ',' y deja p después de la coma.
/// Regresa false cuando ya se consumió todo hasta fin.
bool siguiente(const char *&p, const char *fin, Campo &c) {
    if (p > fin) {
        return false;
    }
    c.ini = p;
    while (p < fin && *p != ',') {
        p++;
    }
    c.fin = p;
    p++;
    return true;
}

/// Entero decimal sin signo, solo dígitos, sin pasarse de max.
bool entero(const Campo &c, uint32_t max, uint32_t &v) {
    if (c.ini == c.fin) {
        return false;
    }
    uint32_t r = 0;
    for (const char *q = c.ini; q < c.fin; q++) {
        if (*q < '0' || *q > '9') {
            return false;
        }
        uint32_t d = static_cast<uint32_t>(*q - '0');
        if (r > (max - d) / 10) {
            return false;
        }
        r = r * 10 + d;
    }
    v = r;
    return true;
}

}  // namespace

const char *resultadoTexto(ResultadoLinea r) {
    switch (r) {
        case ResultadoLinea::ok:        return "ok";
        case ResultadoLinea::formato:   return "formato";
        case ResultadoLinea::checksum:  return "checksum";
        case ResultadoLinea::grupo:     return "grupo";
        case ResultadoLinea::conteo:    return "conteo";
        case ResultadoLinea::valor:     return "valor";
        case ResultadoLinea::capacidad: return "capacidad";
    }
    return "?";
}

ResultadoLinea lineaParsear(const char *linea,
                            const Grupo *grupos, size_t gruposN,
                            Lectura *destino, size_t cap, size_t &n) {
    n = 0;

    if (gruposN > 32) {  // los grupos vistos se marcan en un uint32_t
        return ResultadoLinea::grupo;
    }
    if (linea[0] != '$') {
        return ResultadoLinea::formato;
    }

    const char *asterisco = strrchr(linea, '*');
    if (asterisco == nullptr || strlen(asterisco) != 3) {
        return ResultadoLinea::formato;
    }
    int alto = hexValor(asterisco[1]);
    int bajo = hexValor(asterisco[2]);
    if (alto < 0 || bajo < 0) {
        return ResultadoLinea::formato;
    }

    uint8_t chk = 0;
    for (const char *q = linea + 1; q < asterisco; q++) {
        chk ^= static_cast<uint8_t>(*q);
    }
    if (chk != ((alto << 4) | bajo)) {
        return ResultadoLinea::checksum;
    }

    const char *p = linea + 1;
    Campo campo;
    uint32_t vistos = 0;
    size_t escritas = 0;

    while (siguiente(p, asterisco, campo)) {
        if (campo.fin - campo.ini != 1) {
            return ResultadoLinea::formato;
        }

        size_t g = 0;
        while (g < gruposN && grupos[g].etiqueta != *campo.ini) {
            g++;
        }
        if (g == gruposN || (vistos & (1u << g))) {
            return ResultadoLinea::grupo;
        }
        vistos |= 1u << g;

        uint32_t cuenta = 0;
        if (!siguiente(p, asterisco, campo) || !entero(campo, 255, cuenta)) {
            return ResultadoLinea::formato;
        }
        if (cuenta != grupos[g].n) {
            return ResultadoLinea::conteo;
        }

        for (size_t i = 0; i < cuenta; i++) {
            if (!siguiente(p, asterisco, campo)) {
                return ResultadoLinea::formato;
            }
            if (campo.ini == campo.fin) {
                continue;  // sensor sin dato
            }
            uint32_t v = 0;
            if (!entero(campo, INT32_MAX, v)) {
                return ResultadoLinea::valor;
            }
            if (escritas >= cap) {
                return ResultadoLinea::capacidad;
            }
            destino[escritas++] = {grupos[g].sensores[i], static_cast<int32_t>(v)};
        }
    }

    uint32_t todos = gruposN == 32 ? UINT32_MAX : (1u << gruposN) - 1;
    if (vistos != todos) {
        return ResultadoLinea::grupo;
    }

    n = escritas;
    return ResultadoLinea::ok;
}
