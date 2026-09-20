/*
  =============================================================================
  MICRODB - CONSULTAS COMPLEJAS: MULTI-TABLA (3 TABLAS) Y EXPORTACIÓN A CSV
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  Demostración de capacidades avanzadas de base de datos relacional:
  1. Modelo de 3 Tablas Relacionadas: Clientes <-> Pedidos <-> Productos
  2. Búsqueda y JOIN multidimensional con filtros relacionales
  3. Exportación dinámica de resultados de una consulta a un archivo CSV en la SD
     para abrir en Microsoft Excel o analizar en la computadora
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

// ----------------------------------------------------------------------------
// ESQUEMAS RELACIONALES (3 MODELOS DE DATOS)
// ----------------------------------------------------------------------------

// 1. Tabla: Clientes
struct Client {
  char    name[20];
  char    tier[8];       // "VIP", "STANDARD", "PREMIUM"
};

// 2. Tabla: Productos (Catálogo)
struct Product {
  char    sku[10];       // Código de producto
  char    title[24];     // Nombre
  float   unitPrice;     // Precio unitario
  uint8_t categoryId;    // 1=Hardware, 2=Sensores, 3=Accesorios
};

// 3. Tabla: Pedidos (Tabla de Unión con dos Claves Foráneas: clientId y productId)
struct OrderItem {
  uint32_t clientId;     // FK -> Client.id
  uint32_t productId;    // FK -> Product.id
  uint16_t quantity;     // Cantidad comprada
  uint32_t orderDate;    // Timestamp
};

MicroDB db;
Table<Client>    clientsTable;
Table<Product>   productsTable;
Table<OrderItem> ordersTable;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("    MICRODB - RELACIONES MULTI-TABLA Y EXPORTACION CSV   "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_MULTI", SD_CS_PIN)) {
    Serial.println(F("Error al inicializar la SD"));
    while (1);
  }

  clientsTable  = db.openTable<Client>("clients");
  productsTable = db.openTable<Product>("products");
  ordersTable   = db.openTable<OrderItem>("orders");

  // Registrar metadatos de las 3 tablas para auto-descubrimiento
  clientsTable
    .addColumn("name", TYPE_STRING, offsetof(Client, name), sizeof(Client::name))
    .addColumn("tier", TYPE_STRING, offsetof(Client, tier), sizeof(Client::tier));
  clientsTable.saveSchema();

  productsTable
    .addColumn("sku",        TYPE_STRING, offsetof(Product, sku),        sizeof(Product::sku))
    .addColumn("title",      TYPE_STRING, offsetof(Product, title),      sizeof(Product::title))
    .addColumn("unitPrice",  TYPE_FLOAT,  offsetof(Product, unitPrice),  sizeof(float))
    .addColumn("categoryId", TYPE_UINT8,  offsetof(Product, categoryId), sizeof(uint8_t));
  productsTable.saveSchema();

  ordersTable
    .addForeignKey("clientId",  TYPE_UINT32, offsetof(OrderItem, clientId),  sizeof(uint32_t), "clients",  "id")
    .addForeignKey("productId", TYPE_UINT32, offsetof(OrderItem, productId), sizeof(uint32_t), "products", "id")
    .addColumn("quantity",      TYPE_UINT16, offsetof(OrderItem, quantity),  sizeof(uint16_t))
    .addColumn("orderDate",     TYPE_UINT32, offsetof(OrderItem, orderDate), sizeof(uint32_t));
  ordersTable.saveSchema();

  // Limpiar para prueba
  clientsTable.truncate();
  productsTable.truncate();
  ordersTable.truncate();

  // --------------------------------------------------------------------------
  // 1. POBLAR TABLAS CON DATOS RELACIONADOS
  // --------------------------------------------------------------------------
  Serial.println(F("[1] Creando Catalogo, Clientes y Pedidos..."));

  // Clientes
  Client c1 = { "Jairo Rohatan", "VIP" };
  Client c2 = { "Maria Gomez",   "STANDARD" };
  Client c3 = { "Tech Corp",     "VIP" };
  uint32_t idClient1 = clientsTable.insert(c1);
  uint32_t idClient2 = clientsTable.insert(c2);
  uint32_t idClient3 = clientsTable.insert(c3);

  // Productos
  Product p1 = { "ARD-001", "Arduino Mega 2560", 38.50f, 1 };
  Product p2 = { "SNS-002", "Sensor BMP280",      4.20f, 2 };
  Product p3 = { "DIS-003", "Display LCD 20x4",  12.00f, 3 };
  Product p4 = { "WFI-004", "Modulo ESP32-WROOM", 8.90f, 1 };
  uint32_t idP1 = productsTable.insert(p1);
  uint32_t idP2 = productsTable.insert(p2);
  uint32_t idP3 = productsTable.insert(p3);
  uint32_t idP4 = productsTable.insert(p4);

  // Pedidos
  OrderItem ord1 = { idClient1, idP1, 2, 1774180100 };
  OrderItem ord2 = { idClient1, idP2, 5, 1774180200 };
  OrderItem ord3 = { idClient2, idP3, 1, 1774180300 };
  OrderItem ord4 = { idClient3, idP4, 10, 1774180400 };
  OrderItem ord5 = { idClient3, idP1, 4, 1774180500 };

  ordersTable.insert(ord1);
  ordersTable.insert(ord2);
  ordersTable.insert(ord3);
  ordersTable.insert(ord4);
  ordersTable.insert(ord5);

  Serial.println(F("--> Base de datos relacional poblada con exito."));

  // --------------------------------------------------------------------------
  // 2. CONSULTA RELACIONAL MULTI-TABLA (ORDERS -> CLIENTS + PRODUCTS)
  // --------------------------------------------------------------------------
  Serial.println(F("\n[2] Reporte Detallado de Ventas (INNER JOIN 3 Tablas):"));

  float granTotalVentas = 0.0f;

  ordersTable.forEach([&](uint32_t orderId, const OrderItem& ord) {
    Client  c;
    Product p;

    // Buscar en O(1) los datos del cliente y del producto por sus FKs
    if (clientsTable.getById(ord.clientId, c) && productsTable.getById(ord.productId, p)) {
      float subtotal = ord.quantity * p.unitPrice;
      granTotalVentas += subtotal;

      Serial.print(F("  * Factura #")); Serial.print(orderId);
      Serial.print(F(" | Cliente: ")); Serial.print(c.name);
      Serial.print(F(" [")); Serial.print(c.tier); Serial.print(F("]"));
      Serial.print(F(" -> Producto: ")); Serial.print(p.title);
      Serial.print(F(" (x")); Serial.print(ord.quantity);
      Serial.print(F(" @ $")); Serial.print(p.unitPrice);
      Serial.print(F(") = Subtotal: $")); Serial.println(subtotal);
    }
  });

  Serial.print(F("--> FACTURACION TOTAL ACUMULADA: $"));
  Serial.println(granTotalVentas);

  // --------------------------------------------------------------------------
  // 3. EXPORTACIÓN DINÁMICA DE LA CONSULTA A UN ARCHIVO CSV EN LA SD
  // --------------------------------------------------------------------------
  Serial.println(F("\n[3] Exportando consulta relacional a archivo 'DB_MULTI/REPORT.CSV' en SD..."));

  const char* csvPath = "DB_MULTI/REPORT.CSV";
  if (SD.exists(csvPath)) {
    SD.remove(csvPath);
  }

  File csvFile = SD.open(csvPath, FILE_WRITE);
  if (csvFile) {
    csvFile.println(F("OrderId,Cliente,Nivel,SKU,Producto,Cantidad,PrecioUnitario,Subtotal"));

    ordersTable.forEach([&](uint32_t orderId, const OrderItem& ord) {
      Client  c;
      Product p;
      if (clientsTable.getById(ord.clientId, c) && productsTable.getById(ord.productId, p)) {
        float subtotal = ord.quantity * p.unitPrice;

        csvFile.print(orderId); csvFile.print(F(","));
        csvFile.print(c.name);  csvFile.print(F(","));
        csvFile.print(c.tier);  csvFile.print(F(","));
        csvFile.print(p.sku);   csvFile.print(F(","));
        csvFile.print(p.title); csvFile.print(F(","));
        csvFile.print(ord.quantity); csvFile.print(F(","));
        csvFile.print(p.unitPrice); csvFile.print(F(","));
        csvFile.println(subtotal);
      }
    });

    csvFile.flush();
    csvFile.close();
    Serial.println(F("--> Archivo 'DB_MULTI/REPORT.CSV' generado exitosamente en la SD!"));
    Serial.println(F("    Puedes insertar la tarjeta en tu PC y abrirlo con Microsoft Excel."));
  } else {
    Serial.println(F("Error al crear REPORT.CSV"));
  }

  Serial.println(F("\n========================================================"));
  Serial.println(F("         CONSULTAS MULTI-TABLA COMPLETADAS              "));
  Serial.println(F("========================================================"));
}

void loop() {}
