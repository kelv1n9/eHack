#include "MegaCodeDecoder.h"

// --- Внутренние переменные модуля ---
enum MegaCodeState {
    MEGACODE_RESET,
    MEGACODE_WAIT_PULSE,
    MEGACODE_WAIT_PAUSE,
};
volatile MegaCodeState megaCodeState = MEGACODE_RESET;
volatile uint32_t megaCodeRawData = 0;
volatile uint8_t megaCodeBitCount = 0;
volatile uint32_t megaCodeLastData = 0;

// Константы таймингов
#define MEGACODE_TE 1000
#define MEGACODE_TE_DELTA 300 // Допуск
#define MEGACODE_GUARD_MIN_US 10000

// --- Вспомогательная функция для проверки длительности ---
static bool is_te(uint32_t duration, uint8_t multiplier) {
    uint32_t base = MEGACODE_TE * multiplier;
    uint32_t delta = MEGACODE_TE_DELTA * multiplier;
    return (duration > base - delta) && (duration < base + delta);
}

// --- Главная функция-обработчик ---
void processMegaCodeSignal(bool level, uint32_t duration) {
    // Длинная пауза между пакетами всегда обрабатывается первой
    if (!level && duration > MEGACODE_GUARD_MIN_US) {
        if (megaCodeBitCount == 24) {
            // Логика "приема дважды"
            if (megaCodeLastData != 0 && megaCodeLastData == megaCodeRawData) {
                barrierCodeMain = megaCodeRawData;
                barrierProtocol = 11; // ID для MegaCode
                newSignalReady = true;
            }
            megaCodeLastData = megaCodeRawData;
        } else {
             megaCodeLastData = 0;
        }
        // Начинаем слушать новый пакет
        megaCodeState = MEGACODE_WAIT_PULSE;
        megaCodeRawData = 0;
        megaCodeBitCount = 0;
        return;
    }

    switch (megaCodeState) {
    case MEGACODE_RESET:
        // Ждем длинную паузу, чтобы начать (обработано выше)
        break;

    case MEGACODE_WAIT_PULSE:
        // Ждем ВЫСОКИЙ импульс. Он всегда должен быть ~1000 мкс.
        if (level && is_te(duration, 1)) {
            megaCodeState = MEGACODE_WAIT_PAUSE;
        } else if (level) { // Если пришел высокий импульс, но не той длины
            megaCodeState = MEGACODE_RESET;
        }
        break;

    case MEGACODE_WAIT_PAUSE:
        // Ждем НИЗКИЙ импульс (паузу) и декодируем бит
        if (!level) {
            bool bit_val;
            bool success = false;
            
            // Пакет всегда начинается с бита "1", который мы не декодируем, а просто знаем
            if (megaCodeBitCount == 0) {
                megaCodeRawData = 1;
                megaCodeBitCount = 1;
            }

            // Декодируем бит по длине паузы
            if (is_te(duration, 2)) {      // Пауза ~2000 мкс -> бит 0
                bit_val = 0; success = true;
            } else if (is_te(duration, 5)) { // Пауза ~5000 мкс -> бит 1
                bit_val = 1; success = true;
            }

            if (success) {
                if (megaCodeBitCount < 24) {
                    megaCodeRawData = (megaCodeRawData << 1) | bit_val;
                    megaCodeBitCount++;
                }
                megaCodeState = MEGACODE_WAIT_PULSE;
            } else {
                megaCodeState = MEGACODE_RESET;
            }
        } else {
            megaCodeState = MEGACODE_RESET;
        }
        break;
    }
}