#include "MagellanDecoder.h"

// Раскомментируй следующую строку, чтобы включить подробный вывод в Serial
// #define DEBUG_MAGELLAN

#ifdef DEBUG_MAGELLAN
#define MAGELLAN_DEB_PRINT(...) Serial.print(__VA_ARGS__)
#define MAGELLAN_DEB_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define MAGELLAN_DEB_PRINT(...)
#define MAGELLAN_DEB_PRINTLN(...)
#endif

// --- Внутренние переменные модуля ---
enum MagellanState {
    MAGELLAN_RESET,
    MAGELLAN_CHECK_PREAMBLE_PULSE,
    MAGELLAN_CHECK_PREAMBLE_PAUSE,
    MAGELLAN_FOUND_PREAMBLE,
    MAGELLAN_SAVE_DURATION,
    MAGELLAN_CHECK_DURATION
};
volatile MagellanState magellanState = MAGELLAN_RESET;
volatile uint32_t magellanRawData = 0;
volatile uint8_t magellanBitCount = 0;
volatile uint16_t magellanHeaderCount = 0;
volatile uint32_t magellanLastDuration = 0;

// Константы
#define MAGELLAN_TE_SHORT 200
#define MAGELLAN_TE_LONG 400
#define MAGELLAN_TE_DELTA 120

// --- Вспомогательные функции ---
static bool is_duration_magellan(uint32_t duration, uint8_t multiplier) {
    uint32_t base = MAGELLAN_TE_SHORT * multiplier;
    uint32_t delta = MAGELLAN_TE_DELTA * multiplier;
    return (duration > base - delta) && (duration < base + delta);
}

static uint8_t crc8_magellan(uint8_t* data, size_t len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
        }
    }
    return crc;
}

static bool check_magellan_crc(uint32_t data) {
    uint8_t data_for_crc[3] = {(uint8_t)(data >> 24), (uint8_t)(data >> 16), (uint8_t)(data >> 8)};
    uint8_t received_crc = data & 0xFF;
    uint8_t calculated_crc = crc8_magellan(data_for_crc, 3);
    MAGELLAN_DEB_PRINT(" -> CRC check. Calculated: 0x"); MAGELLAN_DEB_PRINT(calculated_crc, HEX);
    MAGELLAN_DEB_PRINT(", Received: 0x"); MAGELLAN_DEB_PRINTLN(received_crc, HEX);
    return received_crc == calculated_crc;
}

