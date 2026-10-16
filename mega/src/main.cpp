//
// Created by Erick on 10/16/26.
//


#include <Arduino.h>

// ---- Configuración: ajustar a lo que está cableado en el banco ----

constexpr uint8_t PINES_PH[] = {A0, A1, A2};
constexpr uint8_t PINES_TDS[] = {A4, A5, A6};

// Caudalímetros: solo pines con interrupción externa (2, 3, 18, 19).
constexpr uint8_t PINES_CAUDAL[] = {2, 3};

constexpr uint32_t INTERVALO_MS = 60000;  // mismo intervalo que la WeMos
constexpr uint8_t MUESTRAS = 50;          // mismo promedio que la calibración de pH
constexpr uint8_t ESPERA_MS = 10;         // separación entre muestras, igual que la calibración
constexpr uint32_t BAUD_WEMOS = 9600;

// ---- Fin de configuración ----

constexpr size_t N_PH = sizeof(PINES_PH) / sizeof(PINES_PH[0]);
constexpr size_t N_TDS = sizeof(PINES_TDS) / sizeof(PINES_TDS[0]);
constexpr size_t N_CAUDAL = sizeof(PINES_CAUDAL) / sizeof(PINES_CAUDAL[0]);

static_assert(N_CAUDAL <= 4, "El Mega solo tiene 4 pines de interrupcion libres");

volatile uint32_t pulsos[4];

template <uint8_t I>
void isrCaudal() {
    pulsos[I]++;
}

void (*const ISRS[4])() = {isrCaudal<0>, isrCaudal<1>, isrCaudal<2>, isrCaudal<3>};

char linea[160];
size_t pos = 0;

template <typename... T>
void agregar(const char *fmt, T... args) {
    if (pos >= sizeof(linea)) return;
    int n = snprintf(linea + pos, sizeof(linea) - pos, fmt, args...);
    if (n > 0) pos += n;
}

// Promedio en décimas de cuenta del ADC (0 a 10230).
// valor / 10 = cuentas; cuentas * 5.0 / 1023 = voltios, misma fórmula de la calibración.
uint16_t leerAnalogico(uint8_t pin) {
    analogRead(pin);  // se descarta la primera lectura tras cambiar de canal
    delayMicroseconds(200);
    uint32_t suma = 0;
    for (uint8_t i = 0; i < MUESTRAS; i++) {
        suma += analogRead(pin);
        delay(ESPERA_MS);
    }
    return (suma * 10 + MUESTRAS / 2) / MUESTRAS;
}

void agregarAnalogicos(const uint8_t *pines, size_t n) {
    for (size_t i = 0; i < n; i++) {
        agregar(",%u", leerAnalogico(pines[i]));
    }
}

// Formato: $H,<n>,<ph...>,T,<n>,<tds...>,C,<n>,<pulsos...>*HH
// H y T en décimas de cuenta del ADC; C en pulsos acumulados del intervalo.
// HH = XOR en hex de todo lo que hay entre '$' y '*'.
void enviar() {
    uint32_t snap[4];
    noInterrupts();
    for (size_t i = 0; i < N_CAUDAL; i++) {
        snap[i] = pulsos[i];
        pulsos[i] = 0;
    }
    interrupts();

    pos = 0;
    agregar("H,%u", (unsigned)N_PH);
    agregarAnalogicos(PINES_PH, N_PH);
    agregar(",T,%u", (unsigned)N_TDS);
    agregarAnalogicos(PINES_TDS, N_TDS);
    agregar(",C,%u", (unsigned)N_CAUDAL);
    for (size_t i = 0; i < N_CAUDAL; i++) {
        agregar(",%lu", (unsigned long)snap[i]);
    }

    if (pos >= sizeof(linea)) {
        Serial.println(F("ERROR: linea demasiado larga, lote descartado"));
        return;
    }

    uint8_t chk = 0;
    for (size_t i = 0; i < pos; i++) chk ^= (uint8_t)linea[i];

    char cola[4];
    snprintf(cola, sizeof(cola), "*%02X", chk);

    Serial2.print('$');
    Serial2.print(linea);
    Serial2.println(cola);

    Serial.print('$');
    Serial.print(linea);
    Serial.println(cola);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(BAUD_WEMOS);
    analogReference(DEFAULT);

    for (size_t i = 0; i < N_CAUDAL; i++) {
        int irq = digitalPinToInterrupt(PINES_CAUDAL[i]);
        if (irq == NOT_AN_INTERRUPT) {
            Serial.print(F("ERROR: pin sin interrupcion: "));
            Serial.println(PINES_CAUDAL[i]);
            continue;
        }
        pinMode(PINES_CAUDAL[i], INPUT_PULLUP);
        attachInterrupt(irq, ISRS[i], FALLING);
    }

    Serial.print(F("Mega listo, primera linea en "));
    Serial.print(INTERVALO_MS / 1000);
    Serial.println(F(" s"));
}

void loop() {
    static uint32_t ultimo = millis();
    if (millis() - ultimo >= INTERVALO_MS) {
        ultimo += INTERVALO_MS;  // sin deriva acumulada
        enviar();
    }
}