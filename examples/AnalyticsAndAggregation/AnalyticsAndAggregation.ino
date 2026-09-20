/*
  =============================================================================
  MICRODB - CONSULTAS COMPLEJAS: AGREGACIONES, ESTADÍSTICAS Y STREAMING
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  Demostración de analítica avanzada sobre la tarjeta SD:
  1. Cálculo de Métricas Estadísticas en Streaming: MIN, MAX, SUM, AVG
  2. Detección de Valores Atípicos (Outliers / Anomalías térmicas o eléctricas)
  3. Agrupación por Rangos (Bucketing / Histograma de frecuencias)
  4. Reducción de Muestreo (Downsampling) en tiempo real
  Todo con memoria constante O(1) RAM sin importar la cantidad de registros.
  =============================================================================
*/

#include <SPI.h>
#include <SD.h>
#include <MicroDB.h>

#if defined(ESP32)
  const uint8_t SD_CS_PIN = 5;
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  const uint8_t SD_CS_PIN = 53;
#else
  const uint8_t SD_CS_PIN = 4;
#endif

// Esquema de telemetría industrial
struct TelemetryRecord {
  uint32_t timestamp;  // Epoch
  uint8_t  sensorId;   // ID del sensor (1=Caldera, 2=Motor, 3=Refrigerador)
  float    temperature;// Grados Celsius
  float    pressure;   // Bares
  float    powerDraw;  // Watts
};

