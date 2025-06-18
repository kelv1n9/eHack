#include "DooyaDecoder.h"

// --- Внутренние переменные модуля ---
enum DooyaState {
    DOOYA_RESET,
    DOOYA_WAIT_SYNC_PULSE,
    DOOYA_WAIT_SYNC_PAUSE,
    DOOYA_SAVE_DURATION,
    DOOYA_CHECK_DURATION
};
volatile DooyaState dooyaState = DOOYA_RESET;
volatile uint64_t dooyaRawData = 0;
volatile uint8_t dooyaBitCount = 0;
volatile uint32_t dooyaLastDuration = 0;

// Константы таймингов
#define DOOYA_TE_SHORT 366
#define DOOYA_TE_LONG 733
#define DOOYA_TE_DELTA 150

// --- Вспомогательная функция для проверки длительности ---
static bool is_duration_dooya(uint32_t duration, uint8_t multiplier) {
    uint32_t base = DOOYA_TE_SHORT * multiplier;
    uint32_t delta = (multiplier < 3) ? (DOOYA_TE_DELTA * 2) : (DOOYA_TE_DELTA * multiplier);
    return (duration > base - delta) && (duration < base + delta);
}

// Разбор пакета и заполнение глобальных переменных
static void parse_dooya_packet(uint64_t packet) {
    barrierCodeMain = (packet >> 16);
    barrierCodeAdd = (packet >> 8) & 0xFF;
    barrierBit = packet & 0xFF;
}

// --- Главная функция-обработчик ---
void processDooyaSignal(bool level, uint32_t duration) {
    switch (dooyaState) {
    case DOOYA_RESET:
        if (!level && (duration > (DOOYA_TE_LONG * 11) && duration < (DOOYA_TE_LONG * 14) )) {
            dooyaState = DOOYA_WAIT_SYNC_PULSE;
        }
        break;

    case DOOYA_WAIT_SYNC_PULSE:
        if (level && is_duration_dooya(duration, 13)) {
            dooyaState = DOOYA_WAIT_SYNC_PAUSE;
        } else {
            dooyaState = DOOYA_RESET;
        }
        break;

    case DOOYA_WAIT_SYNC_PAUSE:
        if (!level && is_duration_dooya(duration, 4)) {
            dooyaRawData = 0;
            dooyaBitCount = 0;
            dooyaState = DOOYA_SAVE_DURATION;
        } else {
            dooyaState = DOOYA_RESET;
        }
        break;
    
    case DOOYA_SAVE_DURATION:
        if (level) {
            dooyaLastDuration = duration;
            dooyaState = DOOYA_CHECK_DURATION;
        } else {
            dooyaState = DOOYA_RESET;
        }
        break;

    case DOOYA_CHECK_DURATION:
        if (!level) {
            bool bit_val;
            bool success = false;
            
            // ИСПРАВЛЕНО: Сначала проверяем на конец пакета (длинная пауза)
            if (duration >= (DOOYA_TE_LONG * 4)) {
                if (is_duration_dooya(dooyaLastDuration, 2)) { // длинный HIGH
                    bit_val = 1; success = true;
                } else if(is_duration_dooya(dooyaLastDuration, 1)) { // короткий HIGH
                    bit_val = 0; success = true;
                }

                if(success) {
                    if (dooyaBitCount < 40) {
                        dooyaRawData = (dooyaRawData << 1) | bit_val;
                        dooyaBitCount++;
                    }
                }
                
                if (dooyaBitCount == 40) {
                    parse_dooya_packet(dooyaRawData);
                    barrierProtocol = 26;
                    newSignalReady = true;
                }
                dooyaState = DOOYA_RESET;
                break;
            }
            
            // Если это не конец, декодируем обычный бит
            if (is_duration_dooya(dooyaLastDuration, 2) && is_duration_dooya(duration, 1)) {
                bit_val = 1; success = true;
            } else if (is_duration_dooya(dooyaLastDuration, 1) && is_duration_dooya(duration, 2)) {
                bit_val = 0; success = true;
            }

            if (success) {
                if (dooyaBitCount < 40) {
                    dooyaRawData = (dooyaRawData << 1) | bit_val;
                    dooyaBitCount++;
                }
                dooyaState = DOOYA_SAVE_DURATION;
            } else {
                dooyaState = DOOYA_RESET;
            }
        } else {
            dooyaState = DOOYA_RESET;
        }
        break;
    }
}