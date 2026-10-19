#pragma once

#include <cstddef>
#include <cstdint>

#include "../red/lectura.h"

/// Un grupo de la línea del Mega: su letra, cuántos valores trae y a qué
/// sensor va cada uno, en el orden en que llegan.
struct Grupo {
    char etiqueta;
    const uint16_t *sensores;
    size_t n;
};

enum class ResultadoLinea {
    ok,
    formato,    // falta '$' o '*HH', sobra o falta un campo
    checksum,   // la línea llegó corrupta
    grupo,      // letra desconocida o repetida, o falta un grupo
    conteo,     // el Mega manda otro número de valores que el configurado aquí
    valor,      // un valor no es entero positivo o no cabe en integer
    capacidad,  // el arreglo destino es muy chico
};

const char *resultadoTexto(ResultadoLinea r);

/// Valida y convierte una línea del Mega en lecturas.
///
/// Formato: $<grupo>,<grupo>,...*HH
/// Cada grupo es <letra>,<n>,<v1>,...,<vn>. HH es el XOR en hex de todo lo que
/// hay entre '$' y '*'. Un valor vacío es un sensor sin dato y no genera lectura.
///
/// Es todo o nada: ante cualquier error n queda en 0. Así nunca se sube un lote
/// a medias ni un valor asignado al sensor equivocado.
///
/// @param linea    Línea completa, sin \r ni \n.
/// @param grupos   Grupos esperados. Cada uno debe venir exactamente una vez.
/// @param destino  Donde se escriben las lecturas.
/// @param cap      Capacidad de destino.
/// @param n        Lecturas escritas.
ResultadoLinea lineaParsear(const char *linea,
                            const Grupo *grupos, size_t gruposN,
                            Lectura *destino, size_t cap, size_t &n);
