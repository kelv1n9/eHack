#include "HormannDecoder.h"

// --- Внутренние переменные модуля ---
enum HormannState {
    HORMANN_RESET,
    HORMANN_WAIT_START_PAUSE,
    HORMANN_SAVE_DURATION,
    HORMANN_CHECK_DURATION
};
volatile HormannState hormannState = HORMANN_RESET;
volatile uint64_t hormannRawData = 0;
volatile uint8_t hormannBitCount = 0;
volatile uint32_t hormannLastDuration = 0;

// Константы таймингов
#define HORMANN_TE_SHORT 500
#define HORMANN_TE_LONG 1000
#define HORMANN_TE_DELTA 200
#define HORMANN_HEADER_US (HORMANN_TE_SHORT * 24)

// Паттерн, которому должен соответствовать валидный ключ Hormann HSM
const uint64_t HORMANN_HSM_PATTERN = 0xFF000000003ULL;

// --- Вспомогательные функции ---
static bool is_duration(uint32_t duration, uint32_t base) {
    return (duration > base - HORMANN_TE_DELTA) && (duration < base + HORMANN_TE_DELTA);
}

// Проверка ключа на соответствие паттерну
static bool check_hormann_pattern(uint64_t data) {
    return (data & HORMANN_HSM_PATTERN) == HORMANN_HSM_PATTERN;
}

// --- Главная функция-обработчик ---
void processHormannSignal(bool level, uint32_t duration) {
    switch (hormannState) {
    case HORMANN_RESET:
        // Ждем длинный высокий импульс заголовка суб-пакета
        if (level && (duration > HORMANN_HEADER_US - (HORMANN_TE_DELTA * 10)) && (duration < HORMANN_HEADER_US + (HORMANN_TE_DELTA * 10))) {
            hormannState = HORMANN_WAIT_START_PAUSE;
        }
        break;

    case HORMANN_WAIT_START_PAUSE:
        // После длинного заголовка должна идти короткая пауза
        if (!level && is_duration(duration, HORMANN_TE_SHORT)) {
            hormannRawData = 0;
            hormannBitCount = 0;
            hormannState = HORMANN_SAVE_DURATION;
        } else {
            hormannState = HORMANN_RESET;
        }
        break;

    case HORMANN_SAVE_DURATION:
        // Ждем высокий импульс данных или заголовок следующего суб-пакета
        if (level) {
            // Проверяем, не является ли этот импульс заголовком следующего пакета
            if (duration > (HORMANN_TE_SHORT * 5)) {
                // Если да, то предыдущий пакет закончился. Проверяем его.
                if (hormannBitCount == 44 && check_hormann_pattern(hormannRawData)) {
                    // Успех! Ключ валидный.
                    barrierCodeMain = hormannRawData;
                    barrierProtocol = 6; // Уникальный ID для Hormann HSM
                    newSignalReady = true;
                }
                // В любом случае, мы уже поймали заголовок нового пакета,
                // поэтому переходим к ожиданию его стартовой паузы.
                hormannState = HORMANN_WAIT_START_PAUSE;
                break;
            }
            // Если это не заголовок, то это обычный импульс данных
            hormannLastDuration = duration;
            hormannState = HORMANN_CHECK_DURATION;
        } else {
            hormannState = HORMANN_RESET;
        }
        break;

    case HORMANN_CHECK_DURATION:
        // Ждем паузу и декодируем бит
        if (!level) {
            bool bit_val;
            bool success = false;
            if (is_duration(hormannLastDuration, HORMANN_TE_LONG) && is_duration(duration, HORMANN_TE_SHORT)) {
                bit_val = 1; success = true;
            } else if (is_duration(hormannLastDuration, HORMANN_TE_SHORT) && is_duration(duration, HORMANN_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                if (hormannBitCount < 44) {
                    hormannRawData = (hormannRawData << 1) | bit_val;
                    hormannBitCount++;
                }
                hormannState = HORMANN_SAVE_DURATION; // Готовимся к следующему биту
            } else {
                hormannState = HORMANN_RESET;
            }
        } else {
            hormannState = HORMANN_RESET;
        }
        break;
    }
}