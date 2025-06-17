#include "NeroSketchDecoder.h"

// --- Внутренние переменные модуля ---
enum NeroSketchState {
    NERO_RESET,
    NERO_CHECK_PREAMBLE,
    NERO_SAVE_DURATION,
    NERO_CHECK_DURATION
};
volatile NeroSketchState neroState = NERO_RESET;
volatile uint64_t neroRawData = 0;
volatile uint8_t neroBitCount = 0;
volatile uint16_t neroHeaderCount = 0;
volatile uint32_t neroLastDuration = 0;

// Константы таймингов
#define NERO_TE_SHORT 330
#define NERO_TE_LONG 660
#define NERO_TE_DELTA 150

// Вспомогательная функция для проверки длительности
static bool is_duration(uint32_t duration, uint32_t base) {
    return (duration > base - NERO_TE_DELTA) && (duration < base + NERO_TE_DELTA);
}

// --- Главная функция-обработчик ---
void processNeroSketchSignal(bool level, uint32_t duration) {
    switch (neroState) {
    case NERO_RESET:
        // Ждем первый короткий высокий импульс преамбулы
        if (level && is_duration(duration, NERO_TE_SHORT)) {
            neroHeaderCount = 1;
            neroLastDuration = duration;
            neroState = NERO_CHECK_PREAMBLE;
        }
        break;

    case NERO_CHECK_PREAMBLE:
        if (level) { // пришел высокий импульс
            neroLastDuration = duration;
        } else { // пришла пауза (низкий уровень)
            if (is_duration(neroLastDuration, NERO_TE_SHORT) && is_duration(duration, NERO_TE_SHORT)) {
                // Это часть преамбулы (короткий-короткий)
                neroHeaderCount++;
            } else if (neroHeaderCount > 40 && is_duration(neroLastDuration, NERO_TE_SHORT * 4) && is_duration(duration, NERO_TE_SHORT)) {
                // Это стартовый бит (очень длинный-короткий)! Начинаем прием данных.
                neroRawData = 0;
                neroBitCount = 0;
                neroState = NERO_SAVE_DURATION;
            } else {
                neroState = NERO_RESET;
            }
        }
        break;

    case NERO_SAVE_DURATION:
        if (level) {
            // Проверяем, не является ли это стоп-битом
            if (is_duration(duration, NERO_TE_SHORT * 3)) {
                neroState = NERO_RESET; // Пакет закончился
                if (neroBitCount == 40) {
                    // Успех! Собрали ровно 40 бит.
                    barrierCodeMain = neroRawData;
                    barrierProtocol = 5; // Уникальный ID для Nero Sketch
                    newSignalReady = true;
                }
                break;
            }
            // Если не стоп-бит, то это данные
            neroLastDuration = duration;
            neroState = NERO_CHECK_DURATION;
        } else {
            neroState = NERO_RESET; // Ошибка, после паузы должен быть импульс
        }
        break;

    case NERO_CHECK_DURATION:
        if (!level) {
            bool bit_val;
            bool success = false;
            if (is_duration(neroLastDuration, NERO_TE_LONG) && is_duration(duration, NERO_TE_SHORT)) {
                bit_val = 1; success = true;
            } else if (is_duration(neroLastDuration, NERO_TE_SHORT) && is_duration(duration, NERO_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                neroRawData = (neroRawData << 1) | bit_val;
                neroBitCount++;
                neroState = NERO_SAVE_DURATION;
            } else {
                neroState = NERO_RESET;
            }
        } else {
            neroState = NERO_RESET;
        }
        break;
    }
}