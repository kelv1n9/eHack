#include "HoltekDecoder.h"

// --- Внутренние переменные модуля ---
enum HoltekState {
    HOLTEK_RESET,
    HOLTEK_WAIT_START_BIT,
    HOLTEK_SAVE_PAUSE,
    HOLTEK_CHECK_PULSE
};
volatile HoltekState holtekState = HOLTEK_RESET;
volatile uint64_t holtekRawData = 0; // 40 бит влезают в uint64_t
volatile uint8_t holtekBitCount = 0;
volatile uint32_t holtekLastPauseDuration = 0;

// Константы таймингов и протокола
#define HOLTEK_TE_SHORT 430
#define HOLTEK_TE_LONG 870
#define HOLTEK_TE_DELTA 150 
#define HOLTEK_PREAMBLE_US (HOLTEK_TE_SHORT * 36) // ~15480 us
#define HOLTEK_PREAMBLE_DELTA (HOLTEK_TE_DELTA * 36)

// Валидация ключа: старшие 4 бита должны быть 0x5
const uint64_t HOLTEK_HEADER_MASK = 0xF000000000ULL;
const uint64_t HOLTEK_HEADER      = 0x5000000000ULL;


// --- Вспомогательная функция для проверки длительности ---
static bool is_duration_holtek(uint32_t duration, uint32_t base) {
    uint32_t d = (base == HOLTEK_TE_SHORT) ? HOLTEK_TE_DELTA : (HOLTEK_TE_DELTA * 2);
    return (duration > base - d) && (duration < base + d);
}

// --- Главная функция-обработчик ---
void processHoltekSignal(bool level, uint32_t duration) {
    switch (holtekState) {
    case HOLTEK_RESET:
        // Шаг 1: Ждем длинную паузу преамбулы
        if (!level && (duration > HOLTEK_PREAMBLE_US - HOLTEK_PREAMBLE_DELTA) && (duration < HOLTEK_PREAMBLE_US + HOLTEK_PREAMBLE_DELTA)) {
            holtekState = HOLTEK_WAIT_START_BIT;
        }
        break;

    case HOLTEK_WAIT_START_BIT:
        // Шаг 2: Ждем короткий высокий стартовый импульс
        if (level && is_duration_holtek(duration, HOLTEK_TE_SHORT)) {
            holtekRawData = 0;
            holtekBitCount = 0;
            holtekState = HOLTEK_SAVE_PAUSE; 
        } else {
            holtekState = HOLTEK_RESET;
        }
        break;

    case HOLTEK_SAVE_PAUSE:
        // Шаг 3: Ждем паузу (LOW), которая начинает каждый бит данных
        if (!level) {
            // Длинная пауза в этом состоянии означает конец пакета
             if (duration > (HOLTEK_TE_SHORT * 10)) {
                holtekState = HOLTEK_RESET;
                break;
            }
            holtekLastPauseDuration = duration;
            holtekState = HOLTEK_CHECK_PULSE;
        } else {
             holtekState = HOLTEK_RESET;
        }
        break;

    case HOLTEK_CHECK_PULSE:
        // Шаг 4: Ждем импульс (HIGH) и декодируем бит по паре "пауза-импульс"
        if (level) {
            bool bit_val;
            bool success = false;
            // Длинная пауза + короткий импульс = 1
            if (is_duration_holtek(holtekLastPauseDuration, HOLTEK_TE_LONG) && is_duration_holtek(duration, HOLTEK_TE_SHORT)) {
                bit_val = 1; success = true;
            // Короткая пауза + длинный импульс = 0
            } else if (is_duration_holtek(holtekLastPauseDuration, HOLTEK_TE_SHORT) && is_duration_holtek(duration, HOLTEK_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                if (holtekBitCount < 40) {
                    holtekRawData = (holtekRawData << 1) | bit_val;
                    holtekBitCount++;
                }

                // Если собрали все 40 бит
                if (holtekBitCount == 40) {
                    // Проверяем, что старшие 4 бита соответствуют маске
                    if ((holtekRawData & HOLTEK_HEADER_MASK) == HOLTEK_HEADER) {
                        barrierCodeMain = holtekRawData;
                        barrierProtocol = 9; // ID для Holtek
                        newSignalReady = true;
                    }
                    holtekState = HOLTEK_RESET; // В любом случае сброс
                } else {
                    holtekState = HOLTEK_SAVE_PAUSE; // Готовимся к следующему биту
                }
            } else {
                holtekState = HOLTEK_RESET;
            }
        } else {
            holtekState = HOLTEK_RESET;
        }
        break;
    }
}