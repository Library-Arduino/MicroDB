/*
  =============================================================================
  MICRODB - VALORES ÚNICOS (UNIQUE), UPSERT Y COMPACTACIÓN (VACUUM)
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  Demostración de funciones esenciales de base de datos:
  1. Restricción UNIQUE (Valores no repetidos): Evita emails o documentos duplicados.
  2. Operación UPSERT (Insert or Update): Actualiza si ya existe o inserta si es nuevo.
  3. Mantenimiento VACUUM (Compactación): Desfragmenta la tabla y recupera espacio físico en SD.
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

// Esquema de cuentas de usuario
struct UserAccount {
  uint32_t documentId;   // Cédula / DNI (Debe ser único numéricamente)
  char     username[16]; // Nombre de usuario (Debe ser único en texto)
  char     email[32];    // Correo electrónico (Debe ser único en texto)
  float    balance;      // Saldo
};

// Esquema de estado de dispositivos IoT
struct DeviceTelemetry {
  char     macAddress[18]; // Dirección MAC única (ej: "AA:BB:CC:DD:EE:01")
  float    lastTemperature;
  uint32_t lastPingTime;
};

MicroDB db;
Table<UserAccount>     usersTable;
Table<DeviceTelemetry> devicesTable;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("    MICRODB - VALORES UNICOS (UNIQUE), UPSERT Y VACUUM  "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_UNQ", SD_CS_PIN)) {
    Serial.println(F("Error al iniciar SD"));
    while (1);
  }

  usersTable   = db.openTable<UserAccount>("users_u");
  devicesTable = db.openTable<DeviceTelemetry>("dev_ups");

  // Registrar metadatos de esquemas para auto-descubrimiento en PC
  usersTable
    .addColumn("documentId", TYPE_UINT32, offsetof(UserAccount, documentId), sizeof(uint32_t))
    .addColumn("username",   TYPE_STRING, offsetof(UserAccount, username),   sizeof(UserAccount::username))
    .addColumn("email",      TYPE_STRING, offsetof(UserAccount, email),      sizeof(UserAccount::email))
    .addColumn("balance",    TYPE_FLOAT,  offsetof(UserAccount, balance),    sizeof(float));
  usersTable.saveSchema();

  devicesTable
    .addColumn("macAddress", TYPE_STRING, offsetof(DeviceTelemetry, macAddress),      sizeof(DeviceTelemetry::macAddress))
    .addColumn("lastTemp",   TYPE_FLOAT,  offsetof(DeviceTelemetry, lastTemperature), sizeof(float))
    .addColumn("lastPing",   TYPE_UINT32, offsetof(DeviceTelemetry, lastPingTime),    sizeof(uint32_t));
  devicesTable.saveSchema();

  usersTable.truncate();
  devicesTable.truncate();

  // --------------------------------------------------------------------------
  // CASO 1: RESTRICCIÓN DE VALORES ÚNICOS (UNIQUE CONSTRAINT)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 1: RESTRICCION DE VALORES UNICOS (UNIQUE) ---"));

  UserAccount u1 = { 10203040, "jairo",  "jairo@example.com", 150.0f };
  UserAccount u2 = { 50607080, "maria",  "maria@example.com", 300.0f };
  
  // 1. Insertar usuarios con email único
  Serial.println(F("1. Insertando usuarios iniciales con email unico..."));
  uint32_t id1 = usersTable.insertUniqueString(u1, [](const UserAccount& u){ return u.email; });
  uint32_t id2 = usersTable.insertUniqueString(u2, [](const UserAccount& u){ return u.email; });
  Serial.print(F("--> Usuarios creados con IDs: #")); Serial.print(id1); Serial.print(F(", #")); Serial.println(id2);

  // 2. Intentar insertar un duplicado con el mismo correo 'jairo@example.com'
  Serial.println(F("\n2. Intentando insertar un nuevo usuario con correo duplicado 'jairo@example.com'..."));
  UserAccount uDuplicado = { 99999999, "jairo_clon", "jairo@example.com", 50.0f };
  
  uint32_t idDuplicado = usersTable.insertUniqueString(uDuplicado, [](const UserAccount& u){ return u.email; });
  if (idDuplicado == 0) {
    Serial.println(F("--> [RESULTADO]: Insercion rechazada con exito! La regla UNIQUE protegio la BD."));
  }

  // 3. Intentar insertar un duplicado con el mismo documentId (DNI)
  Serial.println(F("\n3. Intentando insertar un usuario con DocumentId duplicado (10203040)..."));
  UserAccount uDniDuplicado = { 10203040, "otro_user", "otro@example.com", 80.0f };
  uint32_t idDniDuplicado = usersTable.insertUnique(uDniDuplicado, [](const UserAccount& u){ return u.documentId; });
  if (idDniDuplicado == 0) {
    Serial.println(F("--> [RESULTADO]: Insercion rechazada por duplicidad en DocumentId numerico!"));
  }

  // --------------------------------------------------------------------------
  // CASO 2: OPERACIÓN UPSERT (INSERT ON DUPLICATE KEY UPDATE)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 2: OPERACION UPSERT (ACTUALIZAR SI EXISTE, INSERTAR SI NO) ---"));

  // Dispositivo 1 reporta por primera vez -> Se debe INSERTAR
  Serial.println(F("1. Dispositivo 'AA:BB:CC:DD:EE:01' reporta por primera vez (Temp=24.5 C)..."));
  DeviceTelemetry d1 = { "AA:BB:CC:DD:EE:01", 24.5f, 10001 };
  uint32_t devId1 = devicesTable.upsertUniqueString(d1, [](const DeviceTelemetry& d){ return d.macAddress; });
  Serial.print(F("--> Dispositivo nuevo registrado con ID #")); Serial.println(devId1);

  // Dispositivo 1 reporta de nuevo -> Se debe ACTUALIZAR in-place en vez de duplicarse
  Serial.println(F("\n2. El mismo dispositivo 'AA:BB:CC:DD:EE:01' envia nueva lectura (Temp=29.8 C)..."));
  DeviceTelemetry d1_update = { "AA:BB:CC:DD:EE:01", 29.8f, 10002 };
  uint32_t devId1_upsert = devicesTable.upsertUniqueString(d1_update, [](const DeviceTelemetry& d){ return d.macAddress; });
  Serial.print(F("--> Registro actualizado (mismo ID #")); Serial.print(devId1_upsert); Serial.println(F(")"));

  Serial.print(F("--> Total dispositivos en tabla (debe ser 1 solo sin duplicados): "));
  Serial.println(devicesTable.count());

  // --------------------------------------------------------------------------
  // CASO 3: COMPACTACIÓN Y DESFRAGMENTACIÓN DE DISCO (VACUUM)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 3: DESFRAGMENTACION Y COMPACTACION (VACUUM) ---"));

  // Insertar y borrar registros para crear fragmentación (tombstones)
  UserAccount tempU1 = { 111, "temp1", "t1@ex.com", 0 };
  UserAccount tempU2 = { 222, "temp2", "t2@ex.com", 0 };
  UserAccount tempU3 = { 333, "temp3", "t3@ex.com", 0 };

  usersTable.insert(tempU1);
  usersTable.insert(tempU2);
  uint32_t idBorrar = usersTable.insert(tempU3);

  usersTable.remove(idBorrar);
  Serial.print(F("Slots borrados acumulados antes de VACUUM: "));
  Serial.println(usersTable.deletedCount());

  Serial.println(F("Ejecutando vacuum() para compactar archivo binario en la SD..."));
  usersTable.vacuum();

  Serial.print(F("--> Slots borrados despues de VACUUM: "));
  Serial.println(usersTable.deletedCount());
  Serial.println(F("--> [EXITO]: El archivo fisico en la SD ha sido 100% compactado y desfragmentado."));

  Serial.println(F("\n========================================================"));
  Serial.println(F("       DEMOSTRACION UNIQUE, UPSERT Y VACUUM EXITOSA     "));
  Serial.println(F("========================================================"));
}

void loop() {}
