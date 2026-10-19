#include <Arduino.h>

// ============================================================================
// SISTEMA DE ADQUISICION FINAL
// Arduino Mega 2560
//
// 3 sensores de pH
// 3 sensores TDS Meter V1.0
// 4 medidores de flujo
// Wemos D1 ESP8266 por Serial2
// Processing por USB/Serial
//
// SIN microSD
// SIN ADS1115
// SIN sensor de presion diferencial
//
// Trama enviada por USB y Serial2:
//
// DATA,tiempo_ms,pH1,pH2,pH3,EC1,EC2,EC3,TDS1,TDS2,TDS3,F1,F2,F3,F4
//
// ============================================================================


// ============================================================================
// CONFIGURACION GENERAL
// ============================================================================

// IMPORTANTE:
// Estas calibraciones fueron obtenidas usando VREF = 5.0.
// No cambiar VREF sin recalibrar los sensores.
const float VREF = 5.0;

// Intervalo nominal entre reportes
const unsigned long INTERVALO_MS = 1000;

unsigned long tiempoAnterior = 0;


// ============================================================================
// SENSORES DE pH
// ============================================================================
//
// pH 1 -> A0
// pH 2 -> A1
// pH 3 -> A2
//
// Ecuacion:
//
// pH = m*V + b
//
// Calibraciones actuales
// ============================================================================

const uint8_t PIN_PH[3] = {
  A0,
  A1,
  A2
};

const float PH_M[3] = {
  -5.67208390,
  -6.03940842,
  -5.56072783
};

const float PH_B[3] = {
  14.80293456,
  14.20090073,
  13.96646129
};


// Valores medidos
float voltajePH[3] = {
  0.0,
  0.0,
  0.0
};

float valorPH[3] = {
  0.0,
  0.0,
  0.0
};


// ============================================================================
// SENSORES TDS / CONDUCTIVIDAD
// ============================================================================
//
// TDS 1 -> A4
// TDS 2 -> A5
// TDS 3 -> A6
// ============================================================================

const uint8_t PIN_TDS[3] = {
  A4,
  A5,
  A6
};

// Temperatura utilizada actualmente para compensacion.
// Cambiar cuando tengas medicion real de temperatura.
float temperatura = 30.0;

// Factores de calibracion actuales
float K_TDS[3] = {
  0.878,
  1.04,
  2.02
};

// Filtro exponencial
const float ALFA_TDS = 0.2;

float ecFiltrada[3] = {
  0.0,
  0.0,
  0.0
};

bool ecInicializada[3] = {
  false,
  false,
  false
};


// Valores medidos
float voltajeTDS[3] = {
  0.0,
  0.0,
  0.0
};

float ec[3] = {
  0.0,
  0.0,
  0.0
};

float tds[3] = {
  0.0,
  0.0,
  0.0
};


// ============================================================================
// MEDIDORES DE FLUJO
// ============================================================================
//
// Flujo 1 -> D2
// Flujo 2 -> D3
// Flujo 3 -> D19
// Flujo 4 -> D18
//
// Factor actual:
// 313.5 pulsos / litro
// ============================================================================

const uint8_t PIN_FLUJO[4] = {
  2,
  3,
  19,
  18
};

const float PULSOS_POR_LITRO = 313.5;


// Contadores de interrupcion
volatile unsigned long pulsos1 = 0;
volatile unsigned long pulsos2 = 0;
volatile unsigned long pulsos3 = 0;
volatile unsigned long pulsos4 = 0;


// Caudales calculados
float caudal[4] = {
  0.0,
  0.0,
  0.0,
  0.0
};


// ============================================================================
// INTERRUPCIONES
// ============================================================================

void ISR_Flujo1() {
  pulsos1++;
}

void ISR_Flujo2() {
  pulsos2++;
}

void ISR_Flujo3() {
  pulsos3++;
}

void ISR_Flujo4() {
  pulsos4++;
}


// ============================================================================
// LECTURA pH
// ============================================================================
//
// Se conserva una lectura promediada para mantener consistencia
// con la calibracion que hemos utilizado.
// ============================================================================

float leerVoltajePH(uint8_t pin) {

  const uint8_t NUM_MUESTRAS = 30;

  uint32_t suma = 0;

  // Lectura de descarte despues de cambiar de canal ADC
  analogRead(pin);
  delayMicroseconds(200);

  for (uint8_t i = 0; i < NUM_MUESTRAS; i++) {

    suma += analogRead(pin);

    delay(10);
  }

  float promedio =
    suma / (float)NUM_MUESTRAS;

  return promedio * VREF / 1023.0;
}


