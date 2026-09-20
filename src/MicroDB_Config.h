#ifndef MICRODB_CONFIG_H
#define MICRODB_CONFIG_H

#include <Arduino.h>
#include <SD.h>

// ============================================================================
// MICRODB - CONFIGURACIÓN DINÁMICA DE HARDWARE, MEMORIA Y MODO DE ARCHIVO
// ============================================================================

// Habilitar diagnósticos y alertas de memoria/rendimiento por puerto Serial (1=Activado, 0=Desactivado)
#ifndef MICRODB_ENABLE_DIAGNOSTICS
    #define MICRODB_ENABLE_DIAGNOSTICS 1
#endif

// Detección dinámica del microcontrolador y ajuste óptimo de memoria RAM
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040) || defined(__SAM3X8E__) || defined(TEENSYDUINO)
    #ifndef MICRODB_MAX_COLUMNS
        #define MICRODB_MAX_COLUMNS      16
    #endif
    #ifndef MICRODB_NAME_LEN
        #define MICRODB_NAME_LEN         16
    #endif
    #ifndef MICRODB_PATH_LEN
        #define MICRODB_PATH_LEN         32
    #endif
    #define MICRODB_CACHE_PAGES          4
    #define MICRODB_MAX_OPEN_TABLES      8
    #define MICRODB_MAX_INDEXES          10
    #define MICRODB_FILE_RW              FILE_WRITE
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    #ifndef MICRODB_MAX_COLUMNS
        #define MICRODB_MAX_COLUMNS      12
    #endif
    #ifndef MICRODB_NAME_LEN
        #define MICRODB_NAME_LEN         12
    #endif
    #ifndef MICRODB_PATH_LEN
        #define MICRODB_PATH_LEN         28
    #endif
    #define MICRODB_CACHE_PAGES          2
    #define MICRODB_MAX_OPEN_TABLES      4
    #define MICRODB_MAX_INDEXES          4
    #define MICRODB_FILE_RW              (FILE_WRITE & ~O_APPEND)
#else
    // AVR estándar de 2 KB SRAM (Arduino Uno, Nano, Pro Mini, Micro, Leonardo - ATmega328P/32U4)
    #ifndef MICRODB_MAX_COLUMNS
        #define MICRODB_MAX_COLUMNS      6
    #endif
    #ifndef MICRODB_NAME_LEN
        #define MICRODB_NAME_LEN         10
    #endif
    #ifndef MICRODB_PATH_LEN
        #define MICRODB_PATH_LEN         28
    #endif
    #define MICRODB_CACHE_PAGES          1
    #define MICRODB_MAX_OPEN_TABLES      2
    #define MICRODB_MAX_INDEXES          2
    #define MICRODB_FILE_RW              (FILE_WRITE & ~O_APPEND)
#endif

// Tamaño de bloque físico estándar de la SD (Sector size)
#define MICRODB_PAGE_SIZE        512

// Magic Numbers para validación de integridad
#define MICRODB_MAGIC_DB         0x4D444231 // "MDB1"
#define MICRODB_MAGIC_TABLE      0x4D544231 // "MTB1"
#define MICRODB_MAGIC_INDEX      0x4D494431 // "MID1"

// Valor para indicar puntero nulo / fin de lista en disco
#define MICRODB_NULL_OFFSET      0xFFFFFFFF

#endif // MICRODB_CONFIG_H
