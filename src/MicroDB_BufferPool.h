#ifndef MICRODB_BUFFERPOOL_H
#define MICRODB_BUFFERPOOL_H

#include <SD.h>
#include "MicroDB_Types.h"

// Manejador de buffer de página con soporte de streaming
class MicroDB_PageCache {
private:
    uint8_t buffer[MICRODB_PAGE_SIZE];
    uint32_t currentSector;
    bool isDirty;
    bool isValid;
    char openFilePath[64];

public:
    MicroDB_PageCache() : currentSector(MICRODB_NULL_OFFSET), isDirty(false), isValid(false) {
        openFilePath[0] = '\0';
    }

    // Limpia la caché y libera el estado
    void clear() {
        isDirty = false;
        isValid = false;
        currentSector = MICRODB_NULL_OFFSET;
        openFilePath[0] = '\0';
    }

    // Lee un bloque de datos con alineación
    bool readSector(File& file, uint32_t sector) {
        if (isValid && currentSector == sector) {
            return true; // Cache hit
        }

        if (isDirty) {
            flush(file);
        }

        file.seek(sector * MICRODB_PAGE_SIZE);
        size_t readBytes = file.read(buffer, MICRODB_PAGE_SIZE);
        if (readBytes > 0) {
            currentSector = sector;
            isValid = true;
            isDirty = false;
            return true;
        }
        return false;
    }

    // Escribe datos en la página de caché
    bool writeSector(File& file, uint32_t sector, const uint8_t* srcData, size_t offsetInPage, size_t length) {
        if (!readSector(file, sector)) {
            // Si es un sector nuevo al final del archivo, rellenar en cero
            memset(buffer, 0, MICRODB_PAGE_SIZE);
            currentSector = sector;
            isValid = true;
        }

        if (offsetInPage + length <= MICRODB_PAGE_SIZE) {
            memcpy(buffer + offsetInPage, srcData, length);
            isDirty = true;
            return true;
        }
        return false;
    }

    // Guarda los cambios pendientes al archivo físico
    bool flush(File& file) {
        if (isValid && isDirty && currentSector != MICRODB_NULL_OFFSET) {
            file.seek(currentSector * MICRODB_PAGE_SIZE);
            size_t written = file.write(buffer, MICRODB_PAGE_SIZE);
            file.flush();
            isDirty = false;
            return written == MICRODB_PAGE_SIZE;
        }
        return true;
    }

    uint8_t* getBuffer() { return buffer; }
    uint32_t getSector() const { return currentSector; }
};

#endif // MICRODB_BUFFERPOOL_H