// ============================================================================
// CALCULO pH
// ============================================================================

void leerPH() {

  for (uint8_t i = 0; i < 3; i++) {

    voltajePH[i] =
      leerVoltajePH(PIN_PH[i]);

    valorPH[i] =
      PH_M[i] * voltajePH[i]
      +
      PH_B[i];
  }
}


// ============================================================================
// LECTURA TDS
// ============================================================================
//
// Se conserva el metodo de mediana de tu codigo de calibracion.
// ============================================================================

float leerVoltajeTDS(uint8_t pin) {

  int buf[30];

  // Lectura de descarte
  analogRead(pin);

  for (int i = 0; i < 30; i++) {

    buf[i] =
      analogRead(pin);

    delay(10);
  }


  // Ordenamiento
  for (int i = 0; i < 29; i++) {

    for (int j = i + 1; j < 30; j++) {

      if (buf[j] < buf[i]) {

        int temporal =
          buf[i];

        buf[i] =
          buf[j];

        buf[j] =
          temporal;
      }
    }
  }


  // Se mantiene /1024.0 para conservar exactamente
  // la base usada por la calibracion TDS actual.
  return buf[15] * VREF / 1024.0;
}


// ============================================================================
// TDS CRUDO
// ============================================================================

float calcularTDSCrudo(float voltaje) {

  float coefTemperatura =
    1.0
    +
    0.02
    *
    (temperatura - 25.0);


  float voltajeCompensado =
    voltaje
    /
    coefTemperatura;


  float resultado =
    (
      133.42
      *
      voltajeCompensado
      *
      voltajeCompensado
      *
      voltajeCompensado

      -

      255.86
      *
      voltajeCompensado
      *
      voltajeCompensado

      +

      857.39
      *
      voltajeCompensado
    )
    *
    0.5;


  return resultado;
}


// ============================================================================
// FILTRO EC
// ============================================================================

float suavizarEC(
  uint8_t sensor,
  float ecNueva
) {

  if (!ecInicializada[sensor]) {

    ecFiltrada[sensor] =
      ecNueva;

    ecInicializada[sensor] =
      true;
  }


  ecFiltrada[sensor] +=
    ALFA_TDS
    *
    (
      ecNueva
      -
      ecFiltrada[sensor]
    );


  return ecFiltrada[sensor];
}


// ============================================================================
// LECTURA CONDUCTIVIDAD
// ============================================================================

void leerTDS() {

  for (uint8_t i = 0; i < 3; i++) {

    voltajeTDS[i] =
      leerVoltajeTDS(
        PIN_TDS[i]
      );


    float tdsCrudo =
      calcularTDSCrudo(
        voltajeTDS[i]
      );


    // Conductividad calibrada en uS/cm
    float ecNueva =
      K_TDS[i]
      *
      tdsCrudo
      *
      2.0;


    ec[i] =
      suavizarEC(
        i,
        ecNueva
      );


    // Conversion empleada actualmente
    // TDS = EC * 0.5
    tds[i] =
      ec[i]
      *
      0.5;
  }
}


// ============================================================================
// CALCULO DE CAUDAL
// ============================================================================

void calcularCaudal(
  float tiempoSegundos
) {

  unsigned long p1;
  unsigned long p2;
  unsigned long p3;
  unsigned long p4;


  // Copiar y borrar contadores atomicamente
  noInterrupts();

  p1 = pulsos1;
  p2 = pulsos2;
  p3 = pulsos3;
  p4 = pulsos4;

  pulsos1 = 0;
  pulsos2 = 0;
  pulsos3 = 0;
  pulsos4 = 0;

  interrupts();


  if (tiempoSegundos <= 0.0) {
    return;
  }


  // Pulsos -> litros -> L/min

  caudal[0] =
    (p1 / PULSOS_POR_LITRO)
    *
    (60.0 / tiempoSegundos);


  caudal[1] =
    (p2 / PULSOS_POR_LITRO)
    *
    (60.0 / tiempoSegundos);


  caudal[2] =
    (p3 / PULSOS_POR_LITRO)
    *
    (60.0 / tiempoSegundos);


  caudal[3] =
    (p4 / PULSOS_POR_LITRO)
    *
    (60.0 / tiempoSegundos);
}


// ============================================================================
// ENVIAR TRAMA
// ============================================================================
//
// Se utiliza la misma funcion para:
//
// Serial  -> Processing / PC
// Serial2 -> Wemos D1
//
// ============================================================================

