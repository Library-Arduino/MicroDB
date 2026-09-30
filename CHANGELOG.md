# Changelog - MicroDB (Arduino Library)

Todas las modificaciones notables realizadas en esta librería serán documentadas en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/), y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

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
