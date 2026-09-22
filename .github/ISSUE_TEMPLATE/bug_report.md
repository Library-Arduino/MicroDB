---
name: "🐛 Reporte de Error en la Librería (Bug Report)"
about: "Reporta un problema en el código C++, compilación o ejecución de la librería MicroDB para Arduino/ESP32"
title: "[BUG]: "
labels: ["bug", "triage"]
assignees: []
---

### 📌 Descripción del Problema
Describe de forma clara y concisa qué error o comportamiento anómalo ocurre al usar la librería **MicroDB**.

---

### ⚙️ Entorno de Desarrollo y Hardware
- **Placa / Microcontrolador:** (ej. ESP32-WROOM-32D, Arduino Mega 2560, Arduino Uno R3, STM32F401, Raspberry Pi Pico)
- **Entorno de Programación:** (ej. Arduino IDE 2.3.2, VS Code + PlatformIO, Arduino CLI)
- **Versión del Core:** (ej. `esp32` by Espressif v2.0.14, `arduino:avr` v1.8.6)
- **Versión de MicroDB:** (ej. v1.0.1)
- **Librería SD Utilizada:** (ej. `SD.h` estándar de Arduino, `SdFat.h` v2.2.3, `SD_MMC.h`)
- **Tarjeta MicroSD:** Marca, capacidad y formato (ej. SanDisk Ultra 32GB formateada en FAT32)

---

### 🔄 Pasos para Reproducir
1. Inicializar la base de datos con `MicroDB db('/...')...`
2. Crear o abrir la tabla con `...`
3. Ejecutar la operación `insert()`, `update()`, `remove()`, `find()` o `vacuum()`...
4. Observar el fallo o mensaje en el Monitor Serial.

---

### 💻 Código del Sketch (.ino / C++)
Pega un ejemplo mínimo y reproducible que demuestre el error:

```cpp
#include <SPI.h>
#include <SD.h>
#include <MicroDB.h>

struct MiDato {
    uint32_t id;
    char texto[32];
    float valor;
};

void setup() {
    Serial.begin(115200);
    // Tu código de inicialización y reproducción aquí
}

void loop() {
}
```

---

### 📜 Salida del Monitor Serial / Error del Compilador
Pega los mensajes de error del compilador o la salida por puerto serie (incluyendo cualquier Panic, Stack Trace o mensaje de error de MicroDB):

```text
// Logs del Monitor Serial o error de compilación
```

---

### 🎯 Comportamiento Esperado
¿Qué esperabas que hiciera la librería en este caso?
