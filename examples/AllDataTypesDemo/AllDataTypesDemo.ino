/*
  =============================================================================
  MICRODB - GUÍA Y DEMOSTRACIÓN DE TODOS LOS TIPOS DE DATOS SOPORTADOS
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  En este ejemplo se demuestra cómo almacenar, leer, actualizar y filtrar
  CUALQUIER tipo de dato en MicroDB sobre la tarjeta SD:
  
  1. Booleanos (bool) -> Banderas de estado, alertas, switches.
  2. Enteros (int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t).
  3. Números con decimales (float y double de alta precisión para GPS).
  4. Cadenas de texto fijas (char[] para nombres, identificadores, emails).
  5. Blobs / Arreglos binarios crudos (uint8_t[] para RFID, claves de cifrado).
  6. Documentos JSON completos (almacenados en buffer de texto fijo).
  7. Referencias a Archivos Multimedia en SD (Imágenes, Audio, Logs).
  =============================================================================
*/

#include <SPI.h>
#include <SD.h>
#include <MicroDB.h>

// ----------------------------------------------------------------------------
// CONFIGURACIÓN DE PINES SEGÚN EL MICROCONTROLADOR
// ----------------------------------------------------------------------------
#if defined(ESP32)
  const uint8_t SD_CS_PIN = 5;      // ESP32
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  const uint8_t SD_CS_PIN = 53;     // Arduino Mega
#else
  const uint8_t SD_CS_PIN = 4;      // Arduino Uno / Nano / Otros
#endif

// ============================================================================
// ESTRUCTURA CON TODOS LOS TIPOS DE DATOS INTEGRADOS
// ============================================================================
struct MasterRecord {
  // 1. Booleanos (1 byte cada uno)
  bool     isActive;            // true = Sistema activo, false = Inactivo
  bool     isAlarmTriggered;    // true = Alarma disparada

  // 2. Enteros con y sin signo de diferentes tamaños
  int8_t   tempCalibration;     // Entero con signo (-128 a 127)
  uint8_t  batteryPercent;      // Entero sin signo (0 a 255 %)
  int16_t  altitudeMeters;      // Altura en metros (-32768 a 32767)
  uint16_t rawAdcValue;         // Lectura analógica ADC (0 a 65535)
  int32_t  systemErrorCode;     // Código de error
  uint32_t sampleCounter;       // Contador de muestras
  uint32_t epochTimestamp;      // Marca de tiempo Unix (segundos desde 1970)

  // 3. Punto flotante (Decimales)
  float    sensorVoltage;       // Voltaje (precisión estándar ~6 decimales)
  double   gpsLatitude;         // Latitud GPS de precisión submétrica
  double   gpsLongitude;        // Longitud GPS de precisión submétrica

  // 4. Cadenas de Texto Fijas (C-Strings)
  char     nodeName[16];        // Nombre del nodo/estación (ej: "NODO_NORTE_01")
  char     hardwareVersion[8];  // Versión de hardware (ej: "v2.1.0")

  // 5. Datos Binarios Crudos / BLOBs (Arreglos de Bytes)
  uint8_t  rfidCardUid[7];      // UID de tarjeta RFID Mifare (7 bytes binarios)
  uint8_t  aesEncryptionKey[16];// Clave criptográfica de 128 bits (16 bytes crudos)

  // 6. JSON Completo (Almacenado como texto en buffer fijo)
  char     jsonConfig[64];      // ej: "{\"fan\":true,\"targetTemp\":24.5,\"mode\":\"ECO\"}"

  // 7. Puntero a Archivo Externo en la SD
  char     mediaFilePath[24];   // Ruta al archivo multimedia en la SD (ej: "CAM/FOTO_01.JPG")
};

