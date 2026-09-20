/*
  =============================================================================
  MICRODB - EJEMPLO MAESTRO DE CRUD COMPLETO (CREATE, READ, UPDATE, DELETE)
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  Este sketch demuestra paso a paso las 4 operaciones fundamentales (CRUD):
  1. [C] CREATE (Inserción): Cómo registrar datos con IDs autoincrementales.
  2. [R] READ   (Lectura):   Búsqueda directa O(1) por ID y consultas con filtros (WHERE).
  3. [U] UPDATE (Edición):   Modificación in-place directa en la SD sin reescribir la tabla.
  4. [D] DELETE (Borrado):   Eliminación O(1) y reciclaje de espacio libre (Free-List).
  =============================================================================
*/

#include <SPI.h>
#include <SD.h>
#include <MicroDB.h>

// Configuración de Pin CS según el microcontrolador
#if defined(ESP32)
  const uint8_t SD_CS_PIN = 5;      // ESP32
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  const uint8_t SD_CS_PIN = 53;     // Arduino Mega
#else
  const uint8_t SD_CS_PIN = 4;      // Arduino Uno / Nano / Otros
#endif

// ============================================================================
// DEFINICIÓN DEL ESQUEMA DE DATOS (TABLA DE PRODUCTOS DE INVENTARIO)
// ============================================================================
struct ProductItem {
  char     name[24];     // Nombre del producto
  float    price;        // Precio unitario
  uint16_t stock;        // Cantidad disponible en inventario
  bool     isAvailable;  // Estado (Disponible / Agotado)
};

MicroDB db;
Table<ProductItem> inventory;

