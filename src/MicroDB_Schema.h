#ifndef MICRODB_SCHEMA_H
#define MICRODB_SCHEMA_H

#include <Arduino.h>
#include <SD.h>
#include "MicroDB_Config.h"
#include "MicroDB_Types.h"

// Definición de una columna individual en el catálogo de metadatos (Optimizada para RAM)
struct ColumnMetadata {
    char     name[MICRODB_NAME_LEN];          // Nombre del campo (ej: "id", "name", "email")
    uint8_t  type;                            // TYPE_BOOL, TYPE_UINT8, TYPE_INT16, TYPE_INT32, TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING, TYPE_BLOB
    uint16_t offset;                          // Desplazamiento en bytes dentro del struct
    uint16_t length;                          // Longitud en bytes del campo
    bool     isUnique;                        // ¿Tiene restricción UNIQUE?
    bool     isForeignKey;                    // ¿Es Clave Foránea (FK)?
    char     foreignTable[MICRODB_NAME_LEN];  // Tabla referenciada (ej: "users")
    char     foreignColumn[MICRODB_NAME_LEN]; // Columna referenciada (ej: "id")
};

// Conversor de tipo de dato a texto legible
inline const char* getFieldTypeName(uint8_t type) {
    switch (type) {
        case TYPE_BOOL:    return "BOOL";
        case TYPE_INT8:    return "INT8";
        case TYPE_UINT8:   return "UINT8";
        case TYPE_INT16:   return "INT16";
        case TYPE_UINT16:  return "UINT16";
        case TYPE_INT32:   return "INT32";
        case TYPE_UINT32:  return "UINT32";
        case TYPE_FLOAT:   return "FLOAT";
        case TYPE_DOUBLE:  return "DOUBLE";
        case TYPE_STRING:  return "STRING";
        case TYPE_BLOB:    return "BLOB";
        default:           return "UNKNOWN";
    }
}

// Gestor de Esquemas y Metadatos de Tablas
class TableSchema {
public:
    static const uint8_t MAX_COLUMNS = MICRODB_MAX_COLUMNS;
    ColumnMetadata columns[MICRODB_MAX_COLUMNS];
    uint8_t columnCount;
    char tableName[MICRODB_NAME_LEN + 2];

    TableSchema() : columnCount(0) {
        tableName[0] = '\0';
    }

    void init(const char* name) {
        strncpy(tableName, name, sizeof(tableName) - 1);
        tableName[sizeof(tableName) - 1] = '\0';
        columnCount = 0;
    }

    // Agrega una columna estándar al esquema
    bool addColumn(const char* colName, uint8_t colType, uint16_t colOffset, uint16_t colLength, bool isUnique = false) {
        if (columnCount >= MAX_COLUMNS) return false;

        strncpy(columns[columnCount].name, colName, sizeof(columns[columnCount].name) - 1);
        columns[columnCount].name[sizeof(columns[columnCount].name) - 1] = '\0';
        columns[columnCount].type = colType;
        columns[columnCount].offset = colOffset;
        columns[columnCount].length = colLength;
        columns[columnCount].isUnique = isUnique;
        columns[columnCount].isForeignKey = false;
        columns[columnCount].foreignTable[0] = '\0';
        columns[columnCount].foreignColumn[0] = '\0';

        columnCount++;
        return true;
    }

    // Agrega una Clave Foránea (Relación con otra tabla) al esquema
    bool addForeignKey(const char* colName, uint8_t colType, uint16_t colOffset, uint16_t colLength, const char* refTable, const char* refCol = "id") {
        if (columnCount >= MAX_COLUMNS) return false;

        strncpy(columns[columnCount].name, colName, sizeof(columns[columnCount].name) - 1);
        columns[columnCount].name[sizeof(columns[columnCount].name) - 1] = '\0';
        columns[columnCount].type = colType;
        columns[columnCount].offset = colOffset;
        columns[columnCount].length = colLength;
        columns[columnCount].isUnique = false;
        columns[columnCount].isForeignKey = true;

        strncpy(columns[columnCount].foreignTable, refTable ? refTable : "", sizeof(columns[columnCount].foreignTable) - 1);
        columns[columnCount].foreignTable[sizeof(columns[columnCount].foreignTable) - 1] = '\0';

        strncpy(columns[columnCount].foreignColumn, refCol ? refCol : "id", sizeof(columns[columnCount].foreignColumn) - 1);
        columns[columnCount].foreignColumn[sizeof(columns[columnCount].foreignColumn) - 1] = '\0';

        columnCount++;
        return true;
    }