MicroDB db;
Table<TelemetryRecord> telemetryTable;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("    MICRODB - ANALITICA AVANZADA Y AGREGACIONES EN SD   "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_STATS", SD_CS_PIN)) {
    Serial.println(F("Error al iniciar SD"));
    while (1);
  }

  telemetryTable = db.openTable<TelemetryRecord>("telem");

  // Registrar metadatos de columnas para lectura externa
  telemetryTable
    .addColumn("timestamp",   TYPE_UINT32, offsetof(TelemetryRecord, timestamp),   sizeof(uint32_t))
    .addColumn("sensorId",    TYPE_UINT8,  offsetof(TelemetryRecord, sensorId),    sizeof(uint8_t))
    .addColumn("temperature", TYPE_FLOAT,  offsetof(TelemetryRecord, temperature), sizeof(float))
    .addColumn("pressure",    TYPE_FLOAT,  offsetof(TelemetryRecord, pressure),    sizeof(float))
    .addColumn("powerDraw",   TYPE_FLOAT,  offsetof(TelemetryRecord, powerDraw),   sizeof(float));
  telemetryTable.saveSchema();

  telemetryTable.truncate(); // Limpiar para prueba controlada

  // --------------------------------------------------------------------------
  // 1. GENERACIÓN DE LOTE DE DATOS DE PRUEBA (BATCH INSERT)
  // --------------------------------------------------------------------------
  Serial.println(F("[1] Generando 50 lecturas de telemetria en la SD..."));

  uint32_t baseTime = 1774180000;
  for (int i = 0; i < 50; i++) {
    TelemetryRecord t;
    t.timestamp = baseTime + (i * 60); // 1 muestra cada minuto
    t.sensorId = (i % 3) + 1;          // Sensores 1, 2 o 3

    // Generar variaciones realistas con una anomalía intencional en el registro 25
    if (i == 25) {
      t.temperature = 115.8f; // ¡Anomalía térmica!
      t.pressure = 8.5f;
    } else {
      t.temperature = 22.0f + (sin(i * 0.3f) * 6.0f); // 16°C a 28°C
      t.pressure = 2.0f + ((i % 5) * 0.2f);
    }
    t.powerDraw = 120.0f + (i * 2.5f);

    telemetryTable.insert(t);
  }
  Serial.print(F("--> Total registros en SD: "));
  Serial.println(telemetryTable.count());

  // --------------------------------------------------------------------------
  // 2. CÁLCULO DE AGREGACIONES (MIN, MAX, SUM, AVG) EN STREAMING (O(1) RAM)
  // --------------------------------------------------------------------------
  Serial.println(F("\n[2] Calculando metricas para el 'Sensor #1' (Caldera):"));

  float minTemp = 9999.0f;
  float maxTemp = -9999.0f;
  float sumTemp = 0.0f;
  float sumPower = 0.0f;
  uint32_t count = 0;

  telemetryTable.where(
    [](uint32_t id, const TelemetryRecord& t) { return t.sensorId == 1; },
    [&](uint32_t id, const TelemetryRecord& t) {
      if (t.temperature < minTemp) minTemp = t.temperature;
      if (t.temperature > maxTemp) maxTemp = t.temperature;
      sumTemp += t.temperature;
      sumPower += t.powerDraw;
      count++;
    }
  );

  if (count > 0) {
    float avgTemp = sumTemp / count;
    float avgPower = sumPower / count;

    Serial.print(F("  - Muestras analizadas: ")); Serial.println(count);
    Serial.print(F("  - Temp Minima:         ")); Serial.print(minTemp, 2); Serial.println(F(" °C"));
    Serial.print(F("  - Temp Maxima:         ")); Serial.print(maxTemp, 2); Serial.println(F(" °C"));
    Serial.print(F("  - Temp Promedio (AVG): ")); Serial.print(avgTemp, 2); Serial.println(F(" °C"));
    Serial.print(F("  - Consumo Promedio:    ")); Serial.print(avgPower, 2); Serial.println(F(" W"));
  }

  // --------------------------------------------------------------------------
  // 3. DETECCIÓN DE ANOMALÍAS Y OUTLIERS (ALERTA DE SEGURIDAD)
  // --------------------------------------------------------------------------
  Serial.println(F("\n[3] Buscando anomalias criticas (Temp > 80 °C o Presion > 5.0 bar):"));

  uint32_t anomaliesFound = 0;
  telemetryTable.where(
    [](uint32_t id, const TelemetryRecord& t) {
      return (t.temperature > 80.0f) || (t.pressure > 5.0f);
    },
    [&](uint32_t id, const TelemetryRecord& t) {
      anomaliesFound++;
      Serial.print(F("  [ALERTA CRITICA] Registro #")); Serial.print(id);
      Serial.print(F(" | Sensor: ")); Serial.print(t.sensorId);
      Serial.print(F(" | Temp: ")); Serial.print(t.temperature);
      Serial.print(F(" °C | Presion: ")); Serial.print(t.pressure);
      Serial.println(F(" bar"));
    }
  );
  if (anomaliesFound == 0) {
    Serial.println(F("  -> No se detectaron anomalias."));
  }

  // --------------------------------------------------------------------------
  // 4. HISTOGRAMA / BUCKETING POR RANGOS DE TEMPERATURA
  // --------------------------------------------------------------------------
  Serial.println(F("\n[4] Histograma de distribucion de temperaturas en la planta:"));
  uint32_t rangeCold = 0;   // < 20 °C
  uint32_t rangeNormal = 0; // 20 °C a 30 °C
  uint32_t rangeHot = 0;    // > 30 °C

  telemetryTable.forEach([&](uint32_t id, const TelemetryRecord& t) {
    if (t.temperature < 20.0f) {
      rangeCold++;
    } else if (t.temperature <= 30.0f) {
      rangeNormal++;
    } else {
      rangeHot++;
    }
  });

  Serial.print(F("  * Baja (<20 °C):       ")); Serial.print(rangeCold);   Serial.println(F(" lecturas"));
  Serial.print(F("  * Normal (20 a 30 °C): ")); Serial.print(rangeNormal); Serial.println(F(" lecturas"));
  Serial.print(F("  * Alta / Critica (>30°): ")); Serial.print(rangeHot);  Serial.println(F(" lecturas"));

  Serial.println(F("\n========================================================"));
  Serial.println(F("         CONSULTAS ANALITICAS COMPLETADAS               "));
  Serial.println(F("========================================================"));
}

void loop() {}
