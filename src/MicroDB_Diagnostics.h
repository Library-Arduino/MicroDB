#ifndef MICRODB_DIAGNOSTICS_H
#define MICRODB_DIAGNOSTICS_H

#include <Arduino.h>
#include "MicroDB_Config.h"

// ============================================================================
// MICRODB - SISTEMA DE MONITOREO DE MEMORIA Y ALERTAS DE RENDIMIENTO
// ============================================================================

class MicroDB_Diagnostics {
public:
    // Retorna la cantidad de memoria RAM libre real disponible en el MCU
    static uint32_t getFreeRam() {
#if defined(ESP32)
        return ESP.getFreeHeap();
#elif defined(ESP8266)
        return ESP.getFreeHeap();
#elif defined(__AVR__)
        extern int __heap_start, *__brkval;
        int v;
        return (uint32_t)((int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval));
#elif defined(ARDUINO_ARCH_RP2040)
        return rp2040.getFreeHeap();
#else
        return 0; // No disponible para esta arquitectura
#endif
    }

    // Imprime reporte de estado de memoria
    static void printMemoryReport(const char* context = nullptr) {
#if MICRODB_ENABLE_DIAGNOSTICS
        uint32_t freeRam = getFreeRam();
        Serial.print(F("[MicroDB DIAGNOSTICO] "));
        if (context) {
            Serial.print(context);
            Serial.print(F(" | "));
        }
        Serial.print(F("RAM Libre en MCU: "));
        Serial.print(freeRam);
        Serial.println(F(" bytes"));

        checkRamHealth(freeRam);
#endif
    }

    // Evalúa si la memoria RAM se encuentra en niveles críticos
    static void checkRamHealth(uint32_t freeRam) {
#if MICRODB_ENABLE_DIAGNOSTICS
#if defined(__AVR__)
        // En microcontroladores AVR (2KB a 8KB RAM)
        if (freeRam < 250) {
            Serial.println(F("[MicroDB CRITICO] ¡PELIGRO DE STACK OVERFLOW! Menos de 250B de RAM libres."));
            Serial.println(F("                  Reduce el tamaño de tus structs o buffers de texto."));
        } else if (freeRam < 500) {
            Serial.println(F("[MicroDB ALERTA] Memoria RAM baja (<500 bytes). Monitorea el consumo."));
        }
#elif defined(ESP32) || defined(ESP8266)
        if (freeRam < 10240) { // Menos de 10 KB en ESP32
            Serial.println(F("[MicroDB ALERTA] Heap bajo en ESP (<10KB). Revisa posibles fugas."));
        }
#endif
#endif
    }

    // Evalúa el tamaño de un registro para alertar sobre impacto en el sector SD (512B)
    static void checkRecordSize(const char* tableName, size_t recordSize) {
#if MICRODB_ENABLE_DIAGNOSTICS
        if (recordSize > 256) {
            Serial.print(F("[MicroDB AVISO] Tabla '"));
            Serial.print(tableName);
            Serial.print(F("': Registro grande ("));
            Serial.print(recordSize);
            Serial.println(F(" bytes). Caben < 2 registros por sector de 512B en la SD."));
        }
        if (recordSize > 512) {
            Serial.print(F("[MicroDB ALERTA] Tabla '"));
            Serial.print(tableName);
            Serial.print(F("': El registro supera los 512 bytes de un sector físico."));
            Serial.println(F("                 Cada lectura/escritura requerira multiples accesos a la SD."));
        }
#endif
    }

    // Alerta sobre escaneos lineales en tablas grandes sin índices
    static void checkScanPerformance(const char* tableName, uint32_t recordCount) {
#if MICRODB_ENABLE_DIAGNOSTICS
        if (recordCount >= 500) {
            Serial.print(F("[MicroDB RENDIMIENTO] Escaneo lineal sobre '"));
            Serial.print(tableName);
            Serial.print(F("' con "));
            Serial.print(recordCount);
            Serial.println(F(" registros."));
            Serial.println(F("                      -> Tip: Usa openIndex() para busquedas instantaneas O(log N)."));
        }
#endif
    }
};

#endif // MICRODB_DIAGNOSTICS_H
