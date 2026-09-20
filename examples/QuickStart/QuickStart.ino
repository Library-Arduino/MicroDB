/*
  =============================================================================
  MICRODB - QUICKSTART: INICIO RÁPIDO
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
*/

#include <SPI.h>
#include <SD.h>
#include <MicroDB.h>
// Ajustar Pin CS según placa (ESP32=5, Mega=53, Uno=4)
#if defined(ESP32)
  const uint8_t SD_CS_PIN = 5;
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  const uint8_t SD_CS_PIN = 53;
#else
  const uint8_t SD_CS_PIN = 4;
#endif

// Definición de la estructura de datos
struct SensorLog {
  uint32_t timestamp;
  float    temperature;
  float    humidity;
  bool     isAlert;
};

MicroDB db;
Table<SensorLog> logs;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("              MICRODB - GUIA RAPIDA (QUICKSTART)        "));
  Serial.println(F("========================================================"));

  // 1. Iniciar Base de datos (Directorio independiente DB_QUICK)
  if (!db.begin("DB_QUICK", SD_CS_PIN)) {
    Serial.println(F("Error al iniciar SD"));
    while (1);
  }

  // 2. Abrir o crear tabla
  logs = db.openTable<SensorLog>("sensors");

  // 3. Registrar metadatos de columnas para auto-descubrimiento en PC
  logs
    .addColumn("timestamp",   TYPE_UINT32, offsetof(SensorLog, timestamp),   sizeof(uint32_t))
    .addColumn("temperature", TYPE_FLOAT,  offsetof(SensorLog, temperature), sizeof(float))
    .addColumn("humidity",    TYPE_FLOAT,  offsetof(SensorLog, humidity),    sizeof(float))
    .addColumn("isAlert",     TYPE_BOOL,   offsetof(SensorLog, isAlert),     sizeof(bool));
  logs.saveSchema(); // Genera DB_QUICK/sensors.jsn en la SD

  // 4. Insertar un registro O(1)
  SensorLog reading = { millis(), 24.85f, 60.20f, false };
  uint32_t newId = logs.insert(reading);
  Serial.print(F("Registro guardado con ID #"));
  Serial.println(newId);

  // 5. Leer registro por ID O(1)
  SensorLog result;
  if (logs.getById(newId, result)) {
    Serial.print(F("Lectura ID #")); Serial.print(newId);
    Serial.print(F(": Temp=")); Serial.print(result.temperature);
    Serial.print(F(" C, Hum=")); Serial.print(result.humidity);
    Serial.println(F(" %"));
  }

  // 6. Consulta en Streaming (O(1) RAM)
  Serial.println(F("\nTodos los registros en disco:"));
  logs.forEach([](uint32_t id, const SensorLog& rec) {
    Serial.print(F("  #")); Serial.print(id);
    Serial.print(F(" | Temp: ")); Serial.print(rec.temperature);
    Serial.print(F(" C | Hum: ")); Serial.print(rec.humidity);
    Serial.print(F("% | Alerta: ")); Serial.println(rec.isAlert ? "SI" : "NO");
  });
}

void loop() {
}
