# Instrucciones del Proyecto & Agentes - Librería Arduino MicroDB

Este archivo define las directivas y el comportamiento para los asistentes de inteligencia artificial (Antigravity / Gemini) que interactúan con el repositorio **MicroDB** (`Library-Arduino/MicroDB`).

---

## 🛠️ Tecnologías y Arquitectura

- **Lenguaje:** C++ embebido (C++11 / C++14) para microcontroladores.
- **Plataformas Soportadas:** ESP32, ESP8266, STM32, Arduino AVR (Uno, Nano, Mega), RP2040 (Raspberry Pi Pico).
- **Almacenamiento:** Tarjetas MicroSD formateadas en FAT16/FAT32 vía bus SPI.
- **Arquitectura de Base de Datos:**
  - Registros de longitud fija en disco (`.tbl`).
  - Índices secundarios basados en hash FNV-1a (`.idx`).
  - Esquemas JSON auto-descriptivos (`.jsn`) y binarios (`.sch`).
  - Free-List en memoria y disco para reciclaje de slots $O(1)$.
  - Streaming Cursors con consumo de RAM constante $O(1)$.

---

## 🚀 Agente de Versiones y Protocolo de Release (Tags)

Cuando el usuario solicite crear una nueva versión, preparar un tag, subir una mejora o actualizar la librería MicroDB, **DEBES SEGUIR OBLIGATORIAMENTE ESTE PROTOCOLO**:

### 1. Verificación de Compilación C++ (Salud de la Librería)
- Ejecuta `node scripts/release.mjs check`.
- Este paso usa `arduino-cli` para compilar el ejemplo `examples/QuickStart/QuickStart.ino` contra el núcleo `arduino:avr:uno`.
- **NUNCA continúes si la compilación falla.** Si hay errores de tipos en C++, sintaxis o desbordamiento de memoria Flash/SRAM, detén el proceso e informa al usuario.

### 2. Análisis de Nuevas Funcionalidades en Git
- Inspecciona `git status`, `git diff` y `git log $(git describe --tags --abbrev=0)..HEAD --oneline`.
- Identifica:
  - Nuevas clases, métodos, tipos o macros en `src/`.
  - Nuevos ejemplos en `examples/`.
  - Optimizaciones de rendimiento o de memoria SRAM.

### 3. Documentación y Palabras Clave
- Si se agregaron nuevos métodos o clases:
  - Documenta su firma y ejemplo de uso en C++ en [README.md](file:///c:/Users/jjair/OneDrive/Documentos/Arduino/ARDUINO%20PROYECTO/PROBADOR/SD/MicroDB/README.md).
  - Agrégalos a [keywords.txt](file:///c:/Users/jjair/OneDrive/Documentos/Arduino/ARDUINO%20PROYECTO/PROBADOR/SD/MicroDB/keywords.txt) con la categoría adecuada (`KEYWORD1`, `KEYWORD2`, `LITERAL1`) separados con tabulador (`\t`).
- Redacta la sección correspondiente en [CHANGELOG.md](file:///c:/Users/jjair/OneDrive/Documentos/Arduino/ARDUINO%20PROYECTO/PROBADOR/SD/MicroDB/CHANGELOG.md).

### 4. Incremento de Versión Semántica (SemVer)
- Ejecuta `node scripts/release.mjs bump <patch|minor|major>`.
- Esto sincroniza automáticamente:
  - `version=X.Y.Z` en `library.properties`.
  - `"version": "X.Y.Z"` en `library.json`.

### 5. Git Commit y Git Tag
- Realiza el commit: `git add library.properties library.json keywords.txt CHANGELOG.md README.md src/ examples/ && git commit -m "chore(release): vX.Y.Z - <resumen de mejoras>"`
- Crea el tag anotado: `git tag -a vX.Y.Z -m "Release vX.Y.Z: <resumen de mejoras>"`

### 6. Formato Listo para GitHub Releases
- Ejecuta `node scripts/release.mjs notes <X.Y.Z> "<Título del Release>"`
- Proporciona al usuario el título y el Markdown completo para copiar y pegar en [GitHub Releases](https://github.com/Library-Arduino/MicroDB/releases/new):
  - **Título del Release:** `MicroDB vX.Y.Z - <Título descriptivo>`
  - **Tag:** `vX.Y.Z` | **Rama:** `main`
  - **Descripción en Markdown:** Resumen, nuevas funcionalidades con snippets en C++, tabla de compatibilidad de microcontroladores y guía de instalación.
  - Comando recordatorio para publicar: `git push origin main --tags`.
