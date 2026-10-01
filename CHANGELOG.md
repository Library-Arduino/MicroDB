# Changelog - MicroDB (Arduino Library)

Todas las modificaciones notables realizadas en esta librería serán documentadas en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/), y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

---

## [1.1.0] - 2026-10-01

### 🚀 Nuevas Funcionalidades
- **Gestión Dinámica de Integridad Referencial en Eliminación (`removeRelation`):**
  - Validación en tiempo de ejecución de registros dependientes antes de borrar en la tabla padre.
  - Soporte de políticas configurables a través del nuevo `enum CascadeAction`:
    - `CASCADE_RESTRICT`: Impide la eliminación del registro si existen referencias dependientes.
    - `CASCADE_DELETE`: Borra automáticamente en cascada todos los registros hijos relacionados.
    - `CASCADE_SET_NULL`: Desvincula los registros dependientes ejecutando un callback de usuario (ej: asignando `foreignKey = 0`) y luego borra el registro padre.
    - `CASCADE_FORCE`: Fuerza la eliminación del registro padre sin alterar los registros hijos.
- **Nuevo Método de Desvinculación Directa (`removeSetNull`):**
  - Atajo dedicado para poner en nulo/cero claves foráneas dependientes y eliminar al padre en una sola operación atómica.
- **Nuevo Código de Error `DB_ERROR_FOREIGN_KEY_RESTRICT`:**
  - Código explícito en `DBResult` para identificar rechazos de integridad referencial.
- **Resaltado de Sintaxis Expandido:**
  - Inclusión de `CascadeAction`, `removeRelation`, `removeSetNull` y los literales `CASCADE_*` en `keywords.txt`.

### 🔧 Optimizaciones y Mejoras
- **Aislamiento de Tests de Desarrollo:**
  - Configuración en `.gitignore` para ignorar bocetos temporales y suites de pruebas locales (`examples/*_DevTest/`, `examples/DevTest*`).
- **Documentación de API:**
  - Guía completa y ejemplos de uso en `README.md` para las nuevas políticas de eliminación relacional.

---

## [1.0.1] - 2026-09-22

### 🚀 Nuevas Funcionalidades
- **Inserciones Masivas de Alto Rendimiento (Bulk Insert):**
  - Nuevos métodos `beginBulk()`, `insertBulk()`, `insertBulkWithFK()` y `endBulk()`.
  - Agrupación de escrituras continuas minimizando el overhead de sincronización y flushing en tarjetas SD.
- **Validación de Clave Foránea con Existencia:**
  - Método `insertWithFKAndCheck()` para garantizar integridad referencial estricta antes de escribir en disco.
- **Soporte de Esquema Auto-Descriptivo y Reflection:**
  - Métodos `addColumn()`, `addUniqueColumn()`, `addForeignKey()`, `saveSchema()` y `printSchema()`.
  - Exportación automática a formato `.jsn` compatible con MicroDB Studio y clientes externos.
- **Manifiesto PlatformIO:**
  - Inclusión de `library.json` para integración en el ecosistema PlatformIO.

### 🔧 Optimizaciones y Mejoras
- Optimización de acceso aleatorio $O(1)$ en lectura y actualización por `id` o `slot`.
- Reciclaje de espacio $O(1)$ mediante Free-List LIFO para slots eliminados.
- Expansión de palabras clave para resaltado de sintaxis en `keywords.txt`.
- Documentación exhaustiva en `README.md` con casos de uso industriales y benchmarks de memoria.

---

## [1.0.0] - 2026-09-20

### 🌟 Lanzamiento Inicial
- **Motor DBMS Relacional Embebido:** Motor de almacenamiento binario de registros de longitud fija sobre tarjetas SD mediante interfaz SPI estándar.
- **Consultas con Streaming Cursors:** Evaluación de predicados con uso de RAM constante $O(1)$ mediante callbacks `forEach` y `where`.
- **Indexación Secundaria en Memoria / Disco:** Búsqueda rápida mediante hashing FNV-1a en archivos `.idx`.
- **Integridad Relacional:** Soporte para restricciones de eliminación `removeRestrict()` y `removeCascade()`.
- **Operaciones INNER JOIN:** Cruce relacional eficiente entre dos tablas en disco sin desbordamiento de memoria dinámica.
- **Desfragmentación en Caliente (Vacuum):** Purgado físico de registros borrados y recompactación de tabla en disco.