void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  Serial.println(F("\n========================================================"));
  Serial.println(F("    MICRODB - GUIA DE OPERACIONES CRUD COMPLETAS        "));
  Serial.println(F("========================================================"));

  // 1. Inicializar el motor MicroDB en la SD (carpeta "DB_CRUD")
  Serial.print(F("[1] Inicializando MicroDB en tarjeta SD... "));
  if (!db.begin("DB_CRUD", SD_CS_PIN)) {
    Serial.println(F("ERROR al conectar con la tarjeta SD."));
    while (1) { delay(1000); }
  }
  Serial.println(F("OK!"));

  // 2. Abrir o crear la tabla binaria en disco
  inventory = db.openTable<ProductItem>("inv_crud");

  // 3. Registrar metadatos de columnas para auto-descubrimiento en software de PC
  inventory
    .addColumn("name",        TYPE_STRING, offsetof(ProductItem, name),        sizeof(ProductItem::name))
    .addColumn("price",       TYPE_FLOAT,  offsetof(ProductItem, price),       sizeof(float))
    .addColumn("stock",       TYPE_UINT16, offsetof(ProductItem, stock),       sizeof(uint16_t))
    .addColumn("isAvailable", TYPE_BOOL,   offsetof(ProductItem, isAvailable), sizeof(bool));
  inventory.saveSchema(); // Genera DB_CRUD/inv_crud.jsn y .sch

  // Limpiar datos previos para ejecutar la demostración desde cero
  inventory.truncate();

  // ==========================================================================
  // [C] CREATE - INSERCIÓN DE NUEVOS REGISTROS
  // ==========================================================================
  Serial.println(F("\n========================================================"));
  Serial.println(F(" 1. [CREATE] - INSERTAR REGISTROS (O(1))                "));
  Serial.println(F("========================================================"));

  ProductItem p1 = { "Arduino Mega 2560", 38.50f, 15, true };
  ProductItem p2 = { "Sensor BMP280",      4.20f, 50, true };
  ProductItem p3 = { "Pantalla TFT 3.5",  28.00f,  0, false }; // Agotado
  ProductItem p4 = { "Modulo ESP32",       8.90f, 25, true };

  uint32_t id1 = inventory.insert(p1);
  uint32_t id2 = inventory.insert(p2);
  uint32_t id3 = inventory.insert(p3);
  uint32_t id4 = inventory.insert(p4);

  Serial.print(F("--> Producto 1 insertado con ID #")); Serial.println(id1);
  Serial.print(F("--> Producto 2 insertado con ID #")); Serial.println(id2);
  Serial.print(F("--> Producto 3 insertado con ID #")); Serial.println(id3);
  Serial.print(F("--> Producto 4 insertado con ID #")); Serial.println(id4);
  Serial.print(F("Total de productos activos en la base de datos: "));
  Serial.println(inventory.count());

  // ==========================================================================
  // [R] READ - LECTURA DIRECTA O(1) Y CONSULTAS CON FILTROS (WHERE)
  // ==========================================================================
  Serial.println(F("\n========================================================"));
  Serial.println(F(" 2. [READ] - LECTURA Y CONSULTAS (STREAMING O(1) RAM)   "));
  Serial.println(F("========================================================"));

  // A. Lectura Directa O(1) por ID
  Serial.println(F("A. Lectura Directa O(1) buscando el ID #2:"));
  ProductItem searchItem;
  if (inventory.getById(id2, searchItem)) {
    Serial.print(F("   [ENCONTRADO] Nombre: ")); Serial.print(searchItem.name);
    Serial.print(F(" | Precio: $")); Serial.print(searchItem.price);
    Serial.print(F(" | Stock: ")); Serial.print(searchItem.stock);
    Serial.print(F(" | Estado: ")); Serial.println(searchItem.isAvailable ? F("DISPONIBLE") : F("AGOTADO"));
  }

  // B. Consulta con Filtro (WHERE: Productos con Stock > 0 y Disponibles)
  Serial.println(F("\nB. Consulta con Filtro (WHERE: Disponibles y con Stock > 0):"));
  inventory.where(
    [](uint32_t id, const ProductItem& p) {
      return p.isAvailable && p.stock > 0;
    },
    [](uint32_t id, const ProductItem& p) {
      Serial.print(F("   * ID #")); Serial.print(id);
      Serial.print(F(" | ")); Serial.print(p.name);
      Serial.print(F(" -> Stock: ")); Serial.print(p.stock);
      Serial.print(F(" unid. ($")); Serial.print(p.price); Serial.println(F(")"));
    }
  );

  // ==========================================================================
  // [U] UPDATE - ACTUALIZACIÓN EN SITIO O(1)
  // ==========================================================================
  Serial.println(F("\n========================================================"));
  Serial.println(F(" 3. [UPDATE] - ACTUALIZACION EN SITIO O(1)               "));
  Serial.println(F("========================================================"));

  Serial.println(F("Actualizando 'Pantalla TFT 3.5' (ID #3):"));
  Serial.println(F(" -> Llego nuevo stock: Stock = 20, isAvailable = true, Precio = $26.50"));

  ProductItem itemToEdit;
  if (inventory.getById(id3, itemToEdit)) {
    // Modificar los campos en la variable local
    itemToEdit.stock = 20;
    itemToEdit.isAvailable = true;
    itemToEdit.price = 26.50f;

    // Guardar los cambios directamente en la SD sin reescribir el resto del archivo
    inventory.update(id3, itemToEdit);
  }

  // Leer nuevamente para verificar que la actualización fue persistida en la SD
  ProductItem updatedVerify;
  if (inventory.getById(id3, updatedVerify)) {
    Serial.print(F("--> [VERIFICADO TRAS UPDATE] ID #3: ")); Serial.print(updatedVerify.name);
    Serial.print(F(" | Nuevo Stock: ")); Serial.print(updatedVerify.stock);
    Serial.print(F(" | Nuevo Precio: $")); Serial.print(updatedVerify.price);
    Serial.print(F(" | Estado: ")); Serial.println(updatedVerify.isAvailable ? F("DISPONIBLE") : F("AGOTADO"));
  }

  // ==========================================================================
  // [D] DELETE - BORRADO O(1) Y RECICLAJE AUTOMÁTICO DE ESPACIO (FREE-LIST)
  // ==========================================================================
  Serial.println(F("\n========================================================"));
  Serial.println(F(" 4. [DELETE] - BORRADO Y RECICLAJE DE ESPACIO (FREE-LIST)"));
  Serial.println(F("========================================================"));

  Serial.print(F("Eliminando el producto 'Sensor BMP280' con ID #"));
  Serial.println(id2);

  // Borrado lógico O(1) con marcado Tombstone y encolado en Free-List
  inventory.remove(id2);

  Serial.print(F("--> Productos activos restantes: ")); Serial.println(inventory.count());
  Serial.print(F("--> Slots borrados reciclables en SD: ")); Serial.println(inventory.deletedCount());

  // Intentar leer el registro borrado para confirmar que ya no es accesible
  ProductItem deletedCheck;
  if (!inventory.getById(id2, deletedCheck)) {
    Serial.println(F("--> Confirmado: El ID #2 ya no es accesible (marcado como borrado)."));
  }

  // Demostración del Reciclaje de Espacio:
  // Al insertar un nuevo registro, el motor NO hace crecer el archivo al final,
  // sino que reutiliza el slot vacío del registro recién borrado.
  Serial.println(F("\nInsertando nuevo producto 'Sensor DHT22'..."));
  ProductItem p5 = { "Sensor DHT22", 5.50f, 40, true };
  uint32_t id5 = inventory.insert(p5);

  Serial.print(F("--> Nuevo producto insertado con ID #")); Serial.println(id5);
  Serial.print(F("--> Slots borrados despues de reutilizar espacio: ")); 
  Serial.println(inventory.deletedCount());

  // ==========================================================================
  // ESTADO FINAL DE LA TABLA TRAS EL CICLO CRUD COMPLETO
  // ==========================================================================
  Serial.println(F("\n========================================================"));
  Serial.println(F("          LISTADO FINAL DE LA BASE DE DATOS             "));
  Serial.println(F("========================================================"));

  inventory.forEach([](uint32_t id, const ProductItem& p) {
    Serial.print(F("  [REGISTRO] ID #")); Serial.print(id);
    Serial.print(F(": ")); Serial.print(p.name);
    Serial.print(F(" | Stock: ")); Serial.print(p.stock);
    Serial.print(F(" | Precio: $")); Serial.print(p.price);
    Serial.print(F(" | Estado: ")); Serial.println(p.isAvailable ? F("DISPONIBLE") : F("AGOTADO"));
  });

  Serial.println(F("\n========================================================"));
  Serial.println(F("      DEMOSTRACION CRUD COMPLETADA CON EXITO            "));
  Serial.println(F("========================================================"));
}

void loop() {
  // Nada que repetir
}
