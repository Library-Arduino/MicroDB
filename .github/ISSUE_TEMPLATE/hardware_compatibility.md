---
name: "🔌 Compatibilidad de Placa o Módulo SD (Hardware Compatibility)"
about: "Reporta resultados de pruebas, problemas o soporte con microcontroladores específicos o módulos SD"
title: "[HARDWARE]: "
labels: ["hardware", "testing"]
assignees: []
---

### 📋 Placa y Módulo Probado
- **Placa de desarrollo:** (ej. Raspberry Pi Pico W, Teensy 4.1, ESP32-S3, ESP8266 NodeMCU)
- **Módulo SD / Interfaz:** (ej. Módulo MicroSD SPI estándar, Lector integrado SDIO 4-bit, Shield oficial Arduino SD)
- **Voltaje de operación:** (ej. 3.3V nativo / 5V con convertidor de nivel)

---

### 🔍 Resultado de la Prueba / Problema Encontrado
- [ ] **Funciona Correctamente (Reporte de Éxito / Verified Hardware)**
- [ ] **Error en `SD.begin()` / `db.begin()`**
- [ ] **Error de corrupción de datos en operaciones de alta frecuencia**
- [ ] **Incompatibilidad de pines SPI o bus compartido (con pantallas TFT, sensores, etc.)**
- [ ] **Problemas de memoria (Out of RAM / Heap Exhaustion)**

---

### 💻 Configuración de Pines y Código de Prueba

```cpp
#define SD_CS_PIN 5
// Configuración de pines y bus SPI
```

---

### 📝 Observaciones o Recomendaciones
Consejos, resistencias pull-up necesarias o recomendaciones para otros desarrolladores que usen este hardware.
