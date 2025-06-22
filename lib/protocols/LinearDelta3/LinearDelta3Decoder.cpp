#include "LinearDelta3Decoder.h"

// --- Внутренние переменные модуля ---
enum LinearDelta3State {
    LD3_RESET,
    LD3_SAVE_DURATION,
    LD3_CHECK_DURATION
};
volatile LinearDelta3State ld3_state = LD3_RESET;
volatile uint8_t ld3_rawData = 0;
volatile uint8_t ld3_bitCount = 0;
volatile uint32_t ld3_lastDuration = 0;
volatile uint8_t ld3_lastData = 0;

// Константы таймингов
#define LD3_TE_SHORT 500
#define LD3_TE_LONG 2000
#define LD3_TE_DELTA 200
#define LD3_GUARD_MIN_US 30000

// --- Вспомогательная функция для проверки длительности ---
static bool is_duration_ld3(uint32_t duration, uint32_t base) {
    uint32_t d = (base == LD3_TE_SHORT) ? LD3_TE_DELTA : (LD3_TE_DELTA * 4);
    return (duration > base - d) && (duration < base + d);
}

// --- Главная функция-обработчик ---
void processLinearDelta3Signal(bool level, uint32_t duration) {
    // Длинная пауза между пакетами всегда обрабатывается первой
    if (!level && duration > LD3_GUARD_MIN_US) {
        if (ld3_bitCount == 8) {
            // Логика "приема дважды"
            if (ld3_lastData != 0 && ld3_lastData == ld3_rawData) {
                barrierCodeMain = ld3_rawData;
                barrierProtocol = 8;
                newSignalReady = true;
            }
            ld3_lastData = ld3_rawData;
        } else {
             ld3_lastData = 0;
        }
        // Начинаем слушать новый пакет
        ld3_state = LD3_SAVE_DURATION;
        ld3_rawData = 0;
        ld3_bitCount = 0;
        return;
    }

    switch (ld3_state) {
    case LD3_RESET:
        // Ждем длинную паузу, чтобы начать (обработано выше)
        break;

    case LD3_SAVE_DURATION:
        if (level) {
            ld3_lastDuration = duration;
            ld3_state = LD3_CHECK_DURATION;
        } else {
            ld3_state = LD3_RESET;
        }
        break;

    case LD3_CHECK_DURATION:
        if (!level) {
            bool bit_val;
            bool success = false;
            
            // Бит '1': Короткий HIGH (500) + Очень длинный LOW (3500)
            if (is_duration_ld3(ld3_lastDuration, LD3_TE_SHORT) && is_duration_ld3(duration, LD3_TE_SHORT * 7)) {
                bit_val = 1; success = true;
            // Бит '0': Длинный HIGH (2000) + Длинный LOW (2000)
            } else if (is_duration_ld3(ld3_lastDuration, LD3_TE_LONG) && is_duration_ld3(duration, LD3_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                if (ld3_bitCount < 8) {
                    ld3_rawData = (ld3_rawData << 1) | bit_val;
                    ld3_bitCount++;
                }
                ld3_state = LD3_SAVE_DURATION;
            } else {
                ld3_state = LD3_RESET;
            }
        } else {
            ld3_state = LD3_RESET;
        }
        break;
    }
}