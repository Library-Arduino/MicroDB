---
name: "💡 Sugerencia o Mejora para la Librería (Feature Request)"
about: "Propón nuevas funciones, algoritmos, optimizaciones de memoria o compatibilidad para la librería C++ MicroDB"
title: "[FEATURE]: "
labels: ["enhancement", "idea"]
assignees: []
---

### 🚀 ¿Tu sugerencia está relacionada con una necesidad o problema actual?
Una descripción clara del caso de uso o la limitación actual.  
*Ej: "En microcontroladores con poca RAM (como el ATmega328P), sería útil poder..."*

---

### 💡 Funcionalidad Propuesta
Describe en detalle la nueva función, método de API C++, optimización o algoritmo que te gustaría que tuviera **MicroDB**:
- Métodos sugeridos (ej. `db.countWhere(...)`, `table.bulkInsert(...)`, etc.)
- Modificaciones en la gestión de índices (`.idx`), estructuras binarias (`MTB1`/`MID1`) o Free-List.

---

### 💻 Ejemplo de Sintaxis Deseada en C++
Muestra cómo imaginas que se escribiría el código en un sketch de Arduino:

```cpp
// Ejemplo de uso de la nueva funcionalidad
MicroDB db("/TELEMETRY");
auto cursor = db.table<SensorReadings>("readings").query()
                .where("temp", GT, 25.0f)
                .limit(10);
```

---

### 🎯 Microcontroladores y Aplicaciones Beneficiadas
- [ ] **AVR de 8 bits:** (Arduino Uno, Nano, Mega 2560)
- [ ] **32 bits de alto rendimiento:** (ESP32, ESP8266, STM32, RP2040, Teensy)
- [ ] **Sistemas de Archivos Alternativos:** (LittleFS, SPIFFS, Flash SPI)
- [ ] **Otros:** (Datalogging industrial, IoT, estaciones meteorológicas, robots)

---

### 📌 Contexto Adicional
Cualquier información adicional, referencias bibliográficas, algoritmos o diagramas relacionados.
