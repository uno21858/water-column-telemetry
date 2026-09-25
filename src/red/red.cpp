#include "red.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "../secretos.h"

static constexpr uint32_t TIMEOUT_MS = 15000;

void redInit() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("IP %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("sin WiFi, status=%d\n", WiFi.status());
    }
}

bool redConectada() {
    return WiFi.status() == WL_CONNECTED;
}