// --- Главная функция-обработчик с отладкой ---
void processMagellanSignal(bool level, uint32_t duration) {
    MAGELLAN_DEB_PRINT("Pulse L:"); MAGELLAN_DEB_PRINT(level); MAGELLAN_DEB_PRINT(" D:"); MAGELLAN_DEB_PRINT(duration);
    MAGELLAN_DEB_PRINT(" | State: "); MAGELLAN_DEB_PRINTLN(magellanState);

    switch (magellanState) {
    case MAGELLAN_RESET:
        // Шаг 1: Ищем первый короткий ВЫСОКИЙ импульс преамбулы
        if (level && is_duration_magellan(duration, 1)) {
            magellanState = MAGELLAN_CHECK_PREAMBLE_PAUSE;
            magellanHeaderCount = 0;
            MAGELLAN_DEB_PRINTLN(" -> DEBUG: Got 1st preamble pulse -> CHECK_PREAMBLE_PAUSE");
        }
        break;

    case MAGELLAN_CHECK_PREAMBLE_PAUSE:
        if (!level) {
            // Вариант А: это короткая пауза, часть обычной пары
            if (is_duration_magellan(duration, 1)) {
                magellanHeaderCount++;
                magellanState = MAGELLAN_CHECK_PREAMBLE_PULSE; // Ждем следующий высокий
                MAGELLAN_DEB_PRINT(" -> DEBUG: Preamble pair found, count: "); MAGELLAN_DEB_PRINTLN(magellanHeaderCount);
            // Вариант Б: это длинная пауза, конец преамбулы
            } else if (magellanHeaderCount > 10 && is_duration_magellan(duration, 2)) {
                MAGELLAN_DEB_PRINTLN(" -> DEBUG: Preamble end sequence OK -> FOUND_PREAMBLE");
                magellanState = MAGELLAN_FOUND_PREAMBLE;
            } else {
                 MAGELLAN_DEB_PRINTLN(" -> DEBUG: Wrong preamble pause, resetting.");
                magellanState = MAGELLAN_RESET;
            }
        } else {
             MAGELLAN_DEB_PRINTLN(" -> DEBUG: Expected LOW, got HIGH, resetting.");
            magellanState = MAGELLAN_RESET;
        }
        break;
    
    case MAGELLAN_CHECK_PREAMBLE_PULSE:
        if (level && is_duration_magellan(duration, 1)) {
            magellanState = MAGELLAN_CHECK_PREAMBLE_PAUSE; // Готовимся к следующей паузе
        } else {
             MAGELLAN_DEB_PRINTLN(" -> DEBUG: Wrong preamble pulse, resetting.");
            magellanState = MAGELLAN_RESET;
        }
        break;

    case MAGELLAN_FOUND_PREAMBLE:
         // Шаг 3: Ждем стартовый бит (очень длинный импульс + длинная пауза)
        if (level) {
            magellanLastDuration = duration;
        } else {
            if (is_duration_magellan(magellanLastDuration, 6) && is_duration_magellan(duration, 2)) {
                MAGELLAN_DEB_PRINTLN(" -> DEBUG: Start bit OK -> SAVE_DURATION");
                magellanRawData = 0;
                magellanBitCount = 0;
                magellanState = MAGELLAN_SAVE_DURATION;
            } else {
                MAGELLAN_DEB_PRINTLN(" -> DEBUG: Wrong start bit, resetting.");
                magellanState = MAGELLAN_RESET;
            }
        }
        break;
    
    case MAGELLAN_SAVE_DURATION:
        if (level) {
            magellanLastDuration = duration;
            magellanState = MAGELLAN_CHECK_DURATION;
        } else {
            magellanState = MAGELLAN_RESET;
        }
        break;

    case MAGELLAN_CHECK_DURATION:
        if (!level) {
            if (duration > MAGELLAN_TE_LONG * 3) {
                 MAGELLAN_DEB_PRINT(" -> DEBUG: End of packet detected. Bits: "); MAGELLAN_DEB_PRINTLN(magellanBitCount);
                 if (magellanBitCount == 32 && check_magellan_crc(magellanRawData)) {
                    MAGELLAN_DEB_PRINTLN("SUCCESS! Capturing data.");
                    barrierCodeMain = magellanRawData >> 8; 
                    barrierBit = magellanRawData & 0xFF;
                    barrierProtocol = 20;
                    newSignalReady = true;
                }
                magellanState = MAGELLAN_RESET;
                break;
            }

            bool bit_val;
            bool success = false;
            if (is_duration_magellan(magellanLastDuration, 1) && is_duration_magellan(duration, 2)) {
                bit_val = 1; success = true;
            } else if (is_duration_magellan(magellanLastDuration, 2) && is_duration_magellan(duration, 1)) {
                bit_val = 0; success = true;
            }

            if (success) {
                MAGELLAN_DEB_PRINT(bit_val);
                if(magellanBitCount < 32) {
                    magellanRawData = (magellanRawData << 1) | bit_val;
                    magellanBitCount++;
                }
                magellanState = MAGELLAN_SAVE_DURATION;
            } else {
                MAGELLAN_DEB_PRINTLN("\n -> Bit timing error, resetting.");
                magellanState = MAGELLAN_RESET;
            }
        } else {
            magellanState = MAGELLAN_RESET;
        }
        break;
    }
}