    // Genera el archivo de Metadatos JSON (.jsn formato FAT 8.3) en la SD
    // Incluye columnas, tipos, offsets, unicidad (UNIQUE) y relaciones (FOREIGN KEYS)
    bool exportJsonSchema(const char* dirPath) {
        char jsonPath[MICRODB_PATH_LEN];
        snprintf(jsonPath, sizeof(jsonPath), "%s/%s.jsn", dirPath, tableName);

        if (SD.exists(jsonPath)) {
            SD.remove(jsonPath);
        }

        File file = SD.open(jsonPath, MICRODB_FILE_RW);
        if (!file) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ERROR] No se pudo crear metadatos '"));
            Serial.print(jsonPath);
            Serial.println(F("' en la SD."));
            #endif
            return false;
        }

        file.seek(0);
        file.print(F("{\n  \"table\": \""));
        file.print(tableName);
        file.print(F("\",\n  \"columnCount\": "));
        file.print(columnCount);
        file.println(F(",\n  \"columns\": ["));

        for (uint8_t i = 0; i < columnCount; i++) {
            file.print(F("    {\n      \"name\": \""));
            file.print(columns[i].name);
            file.print(F("\",\n      \"type\": \""));
            file.print(getFieldTypeName(columns[i].type));
            file.print(F("\",\n      \"typeId\": "));
            file.print(columns[i].type);
            file.print(F(",\n      \"offset\": "));
            file.print(columns[i].offset);
            file.print(F(",\n      \"length\": "));
            file.print(columns[i].length);

            if (columns[i].isUnique) {
                file.print(F(",\n      \"isUnique\": true"));
            }

            if (columns[i].isForeignKey) {
                file.print(F(",\n      \"isForeignKey\": true"));
                file.print(F(",\n      \"references\": {\n        \"table\": \""));
                file.print(columns[i].foreignTable);
                file.print(F("\",\n        \"column\": \""));
                file.print(columns[i].foreignColumn);
                file.print(F("\"\n      }"));
            }

            file.println(F("\n    }"));
            if (i < columnCount - 1) {
                file.print(F(","));
            }
            file.println();
        }
        file.println(F("  ]\n}"));
        file.flush();
        file.close();

        #if MICRODB_ENABLE_DIAGNOSTICS
        Serial.print(F("[MicroDB INFO] Metadatos guardados con exito en '"));
        Serial.print(jsonPath);
        Serial.println(F("'"));
        #endif

        return true;
    }

    // Imprime el esquema completo en el puerto Serial
    void printSchemaToSerial() {
        Serial.print(F("--- ESQUEMA DE METADATOS: Tabla '"));
        Serial.print(tableName);
        Serial.println(F("' ---"));
        Serial.println(F("Pos | Nombre Columna  | Tipo      | Offset | Long | Restricciones / Relaciones"));
        Serial.println(F("----+-----------------+-----------+--------+------+---------------------------"));
        for (uint8_t i = 0; i < columnCount; i++) {
            Serial.print(F(" ")); Serial.print(i + 1); Serial.print(F("  | "));
            Serial.print(columns[i].name);
            for (int s = strlen(columns[i].name); s < 15; s++) Serial.print(F(" "));
            Serial.print(F(" | "));
            Serial.print(getFieldTypeName(columns[i].type));
            for (int s = strlen(getFieldTypeName(columns[i].type)); s < 9; s++) Serial.print(F(" "));
            Serial.print(F(" | "));
            Serial.print(columns[i].offset);
            Serial.print(F("     | "));
            Serial.print(columns[i].length);
            Serial.print(F("    | "));

            if (columns[i].isForeignKey) {
                Serial.print(F("FK -> "));
                Serial.print(columns[i].foreignTable);
                Serial.print(F("."));
                Serial.print(columns[i].foreignColumn);
            } else if (columns[i].isUnique) {
                Serial.print(F("UNIQUE"));
            } else {
                Serial.print(F("-"));
            }
            Serial.println();
        }
        Serial.println(F("-------------------------------------------------------------------------"));
    }
};

#endif // MICRODB_SCHEMA_H
