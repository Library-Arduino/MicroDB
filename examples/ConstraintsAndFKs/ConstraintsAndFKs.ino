/*
  =============================================================================
  MICRODB - RESTRICCIONES (CHECK) E INTEGRIDAD REFERENCIAL (FOREIGN KEYS)
  =============================================================================
  Autor: Jairo Antonio Rohatan Zapata (https://github.com/Jairo2020)
  Repositorio: https://github.com/Library-Arduino/MicroDB.git
  
  En este ejemplo aprenderás cómo proteger la integridad de tus datos:
  1. Validación de Clave Foránea (FK): Impide registrar pedidos para clientes inexistentes.
  2. Restricciones de Validación (CHECK): Valida emails, longitudes de texto o rangos de edad.
  3. Metadatos de Relaciones y Unicidad exportados a JSON (.jsn) para software en PC.
  4. Borrado con Restricción (ON DELETE RESTRICT): Evita borrar un padre con hijos huérfanos.
  5. Borrado en Cascada (ON DELETE CASCADE): Elimina automáticamente los hijos dependientes.
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

// Tabla Padre: Usuarios
struct User {
  char    name[20];
  char    email[32];
  uint8_t age;
};

// Tabla Hija: Pedidos
struct Order {
  uint32_t user_id;   // Clave Foránea (FK) hacia User.id
  char     item[20];
  float    price;
};

MicroDB db;
Table<User>  usersTable;
Table<Order> ordersTable;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println(F("\n========================================================"));
  Serial.println(F("   MICRODB - INTEGRIDAD REFERENCIAL (FK) Y RESTRICCIONES "));
  Serial.println(F("========================================================"));

  if (!db.begin("DB_FK", SD_CS_PIN)) {
    Serial.println(F("Error al iniciar SD"));
    while (1);
  }

  usersTable  = db.openTable<User>("users");
  ordersTable = db.openTable<Order>("orders");

  // --------------------------------------------------------------------------
  // 1. REGISTRO DE METADATOS CON RELACIONES Y RESTRICCIONES
  // --------------------------------------------------------------------------
  Serial.println(F("[1] Registrando Metadatos con Relaciones y Unicidad en SD..."));

  // users: email es UNIQUE
  usersTable
    .addColumn("name",       TYPE_STRING, offsetof(User, name),  sizeof(User::name))
    .addUniqueColumn("email",TYPE_STRING, offsetof(User, email), sizeof(User::email))
    .addColumn("age",        TYPE_UINT8,  offsetof(User, age),   sizeof(uint8_t));
  usersTable.saveSchema(); // Genera DB/users.jsn con isUnique: true

  // orders: user_id es Clave Foránea (FK -> users.id)
  ordersTable
    .addForeignKey("user_id", TYPE_UINT32, offsetof(Order, user_id), sizeof(uint32_t), "users", "id")
    .addColumn("item",        TYPE_STRING, offsetof(Order, item),    sizeof(Order::item))
    .addColumn("price",       TYPE_FLOAT,  offsetof(Order, price),   sizeof(float));
  ordersTable.saveSchema(); // Genera DB/orders.jsn con isForeignKey y references

  // Imprimir los esquemas auto-descubiertos
  usersTable.printSchema();
  ordersTable.printSchema();

  // Limpiar para la demostración
  usersTable.truncate();
  ordersTable.truncate();

  // --------------------------------------------------------------------------
  // CASO 1: RESTRICCIÓN DE VALIDACIÓN (CHECK CONSTRAINT) EN INSERCIÓN
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 1: RESTRICCIONES DE VALIDACION (CHECK) ---"));

  // Intentar registrar un usuario con email inválido (sin '@') y menor de edad
  User usuarioInvalido = { "Usuario Invalido", "correo_sin_arroba.com", 15 };
  Serial.println(F("1. Intentando insertar usuario menor de edad y con email sin '@'..."));

  uint32_t idInvalido = usersTable.insertIf(usuarioInvalido, [](const User& u) {
    return (strlen(u.name) > 0) && (strchr(u.email, '@') != nullptr) && (u.age >= 18);
  });

  if (idInvalido == 0) {
    Serial.println(F("--> [RESULTADO]: Insercion rechazada con exito por violar regla CHECK."));
  }

  // Insertar un usuario válido
  User usuarioValido = { "Jairo Rohatan", "jairo@example.com", 28 };
  Serial.println(F("\n2. Insertando usuario valido con reglas CHECK aprobadas..."));
  uint32_t idValido = usersTable.insertIf(usuarioValido, [](const User& u) {
    return (strlen(u.name) > 0) && (strchr(u.email, '@') != nullptr) && (u.age >= 18);
  });

  Serial.print(F("--> [RESULTADO]: Usuario insertado exitosamente con ID #"));
  Serial.println(idValido);

  // --------------------------------------------------------------------------
  // CASO 2: VALIDACIÓN DE CLAVE FORÁNEA (FOREIGN KEY INTEGRITY)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 2: INTEGRIDAD DE CLAVE FORANEA (FOREIGN KEY) ---"));

  // Intentar crear un pedido con un userId que NO existe (ej: user_id = 999)
  Order pedidoHuerfano = { 999, "Sensor Laser", 45.0f };
  Serial.println(F("1. Intentando insertar pedido para cliente inexistente (ID #999)..."));

  uint32_t idPedidoHuerfano = ordersTable.insertWithFK(pedidoHuerfano, usersTable, pedidoHuerfano.user_id);
  if (idPedidoHuerfano == 0) {
    Serial.println(F("--> [RESULTADO]: Insercion abortada! No se permiten pedidos huerfanos."));
  }

  // Insertar pedido para el cliente real (idValido = 1)
  Order pedidoValido = { idValido, "Arduino Mega", 38.5f };
  Serial.print(F("\n2. Insertando pedido para cliente existente (ID #"));
  Serial.print(idValido); Serial.println(F(")..."));

  uint32_t idPedidoValido = ordersTable.insertWithFK(pedidoValido, usersTable, pedidoValido.user_id);
  Serial.print(F("--> [RESULTADO]: Pedido insertado exitosamente con ID #"));
  Serial.println(idPedidoValido);

  // --------------------------------------------------------------------------
  // CASO 3: BORRADO CON RESTRICCIÓN (ON DELETE RESTRICT)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 3: BORRADO CON RESTRICCION (ON DELETE RESTRICT) ---"));
  Serial.println(F("Intentando eliminar al usuario ID #1 que tiene un pedido pendiente..."));

  bool eliminado = usersTable.removeRestrict(idValido, ordersTable, [](const Order& o) {
    return o.user_id;
  });

  if (!eliminado) {
    Serial.println(F("--> [RESULTADO]: Eliminacion bloqueada! El usuario tiene pedidos asociados."));
  }

  // --------------------------------------------------------------------------
  // CASO 4: BORRADO EN CASCADA (ON DELETE CASCADE)
  // --------------------------------------------------------------------------
  Serial.println(F("\n--- CASO 4: BORRADO EN CASCADA (ON DELETE CASCADE) ---"));
  Serial.println(F("Eliminando usuario ID #1 y borrando automaticamente todos sus pedidos..."));

  uint32_t pedidosBorrados = usersTable.removeCascade(idValido, ordersTable, [](const Order& o) {
    return o.user_id;
  });

  Serial.print(F("--> [RESULTADO]: Usuario eliminado y "));
  Serial.print(pedidosBorrados);
  Serial.println(F(" pedidos asociados eliminados en cascada."));

  Serial.print(F("--> Total usuarios activos restantes: ")); Serial.println(usersTable.count());
  Serial.print(F("--> Total pedidos activos restantes:  ")); Serial.println(ordersTable.count());

  Serial.println(F("\n========================================================"));
  Serial.println(F("   PRUEBAS DE INTEGRIDAD Y RESTRICCIONES COMPLETADAS     "));
  Serial.println(F("========================================================"));
}

void loop() {}
