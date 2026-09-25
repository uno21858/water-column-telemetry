//
// Created by Erick on 9/22/26.
//

#include "../db.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "red.h"
#include "../include/secretos.h"

// Debe coincidir con los id de sbagua.sensores. Indice 0 es el tubo 1.
static constexpr uint8_t SENSOR_ID_CAUDAL[CAUDAL_N] = {1, 2, 3, 4};

bool dbEnviarCaudal(const uint32_t p[CAUDAL_N]) {
    if (!redConectada()) {
        return false;
    }

    char cuerpo[256];
    size_t n = 0;
    cuerpo[n++] = '[';

    for (size_t i = 0; i < CAUDAL_N; i++) {
        int escrito = snprintf(cuerpo + n, sizeof(cuerpo) - n,
                               "%s{\"sensor_id\":%u,\"valor\":%u}",
                               i ? "," : "",
                               static_cast<unsigned>(SENSOR_ID_CAUDAL[i]),
                               static_cast<unsigned>(p[i]));
        if (escrito < 0 || static_cast<size_t>(escrito) >= sizeof(cuerpo) - n) {
            return false;
        }
        n += escrito;
    }

    if (n + 2 > sizeof(cuerpo)) {
        return false;
    }
    cuerpo[n++] = ']';
    cuerpo[n] = '\0';

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

    int codigo = http.POST(reinterpret_cast<const uint8_t*>(cuerpo), n);
    http.end();

    if (codigo != 201) {
        Serial.printf("POST fallo: %d\n", codigo);
        return false;
    }
    return true;
}