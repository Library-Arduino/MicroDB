#ifndef MICRODB_TYPES_H
#define MICRODB_TYPES_H

#include "MicroDB_Config.h"

// Estados de un registro en disco
enum RecordStatus : uint8_t {
    RECORD_DELETED = 0x00,
    RECORD_ACTIVE  = 0x01
};

// Códigos de resultado de operaciones
enum DBResult : uint8_t {
    DB_SUCCESS = 0,
    DB_ERROR_FILE_NOT_FOUND,
    DB_ERROR_SD_INIT,
    DB_ERROR_CORRUPTED,
    DB_ERROR_OUT_OF_MEMORY,
    DB_ERROR_RECORD_NOT_FOUND,
    DB_ERROR_INDEX_FULL,
    DB_ERROR_KEY_EXISTS,
    DB_ERROR_INVALID_PARAM,
    DB_ERROR_IO
};

// Operadores para consultas relacionales y filtros
enum QueryOp : uint8_t {
    OP_EQ = 0,  // Igual (=)
    OP_NE,      // No igual (!=)
    OP_GT,      // Mayor que (>)
    OP_GTE,     // Mayor o igual (>=)
    OP_LT,      // Menor que (<)
    OP_LTE,     // Menor o igual (<=)
    OP_STR_CONTAINS, // Contiene subcadena
    OP_STR_STARTS_WITH
};

// Tipos de datos soportados para indexación, comparación y metadatos
enum FieldType : uint8_t {
    TYPE_BOOL = 0,
    TYPE_INT8,
    TYPE_UINT8,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BLOB
};

// Cabecera de un archivo de tabla en disco (64 bytes exactos)
struct TableHeader {
    uint32_t magic;               // 0x4D544231 ("MTB1")
    uint16_t version;             // Versión del motor
    uint16_t recordSize;          // Tamaño en bytes del payload del registro (sin header del slot)
    uint32_t totalSlots;          // Total de slots reservados en archivo
    uint32_t activeRecords;       // Cantidad de registros activos (no borrados)
    uint32_t deletedRecords;      // Cantidad de registros marcados como borrados
    uint32_t firstFreeSlot;       // Índice de slot libre para reciclaje (Free-list stack head, 0xFFFFFFFF = none)
    uint32_t nextAutoId;          // Próximo ID autoincremental
    uint32_t dataStartOffset;     // Offset en bytes donde inician los registros (ej. 64)
    uint8_t  reserved[32];        // Espacio para expansión futura
};

// Cabecera por cada Slot de registro individual en disco
// Tamaño: 1 + 4 + 4 = 9 bytes
struct SlotHeader {
    uint8_t  status;              // RECORD_ACTIVE (0x01) o RECORD_DELETED (0x00)
    uint32_t recordId;            // ID único autoincremental
    uint32_t nextFreeSlot;        // Puntero enlazado para la lista de reutilización de slots vacíos
};

// Entrada en un índice secundario simple
struct IndexEntry {
    uint32_t keyHash;             // Hash de clave o valor numérico convertido
    uint32_t recordId;            // ID del registro asociado en la tabla principal
    uint32_t slotIndex;           // Slot físico en el archivo .tbl
};

#endif // MICRODB_TYPES_H