void enviarTrama(Print &salida) {

  salida.print("DATA,");

  // Tiempo
  salida.print(millis());
  salida.print(",");


  // pH
  salida.print(valorPH[0], 3);
  salida.print(",");

  salida.print(valorPH[1], 3);
  salida.print(",");

  salida.print(valorPH[2], 3);
  salida.print(",");


  // Conductividad
  salida.print(ec[0], 1);
  salida.print(",");

  salida.print(ec[1], 1);
  salida.print(",");

  salida.print(ec[2], 1);
  salida.print(",");


  // TDS
  salida.print(tds[0], 1);
  salida.print(",");

  salida.print(tds[1], 1);
  salida.print(",");

  salida.print(tds[2], 1);
  salida.print(",");


  // Caudal
  salida.print(caudal[0], 3);
  salida.print(",");

  salida.print(caudal[1], 3);
  salida.print(",");

  salida.print(caudal[2], 3);
  salida.print(",");

  salida.println(caudal[3], 3);
}


// ============================================================================
// SETUP
// ============================================================================

void setup() {

  // USB -> PC / Processing
  Serial.begin(115200);

  // Mega TX2 pin 16 -> RX Wemos
  // Mega RX2 pin 17 <- TX Wemos
  Serial2.begin(9600);  // 9600: la WeMos lo lee por SoftwareSerial, a 115200 pierde bytes


  analogReference(DEFAULT);


  // --------------------------------------------------------------------------
  // pH
  // --------------------------------------------------------------------------

  for (uint8_t i = 0; i < 3; i++) {

    pinMode(
      PIN_PH[i],
      INPUT
    );
  }


  // --------------------------------------------------------------------------
  // TDS
  // --------------------------------------------------------------------------

  for (uint8_t i = 0; i < 3; i++) {

    pinMode(
      PIN_TDS[i],
      INPUT
    );
  }


  // --------------------------------------------------------------------------
  // FLUJO
  // --------------------------------------------------------------------------

  pinMode(
    PIN_FLUJO[0],
    INPUT_PULLUP
  );

  pinMode(
    PIN_FLUJO[1],
    INPUT_PULLUP
  );

  pinMode(
    PIN_FLUJO[2],
    INPUT_PULLUP
  );

  pinMode(
    PIN_FLUJO[3],
    INPUT_PULLUP
  );


  attachInterrupt(
    digitalPinToInterrupt(
      PIN_FLUJO[0]
    ),
    ISR_Flujo1,
    FALLING
  );


  attachInterrupt(
    digitalPinToInterrupt(
      PIN_FLUJO[1]
    ),
    ISR_Flujo2,
    FALLING
  );


  attachInterrupt(
    digitalPinToInterrupt(
      PIN_FLUJO[2]
    ),
    ISR_Flujo3,
    FALLING
  );


  attachInterrupt(
    digitalPinToInterrupt(
      PIN_FLUJO[3]
    ),
    ISR_Flujo4,
    FALLING
  );


  delay(1000);


  tiempoAnterior =
    millis();


  Serial.println(
    "=== SISTEMA DE ADQUISICION INICIADO ==="
  );

  Serial.println(
    "pH: A0 A1 A2"
  );

  Serial.println(
    "TDS: A4 A5 A6"
  );

  Serial.println(
    "Flujo: D2 D3 D19 D18"
  );

  Serial.println(
    "Wemos: Serial2 TX2=16 RX2=17"
  );

  Serial.println(
    "Esperando datos..."
  );
}


// ============================================================================
// LOOP
// ============================================================================

void loop() {

  unsigned long ahora =
    millis();


  if (
    ahora - tiempoAnterior
    >=
    INTERVALO_MS
  ) {

    // Tiempo REAL transcurrido entre ventanas de conteo
    float tiempoSegundos =
      (
        ahora
        -
        tiempoAnterior
      )
      /
      1000.0;


    tiempoAnterior =
      ahora;


    // --------------------------------------------------------
    // CAUDAL
    // --------------------------------------------------------

    calcularCaudal(
      tiempoSegundos
    );


    // --------------------------------------------------------
    // pH
    // --------------------------------------------------------

    leerPH();


    // --------------------------------------------------------
    // CONDUCTIVIDAD
    // --------------------------------------------------------

    leerTDS();


    // --------------------------------------------------------
    // PC / PROCESSING
    // --------------------------------------------------------

    enviarTrama(
      Serial
    );


    // --------------------------------------------------------
    // WEMOS
    // --------------------------------------------------------

    enviarTrama(
      Serial2
    );
  }
}