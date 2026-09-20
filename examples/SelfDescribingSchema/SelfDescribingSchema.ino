/*
  =============================================================================
  MICRODB - METADATOS Y AUTO-DESCUBRIMIENTO DE COLUMNAS (SCHEMA REFLECTION)
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  En este ejemplo aprenderás cómo hacer que la base de datos sea AUTO-DESCRIPTIVA:
  1. Define las columnas de tu struct con nombres, tipos y desplazamientos.
  2. Guarda el catálogo de metadatos en la SD (.jsn y .sch).
  3. Cualquier software externo (escrito en Python, C#, Electron, Node.js, etc.)
     podrá abrir la SD, leer el archivo JSN/SCH y renderizar una tabla completa
     con encabezados y datos automáticamente, sin tener que adivinar las columnas.
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

// Definición de la estructura de datos
struct Employee {
  char     fullName[24];
  char     department[16];
  uint8_t  age;
  float    salary;
  uint32_t joinTimestamp;
};

MicroDB db;
Table<Employee> employeesTable;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F(" MICRODB - METADATOS Y AUTO-DESCUBRIMIENTO DE COLUMNAS  "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_SCHEM", SD_CS_PIN)) {
    Serial.println(F("Error al inicializar SD"));
    while (1);
  }

  employeesTable = db.openTable<Employee>("employee");

  // --------------------------------------------------------------------------
  // 1. DEFINICIÓN DEL ESQUEMA DE COLUMNAS (METADATOS)
  // --------------------------------------------------------------------------
  Serial.println(F("[1] Registrando definicion de columnas en el catalogo..."));

  employeesTable
    .addColumn("fullName",      TYPE_STRING, offsetof(Employee, fullName),      sizeof(Employee::fullName))
    .addColumn("department",    TYPE_STRING, offsetof(Employee, department),    sizeof(Employee::department))
    .addColumn("age",           TYPE_UINT8,  offsetof(Employee, age),           sizeof(uint8_t))
    .addColumn("salary",        TYPE_FLOAT,  offsetof(Employee, salary),        sizeof(float))
    .addColumn("joinTimestamp", TYPE_UINT32, offsetof(Employee, joinTimestamp), sizeof(uint32_t));

  // 2. Guardar el catálogo de metadatos en la SD (Genera DB_SCHEM/employees.jsn y DB_SCHEM/employees.sch)
  employeesTable.saveSchema();
  Serial.println(F("--> Archivo de Metadatos 'DB_SCHEM/employees.jsn' y 'DB_SCHEM/employees.sch' guardados en SD!"));

  // 3. Imprimir el esquema auto-descubierto en el puerto Serial
  Serial.println(F("\n[2] Esquema de la tabla descubierto:"));
  employeesTable.printSchema();

  // --------------------------------------------------------------------------
  // 4. INSERTAR REGISTROS DE PRUEBA
  // --------------------------------------------------------------------------
  Serial.println(F("\n[3] Insertando registros en la tabla binaria..."));
  employeesTable.truncate();

  Employee e1 = { "Jairo Rohatan", "Desarrollo", 28, 3500.00f, 1774180000 };
  Employee e2 = { "Maria Lopez",   "Sistemas",   32, 4200.50f, 1774180100 };
  Employee e3 = { "Carlos Ruiz",   "Hardware",   24, 2800.00f, 1774180200 };

  employeesTable.insert(e1);
  employeesTable.insert(e2);
  employeesTable.insert(e3);

  Serial.print(F("--> Total empleados activos en SD: "));
  Serial.println(employeesTable.count());

  // --------------------------------------------------------------------------
  // 5. CÓMO EL SOFTWARE EXTERNO LEE LOS DATOS AUTOMÁTICAMENTE
  // --------------------------------------------------------------------------
  Serial.println(F("\n========================================================"));
  Serial.println(F("   COMO FUNCIONA CON TU SOFTWARE EXTERNO (PYTHON / C#)   "));
  Serial.println(F("========================================================"));
  Serial.println(F("1. Tu software en PC abre la tarjeta SD y lee 'DB_SCHEM/employee.jsn'."));
  Serial.println(F("2. 'employee.jsn' le dice exactamente:"));
  Serial.println(F("   - Que 'fullName' es STRING de 24 bytes en offset 0"));
  Serial.println(F("   - Que 'salary' es FLOAT de 4 bytes en offset 41, etc."));
  Serial.println(F("3. Tu software lee 'DB_SCHEM/employee.tbl' en bloques y extrae cada"));
  Serial.println(F("   columna sin necesidad de hardcodear estructuras en tu programa!"));
  Serial.println(F("========================================================"));
}

void loop() {}
