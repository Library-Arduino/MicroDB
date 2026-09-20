#ifndef MICRODB_TABLE_H
#define MICRODB_TABLE_H

#include <SD.h>
#include "MicroDB_Config.h"
#include "MicroDB_Types.h"
#include "MicroDB_Index.h"
#include "MicroDB_Diagnostics.h"
#include "MicroDB_Schema.h"

// Clase genérica para gestionar una tabla tipada en disco
template <typename T>
class Table {
private:
    char tableName[MICRODB_NAME_LEN + 2];
    char tableFilePath[MICRODB_PATH_LEN];
    char dirPathStored[MICRODB_NAME_LEN + 2];
    TableHeader header;
    bool isOpen;
    size_t slotTotalSize; // sizeof(SlotHeader) + sizeof(T)
    TableSchema schema;

    // Calcula el offset exacto en bytes de un slot en el archivo
    uint32_t getSlotOffset(uint32_t slotIndex) const {
        return header.dataStartOffset + (slotIndex * slotTotalSize);
    }

    // Actualiza la cabecera en el archivo
    bool flushHeader(File& file) {
        file.seek(0);
        size_t written = file.write((const uint8_t*)&header, sizeof(TableHeader));
        file.flush();
        return written == sizeof(TableHeader);
    }

    // Comprueba y alerta si la tabla no está abierta
    bool checkIsOpen(const char* operationName) const {
        if (!isOpen) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ORDEN ERROR] Operacion '"));
            Serial.print(operationName);
            Serial.print(F("' fallida: La tabla '"));
            Serial.print(tableName[0] != '\0' ? tableName : "desconocida");
            Serial.println(F("' NO esta abierta."));
            Serial.println(F("                      -> Flujo correcto: 1. db.begin() -> 2. db.openTable() -> 3. Operaciones."));
            #endif
            return false;
        }
        return true;
    }

