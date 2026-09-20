#ifndef MICRODB_INDEX_H
#define MICRODB_INDEX_H

#include <SD.h>
#include "MicroDB_Config.h"
#include "MicroDB_Types.h"

// Cabecera del archivo de Índice (32 bytes)
struct IndexHeader {
    uint32_t magic;         // 0x4D494431 ("MID1")
    uint16_t version;
    uint16_t entrySize;     // sizeof(IndexEntry) = 12 bytes
    uint32_t totalEntries;
    uint32_t isSorted;      // 1 si está ordenado para búsqueda binaria O(log N)
    uint8_t  reserved[16];
};

class MicroDB_Index {
private:
    char filePath[32];
    IndexHeader header;
    bool isLoaded;

public:
    MicroDB_Index() : isLoaded(false) {
        filePath[0] = '\0';
        memset(&header, 0, sizeof(header));
    }

    // Algoritmo de Hashing FNV-1a de 32 bits ultrarrápido para cadenas
    static uint32_t hashString(const char* str) {
        uint32_t hash = 2166136261u;
        if (!str) return hash;
        while (*str) {
            hash ^= (uint8_t)(*str);
            hash *= 16777619u;
            str++;
        }
        return hash;
    }

    bool initCleanIndex() {
        if (SD.exists(filePath)) {
            SD.remove(filePath);
        }
        File file = SD.open(filePath, MICRODB_FILE_RW);
        if (!file) {
            isLoaded = false;
            return false;
        }

        header.magic = MICRODB_MAGIC_INDEX;
        header.version = 1;
        header.entrySize = sizeof(IndexEntry);
        header.totalEntries = 0;
        header.isSorted = 1;
        memset(header.reserved, 0, sizeof(header.reserved));

        file.seek(0);
        file.write((const uint8_t*)&header, sizeof(header));
        file.flush();
        file.close();
        isLoaded = true;
        return true;
    }

    bool open(const char* path) {
        strncpy(filePath, path, sizeof(filePath) - 1);
        filePath[sizeof(filePath) - 1] = '\0';

        if (!SD.exists(filePath)) {
            return initCleanIndex();
        } else {
            // Leer cabecera existente
            File file = SD.open(filePath, FILE_READ);
            if (!file) return initCleanIndex();
            size_t fileSize = file.size();
            size_t r = file.read((uint8_t*)&header, sizeof(header));
            file.close();
            if (fileSize < sizeof(header) || r != sizeof(header) || header.magic != MICRODB_MAGIC_INDEX) {
                return initCleanIndex();
            }
        }

        isLoaded = true;
        return true;
    }

    // Inserción de entrada en el índice
    bool insert(uint32_t key, uint32_t recordId, uint32_t slotIndex) {
        if (!isLoaded) return false;

        File file = SD.open(filePath, MICRODB_FILE_RW);
        if (!file) return false;

        IndexEntry entry = { key, recordId, slotIndex };
        
        // Escribir al final
        file.seek(sizeof(IndexHeader) + (header.totalEntries * sizeof(IndexEntry)));
        size_t written = file.write((const uint8_t*)&entry, sizeof(IndexEntry));
        if (written != sizeof(IndexEntry)) {
            file.close();
            return false;
        }

        header.totalEntries++;
        
        // Actualizar cabecera en offset 0
        file.seek(0);
        file.write((const uint8_t*)&header, sizeof(header));
        file.flush();
        file.close();
        return true;
    }

    // Búsqueda en el índice (Búsqueda binaria si está ordenado, o escaneo directo)
    bool findFirst(uint32_t key, uint32_t& outRecordId, uint32_t& outSlotIndex) {
        if (!isLoaded || header.totalEntries == 0) return false;

        File file = SD.open(filePath, FILE_READ);
        if (!file) return false;

        // Búsqueda binaria en disco O(log N) con bajo consumo de RAM
        int32_t left = 0;
        int32_t right = (int32_t)header.totalEntries - 1;
        IndexEntry entry;

        while (left <= right) {
            int32_t mid = left + (right - left) / 2;
            uint32_t offset = sizeof(IndexHeader) + (mid * sizeof(IndexEntry));
            file.seek(offset);
            if (file.read((uint8_t*)&entry, sizeof(IndexEntry)) != sizeof(IndexEntry)) {
                break;
            }

            if (entry.keyHash == key) {
                outRecordId = entry.recordId;
                outSlotIndex = entry.slotIndex;
                file.close();
                return true;
            } else if (entry.keyHash < key) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        // Si no está completamente ordenado o hay colisión, fallback a escaneo secuencial
        file.seek(sizeof(IndexHeader));
        for (uint32_t i = 0; i < header.totalEntries; i++) {
            if (file.read((uint8_t*)&entry, sizeof(IndexEntry)) != sizeof(IndexEntry)) break;
            if (entry.keyHash == key) {
                outRecordId = entry.recordId;
                outSlotIndex = entry.slotIndex;
                file.close();
                return true;
            }
        }

        file.close();
        return false;
    }

    uint32_t count() const { return isLoaded ? header.totalEntries : 0; }
};

#endif // MICRODB_INDEX_H
