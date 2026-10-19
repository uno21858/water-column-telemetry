//
// Created by Erick on 9/22/26.
//

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "db.h"
#include "red.h"
#include "../include/secretos.h"

// Debe coincidir con los id de sbagua.sensores. Indice 0 es el tubo 1.
static constexpr uint8_t SENSOR_ID_CAUDAL[CAUDAL_N] = {1, 2, 3, 4};

bool dbEnviarCaudal(const uint32_t p[CAUDAL_N]) {
    Lectura lote[CAUDAL_N];
    for (size_t i = 0; i < CAUDAL_N; i++) {
        lote[i] = {SENSOR_ID_CAUDAL[i], static_cast<float>(p[i])};
    }
    return dbEnviar(lote, CAUDAL_N);
}

bool dbEnviar(const Lectura *lecturas, size_t n) {
    if (n == 0 || !redConectada()) {
        return false;
    }

    char cuerpo[1024];
    size_t largo = 0;
    cuerpo[largo++] = '[';

    for (size_t i = 0; i < n; i++) {
        int escrito = snprintf(cuerpo + largo, sizeof(cuerpo) - largo,
                               "%s{\"sensor_id\":%u,\"valor\":%.4f}",
                               i ? "," : "",
                               static_cast<unsigned>(lecturas[i].sensor_id),
                               static_cast<double>(lecturas[i].valor));
        if (escrito < 0 || static_cast<size_t>(escrito) >= sizeof(cuerpo) - largo) {
            Serial.println("lote demasiado grande para el buffer");
            return false;
        }
        largo += escrito;
    }

    if (largo + 2 > sizeof(cuerpo)) {
        return false;
    }
    cuerpo[largo++] = ']';
    cuerpo[largo] = '\0';

    BearSSL::WiFiClientSecure cliente;
    cliente.setInsecure();

    HTTPClient http;
    if (!http.begin(cliente, String(SUPABASE_URL) + "/rest/v1/lecturas")) {
        return false;
    }

    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Content-Profile", "sbagua");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");

    int codigo = http.POST(reinterpret_cast<const uint8_t*>(cuerpo), largo);

    if (codigo != 201) {
        // El cuerpo trae el error de Postgres, p. ej. un sensor_id que no existe
        Serial.printf("POST fallo: %d %s\n", codigo,
                      codigo > 0 ? http.getString().c_str() : "");
        http.end();
        return false;
    }
    http.end();
    return true;
}