// Instancias globales
MicroDB db;
Table<MasterRecord> masterTable;

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  Serial.println(F("\n========================================================"));
  Serial.println(F("    MICRODB - DEMOSTRACION DE TODOS LOS TIPOS DE DATOS   "));
  Serial.println(F("========================================================"));

  // 1. Inicializar la Base de Datos en la carpeta "DB_TYPES"
  Serial.print(F("[1] Inicializando MicroDB en tarjeta SD... "));
  if (!db.begin("DB_TYPES", SD_CS_PIN)) {
    Serial.println(F("ERROR al conectar con la tarjeta SD."));
    while (1) { delay(1000); }
  }
  Serial.println(F("OK!"));

  // 2. Abrir la tabla "alldata"
  masterTable = db.openTable<MasterRecord>("alldata");

  // 3. Registrar metadatos completos de todas las columnas para software de PC
  masterTable
    .addColumn("isActive",         TYPE_BOOL,   offsetof(MasterRecord, isActive),         sizeof(bool))
    .addColumn("isAlarm",          TYPE_BOOL,   offsetof(MasterRecord, isAlarmTriggered), sizeof(bool))
    .addColumn("battery",          TYPE_UINT8,  offsetof(MasterRecord, batteryPercent),   sizeof(uint8_t))
    .addColumn("altitude",         TYPE_INT16,  offsetof(MasterRecord, altitudeMeters),   sizeof(int16_t))
    .addColumn("adcRaw",           TYPE_UINT16, offsetof(MasterRecord, rawAdcValue),      sizeof(uint16_t))
    .addColumn("samples",          TYPE_UINT32, offsetof(MasterRecord, sampleCounter),    sizeof(uint32_t))
    .addColumn("epoch",            TYPE_UINT32, offsetof(MasterRecord, epochTimestamp),   sizeof(uint32_t))
    .addColumn("voltage",          TYPE_FLOAT,  offsetof(MasterRecord, sensorVoltage),    sizeof(float))
    .addColumn("latitude",         TYPE_DOUBLE, offsetof(MasterRecord, gpsLatitude),     sizeof(double))
    .addColumn("longitude",        TYPE_DOUBLE, offsetof(MasterRecord, gpsLongitude),    sizeof(double))
    .addColumn("nodeName",         TYPE_STRING, offsetof(MasterRecord, nodeName),         sizeof(MasterRecord::nodeName))
    .addColumn("jsonConfig",       TYPE_STRING, offsetof(MasterRecord, jsonConfig),       sizeof(MasterRecord::jsonConfig))
    .addColumn("mediaPath",        TYPE_STRING, offsetof(MasterRecord, mediaFilePath),    sizeof(MasterRecord::mediaFilePath));
  masterTable.saveSchema(); // Genera DB_TYPES/alldata.jsn en la SD

  // Limpiar para ejecutar la demostración limpia
  masterTable.truncate();

  // --------------------------------------------------------------------------
  // CASO 1: CREACIÓN E INSERCIÓN DE UN REGISTRO CON TODOS LOS TIPOS DE DATOS
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 1: INSERCION DE TIPOS COMPLEJOS ---"));

  MasterRecord r1;
  // Booleanos
  r1.isActive = true;
  r1.isAlarmTriggered = false;

  // Enteros
  r1.tempCalibration = -3;
  r1.batteryPercent = 95;
  r1.altitudeMeters = 2600;
  r1.rawAdcValue = 1023;
  r1.systemErrorCode = 0;
  r1.sampleCounter = 5001;
  r1.epochTimestamp = 1774182000;

  // Decimales (Floats y Doubles)
  r1.sensorVoltage = 3.31f;
  r1.gpsLatitude = 4.710989;     // Coordenada exacta
  r1.gpsLongitude = -74.072092;

  // Cadenas de texto
  strncpy(r1.nodeName, "ESTACION_BOGOTA", sizeof(r1.nodeName) - 1);
  r1.nodeName[sizeof(r1.nodeName) - 1] = '\0';
  
  strncpy(r1.hardwareVersion, "v1.4", sizeof(r1.hardwareVersion) - 1);
  r1.hardwareVersion[sizeof(r1.hardwareVersion) - 1] = '\0';

  // BLOB binario (UID RFID de 7 bytes)
  uint8_t uidMifare[7] = { 0x04, 0x8A, 0x5C, 0x12, 0x7E, 0x33, 0x90 };
  memcpy(r1.rfidCardUid, uidMifare, 7);

  // BLOB binario (Clave AES de 16 bytes)
  uint8_t key128[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
  memcpy(r1.aesEncryptionKey, key128, 16);

  // Documento JSON completo
  strncpy(r1.jsonConfig, "{\"mode\":\"AUTO\",\"relay1\":true,\"target\":22.5}", sizeof(r1.jsonConfig) - 1);
  r1.jsonConfig[sizeof(r1.jsonConfig) - 1] = '\0';

  // Puntero / Ruta a foto en la SD
  strncpy(r1.mediaFilePath, "DCIM/FOTO_001.JPG", sizeof(r1.mediaFilePath) - 1);
  r1.mediaFilePath[sizeof(r1.mediaFilePath) - 1] = '\0';

  // Guardar en disco binario O(1)
  uint32_t id1 = masterTable.insert(r1);
  Serial.print(F("Registro maestro insertado exitosamente con ID #"));
  Serial.println(id1);

  // --------------------------------------------------------------------------
  // CASO 2: LECTURA DIRECTA O(1) Y DECODIFICACIÓN DE CADA TIPO
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 2: LECTURA DIRECTA Y VERIFICACION DE TIPOS ---"));
  MasterRecord loaded;
  if (masterTable.getById(id1, loaded)) {
    Serial.println(F("[DATOS LEIDOS DE LA SD]:"));
    Serial.print(F(" * Booleanos:            isActive=")); 
    Serial.print(loaded.isActive ? F("TRUE") : F("FALSE"));
    Serial.print(F(", isAlarm=")); 
    Serial.println(loaded.isAlarmTriggered ? F("TRUE") : F("FALSE"));

    Serial.print(F(" * Enteros:              Bateria=")); 
    Serial.print(loaded.batteryPercent); Serial.print(F("%, ADC="));
    Serial.print(loaded.rawAdcValue); Serial.print(F(", Muestra="));
    Serial.println(loaded.sampleCounter);

    Serial.print(F(" * Floats & Doubles:     Voltaje=")); 
    Serial.print(loaded.sensorVoltage, 2); Serial.print(F(" V, GPS=("));
    Serial.print(loaded.gpsLatitude, 6); Serial.print(F(", "));
    Serial.print(loaded.gpsLongitude, 6); Serial.println(F(")"));

    Serial.print(F(" * Texto fijo (char[]):  Dispositivo='")); 
    Serial.print(loaded.nodeName); Serial.print(F("', Version='"));
    Serial.print(loaded.hardwareVersion); Serial.println(F("'"));

    Serial.print(F(" * BLOB Binario (RFID):  0x"));
    for (int i = 0; i < 7; i++) {
      if (loaded.rfidCardUid[i] < 0x10) Serial.print(F("0"));
      Serial.print(loaded.rfidCardUid[i], HEX);
      if (i < 6) Serial.print(F(":"));
    }
    Serial.println();

    Serial.print(F(" * Documento JSON:       ")); 
    Serial.println(loaded.jsonConfig);

    Serial.print(F(" * Puntero a Archivo:    ")); 
    Serial.println(loaded.mediaFilePath);
  }

  // --------------------------------------------------------------------------
  // CASO 3: ACTUALIZACIÓN SELECTIVA EN SITIO O(1)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 3: ACTUALIZACION EN SITIO O(1) ---"));
  Serial.println(F("Disparando alarma, bajando bateria al 80% y actualizando JSON..."));
  loaded.isAlarmTriggered = true;
  loaded.batteryPercent = 80;
  strncpy(loaded.jsonConfig, "{\"mode\":\"ALARM\",\"relay1\":false,\"target\":30.0}", sizeof(loaded.jsonConfig) - 1);
  masterTable.update(id1, loaded);

  // --------------------------------------------------------------------------
  // CASO 4: CONSULTAS CON FILTROS COMBINANDO MULTIPLES TIPOS (STREAMING)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 4: FILTRO STREAMING POR MULTIPLES TIPOS (O(1) RAM) ---"));
  Serial.println(F("Buscando nodos con Alarma=TRUE, Bateria >= 70% y Voltaje > 3.0V:"));

  masterTable.where(
    [](uint32_t id, const MasterRecord& r) {
      return (r.isAlarmTriggered == true) && 
             (r.batteryPercent >= 70) && 
             (r.sensorVoltage > 3.0f);
    },
    [](uint32_t id, const MasterRecord& r) {
      Serial.print(F("  [MATCH] ID #")); Serial.print(id);
      Serial.print(F(": Nodo '")); Serial.print(r.nodeName);
      Serial.print(F("' | Alarma: ")); Serial.print(r.isAlarmTriggered ? F("ACTIVA") : F("OFF"));
      Serial.print(F(" | Bateria: ")); Serial.print(r.batteryPercent);
      Serial.print(F("% | Voltaje: ")); Serial.print(r.sensorVoltage); Serial.println(F("V"));
    }
  );

  Serial.println(F("\n========================================================"));
  Serial.println(F("   TODOS LOS TIPOS DE DATOS VERIFICADOS EXITOSAMENTE    "));
  Serial.println(F("========================================================"));
}

void loop() {
}