public:
    Table() : isOpen(false), slotTotalSize(sizeof(SlotHeader) + sizeof(T)) {
        tableName[0] = '\0';
        tableFilePath[0] = '\0';
        dirPathStored[0] = '\0';
        memset(&header, 0, sizeof(header));
    }

    // Inicializa una tabla nueva y limpia con su cabecera binaria
    bool initCleanTable() {
        if (SD.exists(tableFilePath)) {
            SD.remove(tableFilePath);
        }
        File file = SD.open(tableFilePath, MICRODB_FILE_RW);
        if (!file) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ERROR] No se pudo crear el archivo de tabla '"));
            Serial.print(tableFilePath);
            Serial.println(F("'."));
            #endif
            isOpen = false;
            return false;
        }

        header.magic = MICRODB_MAGIC_TABLE;
        header.version = 1;
        header.recordSize = sizeof(T);
        header.totalSlots = 0;
        header.activeRecords = 0;
        header.deletedRecords = 0;
        header.firstFreeSlot = MICRODB_NULL_OFFSET;
        header.nextAutoId = 1;
        header.dataStartOffset = sizeof(TableHeader);
        memset(header.reserved, 0, sizeof(header.reserved));

        if (!flushHeader(file)) {
            file.close();
            isOpen = false;
            return false;
        }
        file.close();
        isOpen = true;
        return true;
    }

    // [PASO 2] Abre o crea la tabla en el directorio especificado
    bool open(const char* dirPath, const char* name) {
        // Garantizar norma FAT 8.3 (máximo 8 caracteres de nombre base)
        strncpy(tableName, name, 8);
        tableName[8] = '\0';

        const char* d = (dirPath && strlen(dirPath) > 0) ? dirPath : "DB";
        strncpy(dirPathStored, d, 8);
        dirPathStored[8] = '\0';

        snprintf(tableFilePath, sizeof(tableFilePath), "%s/%s.tbl", dirPathStored, tableName);

        slotTotalSize = sizeof(SlotHeader) + sizeof(T);
        schema.init(tableName);

        // Verificación de tamaño de registro y alertas de memoria
        MicroDB_Diagnostics::checkRecordSize(tableName, sizeof(T));

        if (!SD.exists(tableFilePath)) {
            return initCleanTable();
        } else {
            // Cargar cabecera existente
            File file = SD.open(tableFilePath, FILE_READ);
            if (!file) {
                return initCleanTable();
            }
            size_t fileSize = file.size();
            size_t readBytes = file.read((uint8_t*)&header, sizeof(TableHeader));
            file.close();

            // Si el archivo está vacío (0 bytes), incompleto, o cabecera corrupta -> Auto-Recuperación
            if (fileSize < sizeof(TableHeader) || readBytes != sizeof(TableHeader) || header.magic != MICRODB_MAGIC_TABLE) {
                #if MICRODB_ENABLE_DIAGNOSTICS
                Serial.print(F("[MicroDB AUTO-RECUPERACION] Archivo '"));
                Serial.print(tableFilePath);
                Serial.println(F("' vacio o cabecera invalida. Inicializando tabla limpia..."));
                #endif
                return initCleanTable();
            }

            // Si el struct C++ cambió de tamaño (ej: se añadieron campos o cambió el padding) -> Auto-Migración limpia
            if (header.recordSize != sizeof(T)) {
                #if MICRODB_ENABLE_DIAGNOSTICS
                Serial.print(F("[MicroDB INFO STRUCT] La definicion del struct cambio (Guardado: "));
                Serial.print(header.recordSize);
                Serial.print(F("B vs C++ actual: "));
                Serial.print(sizeof(T));
                Serial.println(F("B). Reconfigurando tabla limpia para la nueva estructura..."));
                #endif
                return initCleanTable();
            }
        }

        isOpen = true;
        return true;
    }

    // =========================================================================
    // [PASO 3] METADATOS Y AUTO-DESCUBRIMIENTO DE COLUMNAS (SCHEMA REFLECTION)
    // =========================================================================

    Table<T>& addColumn(const char* colName, uint8_t colType, uint16_t colOffset, uint16_t colLength, bool isUnique = false) {
        if (!checkIsOpen("addColumn")) return *this;
        schema.addColumn(colName, colType, colOffset, colLength, isUnique);
        return *this;
    }

    Table<T>& addUniqueColumn(const char* colName, uint8_t colType, uint16_t colOffset, uint16_t colLength) {
        if (!checkIsOpen("addUniqueColumn")) return *this;
        schema.addColumn(colName, colType, colOffset, colLength, true);
        return *this;
    }

    Table<T>& addForeignKey(const char* colName, uint8_t colType, uint16_t colOffset, uint16_t colLength, const char* refTable, const char* refCol = "id") {
        if (!checkIsOpen("addForeignKey")) return *this;
        schema.addForeignKey(colName, colType, colOffset, colLength, refTable, refCol);
        return *this;
    }

    bool saveSchema() {
        if (!checkIsOpen("saveSchema")) return false;
        return schema.exportJsonSchema(dirPathStored);
    }

    void printSchema() {
        if (!checkIsOpen("printSchema")) return;
        schema.printSchemaToSerial();
    }

    // =========================================================================
    // [PASO 4] OPERACIONES CRUD DIRECTAS O(1)
    // =========================================================================

    uint32_t insert(const T& record) {
        if (!checkIsOpen("insert")) return 0;

        File file = SD.open(tableFilePath, MICRODB_FILE_RW);
        if (!file) return 0;

        uint32_t targetSlot = MICRODB_NULL_OFFSET;
        uint32_t assignedId = header.nextAutoId++;

        if (header.firstFreeSlot != MICRODB_NULL_OFFSET) {
            targetSlot = header.firstFreeSlot;
            uint32_t offset = getSlotOffset(targetSlot);
            file.seek(offset);

            SlotHeader oldSlot;
            if (file.read((uint8_t*)&oldSlot, sizeof(SlotHeader)) == sizeof(SlotHeader)) {
                header.firstFreeSlot = oldSlot.nextFreeSlot;
            } else {
                header.firstFreeSlot = MICRODB_NULL_OFFSET;
            }
            header.deletedRecords--;
        } else {
            targetSlot = header.totalSlots++;
        }

        SlotHeader slotHeader;
        slotHeader.status = RECORD_ACTIVE;
        slotHeader.recordId = assignedId;
        slotHeader.nextFreeSlot = MICRODB_NULL_OFFSET;

        uint32_t offset = getSlotOffset(targetSlot);
        file.seek(offset);
        file.write((const uint8_t*)&slotHeader, sizeof(SlotHeader));
        file.write((const uint8_t*)&record, sizeof(T));

        header.activeRecords++;
        
        flushHeader(file);
        file.close();

        return assignedId;
    }

    // =========================================================================
    // VALORES ÚNICOS (UNIQUE CONSTRAINTS) Y UPSERT
    // =========================================================================

    template <typename KeyExtractor>
    uint32_t insertUnique(const T& record, KeyExtractor getKey) {
        if (!checkIsOpen("insertUnique")) return 0;
        auto keyVal = getKey(record);
        bool duplicateFound = false;

        forEach([&](uint32_t id, const T& existing) {
            if (getKey(existing) == keyVal) {
                duplicateFound = true;
            }
        });

        if (duplicateFound) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB UNIQUE ERROR] Tabla '"));
            Serial.print(tableName);
            Serial.println(F("': Violacion de unicidad. El valor clave numerico ya existe."));
            #endif
            return 0;
        }
        return insert(record);
    }

    template <typename StringExtractor>
    uint32_t insertUniqueString(const T& record, StringExtractor getStringKey) {
        if (!checkIsOpen("insertUniqueString")) return 0;
        const char* targetStr = getStringKey(record);
        bool duplicateFound = false;

        forEach([&](uint32_t id, const T& existing) {
            const char* existingStr = getStringKey(existing);
            if (targetStr && existingStr && strcmp(targetStr, existingStr) == 0) {
                duplicateFound = true;
            }
        });

        if (duplicateFound) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB UNIQUE ERROR] Tabla '"));
            Serial.print(tableName);
            Serial.print(F("': Violacion de unicidad. La cadena '"));
            Serial.print(targetStr);
            Serial.println(F("' ya existe en la base de datos."));
            #endif
            return 0;
        }
        return insert(record);
    }

    template <typename KeyExtractor>
    uint32_t upsertUnique(const T& record, KeyExtractor getKey) {
        if (!checkIsOpen("upsertUnique")) return 0;
        auto keyVal = getKey(record);
        uint32_t existingId = 0;

        forEach([&](uint32_t id, const T& existing) {
            if (getKey(existing) == keyVal) {
                existingId = id;
            }
        });

        if (existingId != 0) {
            update(existingId, record);
            return existingId;
        } else {
            return insert(record);
        }
    }

    template <typename StringExtractor>
    uint32_t upsertUniqueString(const T& record, StringExtractor getStringKey) {
        if (!checkIsOpen("upsertUniqueString")) return 0;
        const char* targetStr = getStringKey(record);
        uint32_t existingId = 0;

        forEach([&](uint32_t id, const T& existing) {
            const char* existingStr = getStringKey(existing);
            if (targetStr && existingStr && strcmp(targetStr, existingStr) == 0) {
                existingId = id;
            }
        });

        if (existingId != 0) {
            update(existingId, record);
            return existingId;
        } else {
            return insert(record);
        }
    }

    // =========================================================================
    // RESTRICCIONES (CHECK) E INTEGRIDAD REFERENCIAL (FOREIGN KEYS)
    // =========================================================================

    template <typename Validator>
    uint32_t insertIf(const T& record, Validator validator) {
        if (!checkIsOpen("insertIf")) return 0;
        if (!validator(record)) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB CHECK ERROR] Tabla '"));
            Serial.print(tableName);
            Serial.println(F("': Restriccion violada. Registro rechazado."));
            #endif
            return 0;
        }
        return insert(record);
    }

    template <typename Validator>
    bool updateIf(uint32_t recordId, const T& updatedRecord, Validator validator) {
        if (!checkIsOpen("updateIf")) return false;
        if (!validator(updatedRecord)) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB CHECK ERROR] Tabla '"));
            Serial.print(tableName);
            Serial.println(F("': Restriccion violada en UPDATE. Operacion rechazada."));
            #endif
            return false;
        }
        return update(recordId, updatedRecord);
    }

    template <typename TParent>
    uint32_t insertWithFK(const T& record, Table<TParent>& parentTable, uint32_t foreignKeyId) {
        if (!checkIsOpen("insertWithFK")) return 0;
        if (!parentTable.isTableOpen()) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ORDEN ERROR] La tabla padre '"));
            Serial.print(parentTable.getName());
            Serial.println(F("' NO esta abierta al validar la Clave Foranea."));
            #endif
            return 0;
        }

        TParent dummyParent;
        if (!parentTable.getById(foreignKeyId, dummyParent)) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB FK ERROR] Violacion de Clave Foranea en '"));
            Serial.print(tableName);
            Serial.print(F("': El ID padre #"));
            Serial.print(foreignKeyId);
            Serial.print(F(" NO existe en la tabla '"));
            Serial.print(parentTable.getName());
            Serial.println(F("'. Insercion abortada."));
            #endif
            return 0;
        }
        return insert(record);
    }

    template <typename TChild, typename KeyExtractor>
    bool removeRestrict(uint32_t parentId, Table<TChild>& childTable, KeyExtractor getForeignKey) {
        if (!checkIsOpen("removeRestrict")) return false;
        bool hasChildren = false;
        childTable.forEach([&](uint32_t childId, const TChild& child) {
            if (getForeignKey(child) == parentId) {
                hasChildren = true;
            }
        });

        if (hasChildren) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB RESTRICT ERROR] No se puede eliminar el ID #"));
            Serial.print(parentId);
            Serial.print(F(" de '"));
            Serial.print(tableName);
            Serial.println(F("': Existen registros dependientes en la tabla hija."));
            #endif
            return false;
        }
        return remove(parentId);
    }

    template <typename TChild, typename KeyExtractor>
    uint32_t removeCascade(uint32_t parentId, Table<TChild>& childTable, KeyExtractor getForeignKey) {
        if (!checkIsOpen("removeCascade")) return 0;
        uint32_t deletedChildren = 0;
        childTable.forEach([&](uint32_t childId, const TChild& child) {
            if (getForeignKey(child) == parentId) {
                childTable.remove(childId);
                deletedChildren++;
            }
        });
        remove(parentId);
        return deletedChildren;
    }

    // =========================================================================
    // MANTENIMIENTO Y COMPACTACIÓN (VACUUM)
    // =========================================================================

    bool vacuum() {
        if (!checkIsOpen("vacuum")) return false;
        if (header.deletedRecords == 0) return true;

        char tmpPath[36];
        snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", tableFilePath);

        if (SD.exists(tmpPath)) {
            SD.remove(tmpPath);
        }

        File tmpFile = SD.open(tmpPath, MICRODB_FILE_RW);
        if (!tmpFile) return false;

        TableHeader newHeader = header;
        newHeader.totalSlots = 0;
        newHeader.activeRecords = 0;
        newHeader.deletedRecords = 0;
        newHeader.firstFreeSlot = MICRODB_NULL_OFFSET;
        newHeader.dataStartOffset = sizeof(TableHeader);

        tmpFile.write((const uint8_t*)&newHeader, sizeof(TableHeader));

        forEach([&](uint32_t id, const T& rec) {
            SlotHeader sh;
            sh.status = RECORD_ACTIVE;
            sh.recordId = id;
            sh.nextFreeSlot = MICRODB_NULL_OFFSET;

            tmpFile.write((const uint8_t*)&sh, sizeof(SlotHeader));
            tmpFile.write((const uint8_t*)&rec, sizeof(T));
            newHeader.totalSlots++;
            newHeader.activeRecords++;
        });

        tmpFile.seek(0);
        tmpFile.write((const uint8_t*)&newHeader, sizeof(TableHeader));
        tmpFile.flush();
        tmpFile.close();

        SD.remove(tableFilePath);
        
        File src = SD.open(tmpPath, FILE_READ);
        File dst = SD.open(tableFilePath, MICRODB_FILE_RW);
        if (src && dst) {
            uint8_t copyBuf[64];
            while (src.available()) {
                size_t n = src.read(copyBuf, sizeof(copyBuf));
                dst.write(copyBuf, n);
            }
            dst.flush();
            src.close();
            dst.close();
            SD.remove(tmpPath);
            
            header = newHeader;
            return true;
        }
        return false;
    }

    // =========================================================================
    // [PASO 5] CONSULTAS DIRECTAS Y STREAMING O(1) RAM
    // =========================================================================

    bool getBySlot(uint32_t slotIndex, T& outRecord, uint32_t& outRecordId) {
        if (!checkIsOpen("getBySlot") || slotIndex >= header.totalSlots) return false;

        File file = SD.open(tableFilePath, FILE_READ);
        if (!file) return false;

        file.seek(getSlotOffset(slotIndex));
        SlotHeader slotHeader;
        if (file.read((uint8_t*)&slotHeader, sizeof(SlotHeader)) != sizeof(SlotHeader)) {
            file.close();
            return false;
        }

        if (slotHeader.status != RECORD_ACTIVE) {
            file.close();
            return false;
        }

        if (file.read((uint8_t*)&outRecord, sizeof(T)) != sizeof(T)) {
            file.close();
            return false;
        }

        outRecordId = slotHeader.recordId;
        file.close();
        return true;
    }

    bool getById(uint32_t recordId, T& outRecord, uint32_t* outSlotIndex = nullptr) {
        if (!checkIsOpen("getById") || header.activeRecords == 0) return false;

        File file = SD.open(tableFilePath, FILE_READ);
        if (!file) return false;

        SlotHeader slotHeader;
        for (uint32_t slot = 0; slot < header.totalSlots; slot++) {
            file.seek(getSlotOffset(slot));
            if (file.read((uint8_t*)&slotHeader, sizeof(SlotHeader)) != sizeof(SlotHeader)) break;

            if (slotHeader.status == RECORD_ACTIVE && slotHeader.recordId == recordId) {
                if (file.read((uint8_t*)&outRecord, sizeof(T)) == sizeof(T)) {
                    if (outSlotIndex) *outSlotIndex = slot;
                    file.close();
                    return true;
                }
            }
        }

        file.close();
        return false;
    }

    template <typename Predicate>
    bool findFirst(Predicate predicate, T& outRecord, uint32_t& outRecordId) {
        if (!checkIsOpen("findFirst") || header.activeRecords == 0) return false;

        File file = SD.open(tableFilePath, FILE_READ);
        if (!file) return false;

        SlotHeader slotHeader;
        T recordBuffer;

        for (uint32_t slot = 0; slot < header.totalSlots; slot++) {
            file.seek(getSlotOffset(slot));
            if (file.read((uint8_t*)&slotHeader, sizeof(SlotHeader)) != sizeof(SlotHeader)) break;

            if (slotHeader.status == RECORD_ACTIVE) {
                if (file.read((uint8_t*)&recordBuffer, sizeof(T)) == sizeof(T)) {
                    if (predicate(slotHeader.recordId, recordBuffer)) {
                        outRecord = recordBuffer;
                        outRecordId = slotHeader.recordId;
                        file.close();
                        return true;
                    }
                }
            }
        }

        file.close();
        return false;
    }

    bool update(uint32_t recordId, const T& updatedRecord) {
        if (!checkIsOpen("update")) return false;

        uint32_t slotIndex = 0;
        T dummy;
        if (!getById(recordId, dummy, &slotIndex)) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ERROR] update(): Registro con ID #"));
            Serial.print(recordId);
            Serial.println(F(" no encontrado."));
            #endif
            return false;
        }

        File file = SD.open(tableFilePath, MICRODB_FILE_RW);
        if (!file) return false;

        file.seek(getSlotOffset(slotIndex) + sizeof(SlotHeader));
        size_t written = file.write((const uint8_t*)&updatedRecord, sizeof(T));
        file.flush();
        file.close();

        return written == sizeof(T);
    }

    bool remove(uint32_t recordId) {
        if (!checkIsOpen("remove")) return false;

        uint32_t slotIndex = 0;
        T dummy;
        if (!getById(recordId, dummy, &slotIndex)) {
            #if MICRODB_ENABLE_DIAGNOSTICS
            Serial.print(F("[MicroDB ERROR] remove(): Registro con ID #"));
            Serial.print(recordId);
            Serial.println(F(" no existe para eliminar."));
            #endif
            return false;
        }

        File file = SD.open(tableFilePath, MICRODB_FILE_RW);
        if (!file) return false;

        SlotHeader slotHeader;
        slotHeader.status = RECORD_DELETED;
        slotHeader.recordId = 0;
        slotHeader.nextFreeSlot = header.firstFreeSlot;

        file.seek(getSlotOffset(slotIndex));
        file.write((const uint8_t*)&slotHeader, sizeof(SlotHeader));

        header.firstFreeSlot = slotIndex;
        header.activeRecords--;
        header.deletedRecords++;

        flushHeader(file);
        file.close();
        return true;
    }

    template <typename Callback>
    void forEach(Callback callback) {
        if (!checkIsOpen("forEach") || header.activeRecords == 0) return;

        MicroDB_Diagnostics::checkScanPerformance(tableName, header.activeRecords);

        File file = SD.open(tableFilePath, FILE_READ);
        if (!file) return;

        SlotHeader slotHeader;
        T recordBuffer;

        for (uint32_t slot = 0; slot < header.totalSlots; slot++) {
            file.seek(getSlotOffset(slot));
            if (file.read((uint8_t*)&slotHeader, sizeof(SlotHeader)) != sizeof(SlotHeader)) break;

            if (slotHeader.status == RECORD_ACTIVE) {
                if (file.read((uint8_t*)&recordBuffer, sizeof(T)) == sizeof(T)) {
                    callback(slotHeader.recordId, recordBuffer);
                }
            }
        }

        file.close();
    }

    template <typename Predicate, typename Callback>
    void where(Predicate predicate, Callback callback) {
        if (!checkIsOpen("where")) return;
        forEach([&](uint32_t id, const T& rec) {
            if (predicate(id, rec)) {
                callback(id, rec);
            }
        });
    }

    template <typename Predicate>
    uint32_t countWhere(Predicate predicate) {
        if (!checkIsOpen("countWhere")) return 0;
        uint32_t count = 0;
        forEach([&](uint32_t id, const T& rec) {
            if (predicate(id, rec)) {
                count++;
            }
        });
        return count;
    }

    bool truncate() {
        if (!checkIsOpen("truncate")) return false;
        SD.remove(tableFilePath);
        isOpen = false;
        
        char dir[16];
        char* lastSlash = strrchr(tableFilePath, '/');
        if (lastSlash) {
            size_t len = lastSlash - tableFilePath;
            strncpy(dir, tableFilePath, len);
            dir[len] = '\0';
            return open(dir, tableName);
        } else {
            return open("", tableName);
        }
    }

    uint32_t count() const { return isOpen ? header.activeRecords : 0; }
    uint32_t deletedCount() const { return isOpen ? header.deletedRecords : 0; }
    uint32_t totalSlots() const { return isOpen ? header.totalSlots : 0; }
    const char* getName() const { return tableName; }
    bool isTableOpen() const { return isOpen; }
};

#endif // MICRODB_TABLE_H
