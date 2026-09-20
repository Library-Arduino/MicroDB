#ifndef MICRODB_H
#define MICRODB_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "MicroDB_Config.h"
#include "MicroDB_Types.h"
#include "MicroDB_Diagnostics.h"
#include "MicroDB_BufferPool.h"
#include "MicroDB_Index.h"
#include "MicroDB_Schema.h"
#include "MicroDB_Table.h"

// ============================================================================
// MICRODB - GESTOR DE BASE DE DATOS RELACIONAL PARA TARJETAS SD
// ============================================================================

class MicroDB {
private:
    char dbDirectory[MICRODB_NAME_LEN + 2];
    uint8_t csPin;
    bool isInitialized;

public:
    MicroDB() : csPin(4), isInitialized(false) {
        dbDirectory[0] = '\0';
    }

    // [PASO 1 OBLIGATORIO] Inicializa el sistema de archivos SD y asegura la carpeta de la BD
    bool begin(const char* dirPath = "DB", uint8_t chipSelectPin = 4) {
        csPin = chipSelectPin;
        pinMode(csPin, OUTPUT);

        const char* cleanDir = dirPath;
        if (cleanDir && cleanDir[0] == '/') {
            cleanDir++;
        }

        strncpy(dbDirectory, cleanDir ? cleanDir : "DB", sizeof(dbDirectory) - 1);
        dbDirectory[sizeof(dbDirectory) - 1] = '\0';

        if (!SD.begin(csPin)) {
            isInitialized = false;
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ERROR INICIALIZACION] No se pudo conectar con la SD en pin CS "));
            Serial.print(csPin);
            Serial.println(F(". Revisa cables SPI y formato FAT."));
            #endif
            return false;
        }

        // Crear directorio de base de datos si no existe (formato FAT 8.3)
        if (strlen(dbDirectory) > 0 && !SD.exists(dbDirectory)) {
            SD.mkdir(dbDirectory);
        }

        isInitialized = true;

        // Diagnóstico inicial de memoria
        MicroDB_Diagnostics::printMemoryReport("Inicio MicroDB");

        return true;
    }

    // [PASO 2] Abre o crea una tabla fuertemente tipada
    template <typename T>
    Table<T> openTable(const char* name) {
        Table<T> table;
        if (!isInitialized) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ORDEN ERROR] Intentaste abrir la tabla '"));
            Serial.print(name);
            Serial.println(F("' ANTES de inicializar la base de datos."));
            Serial.println(F("                      -> Correccion: Llama a db.begin() en setup() primero."));
            #endif
            return table;
        }
        table.open(dbDirectory, name);
        return table;
    }

    // [PASO 3 OPCIONAL] Abre o crea un índice secundario para acelerar consultas O(log N)
    MicroDB_Index openIndex(const char* indexName) {
        MicroDB_Index index;
        if (!isInitialized) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.println(F("[MicroDB ORDEN ERROR] Intentaste crear un indice antes de llamar a db.begin()."));
            #endif
            return index;
        }
        char indexPath[MICRODB_PATH_LEN];
        snprintf(indexPath, sizeof(indexPath), "%s/%s.idx", dbDirectory, indexName);
        index.open(indexPath);
        return index;
    }

    // =========================================================================
    // CONSULTAS RELACIONALES (JOIN)
    // =========================================================================
    template <typename TParent, typename TChild, typename KeyExtractor, typename JoinCallback>
    void innerJoin(Table<TParent>& parentTable, 
                   Table<TChild>& childTable, 
                   KeyExtractor getForeignKeyFromChild, 
                   JoinCallback onMatch) 
    {
        if (!parentTable.isTableOpen() || !childTable.isTableOpen()) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.println(F("[MicroDB ORDEN ERROR] No se puede ejecutar innerJoin():"));
            if (!parentTable.isTableOpen()) {
                Serial.print(F("                      -> La tabla padre '"));
                Serial.print(parentTable.getName());
                Serial.println(F("' NO esta abierta."));
            }
            if (!childTable.isTableOpen()) {
                Serial.print(F("                      -> La tabla hija '"));
                Serial.print(childTable.getName());
                Serial.println(F("' NO esta abierta."));
            }
            #endif
            return;
        }

        childTable.forEach([&](uint32_t childId, const TChild& child) {
            uint32_t parentKey = getForeignKeyFromChild(child);
            TParent parent;
            if (parentTable.getById(parentKey, parent)) {
                onMatch(parent, childId, child);
            }
        });
    }

    // Reporte de diagnóstico bajo demanda
    void printMemoryDiagnostics(const char* tag = nullptr) {
        MicroDB_Diagnostics::printMemoryReport(tag);
    }

    // Retorna la RAM libre en bytes
    uint32_t getFreeRam() {
        return MicroDB_Diagnostics::getFreeRam();
    }

    const char* getPath() const { return dbDirectory; }
    bool isReady() const { return isInitialized; }
};

#endif // MICRODB_H
