# MicroDB - Embedded Relational Database for SD Cards

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Author](https://img.shields.io/badge/Author-Jairo%20Antonio%20Rohatan%20Zapata-blue.svg)](https://github.com/Jairo2020)
[![Repository](https://img.shields.io/badge/GitHub-Library--Arduino%2FMicroDB-blue.svg)](https://github.com/Library-Arduino/MicroDB.git)
[![Arduino Compatible](https://img.shields.io/badge/Platform-Arduino%20%7C%20ESP32%20%7C%20STM32%20%7C%20RP2040-green.svg)](https://www.arduino.cc/)

**MicroDB** es un motor de base de datos relacional embebido de alto rendimiento diseñado para almacenar, indexar y consultar grandes volúmenes de datos en tarjetas SD conectadas a microcontroladores (Arduino Uno/Mega, ESP32, ESP8266, STM32, RP2040, Teensy).

A diferencia de los archivos CSV o JSON de texto plano que sufren de lecturas lentas $O(N)$ y desgaste prematuro de la memoria flash, **MicroDB** utiliza un formato binario estructurado por sectores físicos (512 bytes), garantizando operaciones CRUD directas en **$O(1)$**, reciclaje de espacio mediante *Free-List* (Tombstone) y consultas relacionales (`INNER JOIN`) con consumo de RAM constante en streaming (**$O(1)$ RAM**).

---

## Características Principales

* ⚡ **Operaciones CRUD en $O(1)$:** Lectura, inserción, actualización *in-place* y borrado sin tener que recorrer todo el archivo.
* 🛡️ **Persistencia Segura ante Reinicios:** Recuperación instantánea del estado de la base de datos tras cortes de energía o reinicios del microcontrolador.
* 📋 **Auto-Descubrimiento de Columnas (Schema Reflection):** Genera automáticamente metadatos en formato `.jsn` (JSON FAT 8.3) y `.sch` (binario) en la SD para que cualquier software externo (Python, C#, Electron, Web) lea e interprete todas las columnas automáticamente sin configuración manual.
* 🚫 **Valores Únicos No Repetidos (UNIQUE Constraints):** Evita duplicados en campos numéricos (`insertUnique`) o cadenas (`insertUniqueString` para emails, DNI/cédula, seriales o SKUs).
* 🔄 **Operaciones UPSERT:** Inserta automáticamente un registro si es nuevo, o lo actualiza *in-place* si ya existe (`upsertUnique`, `upsertUniqueString`).
* 🧹 **Mantenimiento y Compactación (VACUUM):** Desfragmenta el archivo en la SD y recupera el 100% del espacio en disco tras borrados masivos.
* 🔒 **Integridad Referencial y Claves Foráneas (FK):** Validación $O(1)$ de existencia de IDs padres para evitar registros huérfanos (`insertWithFK`).
* ✅ **Restricciones de Validación (CHECK Constraints):** Reglas para validar emails, cadenas no vacías o rangos numéricos (`insertIf`, `updateIf`) con **cero sobrecarga de RAM**.
* 🚫 **Borrado Seguro y en Cascada:** Soporte para `removeRestrict` (impide borrar padres con hijos) y `removeCascade` (borra hijos automáticamente).
* ♻️ **Reciclaje Automático de Espacio (Free-List):** Los registros borrados se reutilizan en los siguientes inserts, evitando el crecimiento indefinido del archivo.
* 🔍 **Índices Secundarios ($O(\log N)$):** Búsqueda instantánea por campos de texto (mediante hashing FNV-1a de 32 bits) o numéricos sin escanear la tabla completa.
* 🔗 **Soporte Relacional (JOINs):** Consultas tipo `INNER JOIN` entre múltiples tablas relacionadas.
* 📊 **Analítica en Tiempo Real:** Cálculo de agregaciones (`MIN`, `MAX`, `AVG`, `SUM`), histogramas y detección de anomalías sin desbordar la memoria RAM.
* 📉 **Streaming de Memoria:** Procesa millones de filas consumiendo menos de 1 KB de RAM.
* 📤 **Exportación a CSV:** Exporta los resultados de cualquier consulta binaria a CSV para Excel o PC.
* 📦 **Tipado Fuerte en C++:** Define tus tablas directamente a partir de cualquier `struct` en C++.
* 🩺 **Diagnóstico y Alertas en Serial:** Monitorea el estado de la RAM y previene desbordamientos de pila (*Stack Overflow*).

---

## Orden de Ejecución y Ciclo de Vida Obligatorio

Para garantizar la integridad física de los datos, evitar fugas de memoria y asegurar que las restricciones de clave foránea y metadatos funcionen correctamente, **MicroDB** implementa un ciclo de vida estructurado en 5 pasos lógicos:

```
[1. db.begin()] ──> [2. db.openTable() Padres] ──> [3. db.openTable() Hijas] ──> [4. addColumn/saveSchema] ──> [5. Inserts / Queries / Joins]
```

### 1. Inicialización Global del Motor (`db.begin`)
* Debe ejecutarse **una sola vez** en la función `setup()`.
* Crea el directorio principal de la base de datos en la tarjeta SD e inicializa el bus SPI.
```cpp
MicroDB db;
if (!db.begin("DB", SD_CS_PIN)) {
    Serial.println(F("Error al iniciar MicroDB"));
    while(1);
}
```

### 2. Apertura de Tablas Padre / Maestras
* Si existen relaciones con claves foráneas, **siempre abre primero las tablas padre**.
```cpp
Table<User> usersTable = db.openTable<User>("users");
```

### 3. Apertura de Tablas Hijas / Dependientes
* Abre las tablas que referencian a las tablas padre.
```cpp
Table<Order> ordersTable = db.openTable<Order>("orders");
```

### 4. Definición de Esquema y Metadatos (`saveSchema`)
* Define los nombres de columnas, restricciones UNIQUE y referencias de Foreign Key.
* Llama a `.saveSchema()` para generar los archivos `.jsn` y `.sch` en la SD.
```cpp
usersTable.addColumn("userId", "uint32", 4)
          .addUniqueColumn("email", "string", 32)
          .saveSchema();

ordersTable.addColumn("orderId", "uint32", 4)
           .addForeignKey("userId", "uint32", 4, "users")
           .saveSchema();
```

### 5. Inserciones, Consultas y Relaciones (CRUD & Queries)
* **Inserts:** Primero inserta en la tabla padre antes de insertar registros hijos referenciados con `insertWithFK`.
* **Consultas:** Utiliza `where`, `forEach`, o `db.innerJoin` en cualquier momento una vez las tablas estén abiertas.
* **Borrado Relacional:** Usa `usersTable.removeRestrict(id, ordersTable, getFK)` o `usersTable.removeCascade(id, ordersTable, getFK)` según la lógica de negocio.

---

## Detección Automática de Errores de Secuencia

Si ejecutas una operación fuera de orden (por ejemplo, intentar insertar antes de inicializar la base de datos o abrir la tabla), **MicroDB** cancela la operación de forma segura y emite una alerta diagnóstica por el puerto Serial:

| Error Detectado | Mensaje en Serial Monitor | Solución |
| :--- | :--- | :--- |
| **Tabla no abierta** | `[MicroDB ORDEN ERROR] Operacion 'insert' fallida: La tabla 'users' NO esta abierta.` | Llama a `db.openTable<T>("users")` primero. |
| **Falta `db.begin()`** | `[MicroDB ORDEN ERROR] Intentaste abrir la tabla 'users' ANTES de inicializar la base de datos.` | Llama a `db.begin("DB", CS_PIN)` en `setup()`. |
| **JOIN con tabla cerrada** | `[MicroDB ORDEN ERROR] No se puede ejecutar innerJoin(): -> La tabla hija 'orders' NO esta abierta.` | Asegúrate de abrir ambas tablas antes del `innerJoin`. |
| **Tabla Padre Desconectada en FK** | `[MicroDB ORDEN ERROR] La tabla padre 'users' debe estar abierta para validar claves foraneas.` | Abre `usersTable` antes de llamar a `ordersTable.insertWithFK()`. |

---

## Compatibilidad de Memoria y Límites de Hardware

**MicroDB** adapta automáticamente su consumo de memoria RAM (footprint) en tiempo de compilación según el microcontrolador detectado, garantizando que nunca se desborde la memoria ni siquiera en microcontroladores de bajos recursos:

| Plataforma / Microcontrolador | RAM Total (SRAM) | RAM por Tabla | Tablas Simultáneas Recomendadas | Máx. Columnas por Tabla | Longitud Máx. Nombres |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Arduino Uno / Nano / Pro Mini / Micro** (ATmega328P / 32U4) | **2 KB** | $\approx$ 340 bytes | **1 a 3 tablas** abiertas a la vez | **6 columnas** | 10 caracteres |
| **Arduino Mega 2560 / 1280** (ATmega2560) | **8 KB** | $\approx$ 550 bytes | **4 a 8 tablas** abiertas a la vez | **12 columnas** | 12 caracteres |
| **ESP32 / ESP8266** | **320 KB+** | $\approx$ 750 bytes | **10+ tablas** abiertas a la vez | **16 columnas** | 16 caracteres |
| **STM32 / RP2040 / Teensy / SAMD** | **32 KB a 1 MB** | $\approx$ 750 bytes | **10+ tablas** abiertas a la vez | **16 columnas** | 16 caracteres |

---

## Convenciones de Nombres y Estándar FAT 8.3

Las tarjetas SD en microcontroladores utilizan el sistema de archivos **FAT 8.3**. Para garantizar un funcionamiento óptimo y evitar rechazos por parte del controlador físico:

1. **Directorios de Base de Datos (`db.begin("DB_NAME")`):**
   * Longitud máxima recomendada: **8 caracteres**.
   * Caracteres permitidos: Letras (`A-Z`, `a-z`), números (`0-9`) y guiones bajos (`_`).
   * Ejemplos válidos: `"DB"`, `"DB_PROD"`, `"STORE"`, `"LOGS_24"`.
2. **Nombres de Tablas (`db.openTable("name")`):**
   * Longitud máxima recomendada: **8 caracteres**.
   * Ejemplos válidos: `"users"`, `"orders"`, `"products"`, `"sensors"`, `"telem"`.
   * *Nota de seguridad:* Si se ingresa un nombre con más de 8 caracteres (ej. `"telemetry"`), el motor lo trunca automáticamente a 8 caracteres (`"telemetr"`) para garantizar la compatibilidad con el driver FAT.
3. **Extensiones de Archivos Generadas en la SD:**
   * Archivo binario de registros: `.tbl` (ej. `users.tbl`)
   * Esquema binario: `.sch` (ej. `users.sch`)
   * Metadatos JSON universales: `.jsn` (ej. `users.jsn`, extensión de 3 letras compatible con FAT 8.3)
   * Índices secundarios: `.idx` (ej. `email.idx`)

---

## Configuración y Directivas Personalizables

Puedes ajustar el comportamiento del motor y optimizar el uso de memoria definiendo cualquiera de las siguientes directivas antes de incluir la librería:

```cpp
// 1. Activar o desactivar diagnósticos y alertas por Serial (1=Activado por defecto, 0=Silenciar)
#define MICRODB_ENABLE_DIAGNOSTICS 1

// 2. Cantidad máxima de columnas por tabla (por defecto: 6 en Arduino Uno, 12 en Mega, 16 en ESP32/ARM)
#define MICRODB_MAX_COLUMNS 8

// 3. Longitud máxima de los nombres de columnas y tablas (por defecto: 10 en Uno, 12 en Mega, 16 en ESP32/ARM)
#define MICRODB_NAME_LEN 12

// 4. Longitud máxima para buffers de rutas completas en la SD (por defecto: 28 en AVR, 32 en 32-bit)
#define MICRODB_PATH_LEN 28

#include <MicroDB.h>
```

### Tabla de Parámetros de Configuración:

| Macro / Directiva | Valor por Defecto (AVR Uno) | Valor por Defecto (ESP32/ARM) | Descripción |
| :--- | :--- | :--- | :--- |
| `MICRODB_ENABLE_DIAGNOSTICS` | `1` (Habilitado) | `1` (Habilitado) | Muestra reportes de RAM, alertas de stack overflow y diagnósticos de ciclo de vida en Serial. Pon `0` para ahorrar memoria Flash. |
| `MICRODB_MAX_COLUMNS` | `6` | `16` | Límite máximo de columnas registrables por tabla (`addColumn`). |
| `MICRODB_NAME_LEN` | `10` | `16` | Cantidad de caracteres máximos por nombre de columna y tabla. |
| `MICRODB_PATH_LEN` | `28` | `32` | Tamaño del buffer de rutas en la SD (`dir/tabla.ext`). |

---

## Repositorio Oficial
* **GitHub:** [https://github.com/Library-Arduino/MicroDB.git](https://github.com/Library-Arduino/MicroDB.git)

---

## Ejemplos Incluidos

Cada ejemplo utiliza su propia carpeta en la SD para que puedas ejecutarlos todos de forma independiente sin interferir ni borrar los datos de otros tests:

1. **QuickStart** (`DB_QUICK/`): Inicialización básica y consulta por streaming.
2. **FullCRUD** (`DB_CRUD/`): Guía paso a paso de las 4 operaciones: Creación (Insert), Lectura directa $O(1)$, Actualización in-place y Borrado con reciclaje de espacio libre.
3. **SelfDescribingSchema** (`DB_SCHEM/`): Auto-descubrimiento de columnas y metadatos JSON para software externo en PC.
4. **UniqueAndUpsert** (`DB_UNQ/`): Restricciones de valores únicos no repetidos (UNIQUE), inserción/actualización automática (UPSERT) y compactación física (VACUUM).
5. **ConstraintsAndFKs** (`DB_FK/`): Validación de Claves Foráneas (FK), restricciones CHECK, borrado restrictivo y en cascada.
6. **RelationalJoin** (`DB_JOIN/`): Relaciones directas entre tablas con clave foránea mediante `innerJoin()`.
7. **AllDataTypesDemo** (`DB_TYPES/`): Manejo exhaustivo de todos los tipos (`bool`, enteros de 8 a 64 bits, `float`, `double`, `char[]`, BLOBs de bytes, JSON y punteros de archivos).
8. **AnalyticsAndAggregation** (`DB_STATS/`): Estadísticas (`MIN`, `MAX`, `AVG`, `SUM`), detección de anomalías térmicas y distribución en histogramas.
9. **AdvancedMultiJoin** (`DB_MULTI/`): Modelo relacional complejo de 3 tablas (Clientes $\leftrightarrow$ Pedidos $\leftrightarrow$ Productos) y exportador automático a archivo CSV para Excel.

---

## Autor y Licencia

* **Autor:** [Jairo Antonio Rohatan Zapata](https://github.com/Jairo2020)
* **Repositorio Oficial:** [Library-Arduino/MicroDB](https://github.com/Library-Arduino/MicroDB.git)
* **Licencia:** MIT License (Código abierto para uso personal, comercial y educativo).

