/*
  =============================================================================
  MICRODB - RELACIONES DIRECTAS: INNER JOIN
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
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

// Tabla Padre: Clientes
struct Customer {
  char name[20];
  char phone[12];
};

// Tabla Hija: Facturas (Foreign Key customer_id -> Customer.id)
struct Invoice {
  uint32_t customer_id;
  float    amount;
  bool     isPaid;
};

MicroDB db;
Table<Customer> customers;
Table<Invoice> invoices;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("       MICRODB - CONSULTAS RELACIONALES (INNER JOIN)    "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_JOIN", SD_CS_PIN)) {
    Serial.println(F("Error al inicializar la tarjeta SD"));
    while (1);
  }

  customers = db.openTable<Customer>("custs");
  invoices  = db.openTable<Invoice>("invs");

  // Registrar metadatos de esquemas para auto-descubrimiento
  customers
    .addColumn("name",  TYPE_STRING, offsetof(Customer, name),  sizeof(Customer::name))
    .addColumn("phone", TYPE_STRING, offsetof(Customer, phone), sizeof(Customer::phone));
  customers.saveSchema();

  invoices
    .addForeignKey("customer_id", TYPE_UINT32, offsetof(Invoice, customer_id), sizeof(uint32_t), "custs", "id")
    .addColumn("amount",          TYPE_FLOAT,  offsetof(Invoice, amount),      sizeof(float))
    .addColumn("isPaid",          TYPE_BOOL,   offsetof(Invoice, isPaid),      sizeof(bool));
  invoices.saveSchema();

  // Si está vacía, sembrar datos
  if (customers.count() == 0) {
    Customer c1 = { "Empresa ABC", "555-0100" };
    Customer c2 = { "Tech Labs",   "555-0200" };
    uint32_t cid1 = customers.insert(c1);
    uint32_t cid2 = customers.insert(c2);

    Invoice i1 = { cid1, 1500.0f, true };
    Invoice i2 = { cid1,  320.5f, false };
    Invoice i3 = { cid2, 4890.0f, true };
    invoices.insert(i1);
    invoices.insert(i2);
    invoices.insert(i3);
  }

  // Consulta Relacional INNER JOIN
  Serial.println(F("\n--- Listado Relacional (Invoices INNER JOIN Customers) ---"));
  db.innerJoin(
    customers,
    invoices,
    [](const Invoice& inv) { return inv.customer_id; },
    [](const Customer& cust, uint32_t invId, const Invoice& inv) {
      Serial.print(F("  Factura #")); Serial.print(invId);
      Serial.print(F(" [Monto: $")); Serial.print(inv.amount);
      Serial.print(F(" | Pagado: ")); Serial.print(inv.isPaid ? "SI" : "NO");
      Serial.print(F("] -> Cliente: ")); Serial.print(cust.name);
      Serial.print(F(" (Tel: ")); Serial.print(cust.phone); Serial.println(F(")"));
    }
  );
}

void loop() {
}
