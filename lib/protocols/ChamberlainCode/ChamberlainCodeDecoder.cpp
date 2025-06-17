#include "ChamberlainCodeDecoder.h"

// --- Внутренние переменные модуля ---
enum ChamberlainState {
    CHAMB_RESET,
    CHAMB_WAIT_START_BIT,
    CHAMB_SAVE_PAUSE,
    CHAMB_CHECK_PULSE
};
volatile ChamberlainState chambState = CHAMB_RESET;
volatile uint16_t chambRawData = 0;
volatile uint8_t chambBitCount = 0;
volatile uint32_t chambLastPauseDuration = 0;

// Константы таймингов
#define CHAMB_TE 1000
#define CHAMB_TE_DELTA 250 // Допуск

// --- Вспомогательная функция для проверки длительности ---
static bool is_te_chamb(uint32_t duration, uint8_t multiplier) {
    uint32_t base = CHAMB_TE * multiplier;
    uint32_t delta = CHAMB_TE_DELTA * multiplier;
    return (duration > base - delta) && (duration < base + delta);
}

// --- Главная функция-обработчик ---
void processChamberlainSignal(bool level, uint32_t duration) {
    switch (chambState) {
    case CHAMB_RESET:
        // Шаг 1: Ждем длинную паузу преамбулы
        if (!level && (duration > CHAMB_TE * 35)) { // Пауза > 35 мс
            chambState = CHAMB_WAIT_START_BIT;
        }
        break;

    case CHAMB_WAIT_START_BIT:
        // Шаг 2: Ждем короткий высокий стартовый импульс
        if (level && is_te_chamb(duration, 1)) {
            chambRawData = 0;
            chambBitCount = 0;
            chambState = CHAMB_SAVE_PAUSE;
        } else {
            chambState = CHAMB_RESET;
        }
        break;

    case CHAMB_SAVE_PAUSE:
        // Шаг 3: Ждем паузу (LOW), которая начинает каждый символ
        if (!level) {
            chambLastPauseDuration = duration;
            chambState = CHAMB_CHECK_PULSE;
        } else {
            chambState = CHAMB_RESET;
        }
        break;

    case CHAMB_CHECK_PULSE:
        // Шаг 4: Ждем импульс (HIGH) и декодируем символ по паре "пауза-импульс"
        if (level) {
            // Символ '0': Пауза 1*TE + Импульс 3*TE
            if (is_te_chamb(chambLastPauseDuration, 1) && is_te_chamb(duration, 3)) {
                if (chambBitCount < 9) { // Защита от переполнения
                    chambRawData = (chambRawData << 1) | 0;
                    chambBitCount++;
                }
                chambState = CHAMB_SAVE_PAUSE;
            // Символ '1': Пауза 2*TE + Импульс 2*TE
            } else if (is_te_chamb(chambLastPauseDuration, 2) && is_te_chamb(duration, 2)) {
                if (chambBitCount < 9) {
                    chambRawData = (chambRawData << 1) | 1;
                    chambBitCount++;
                }
                chambState = CHAMB_SAVE_PAUSE;
            // Стоп-символ: Пауза 3*TE + Импульс 1*TE
            } else if (is_te_chamb(chambLastPauseDuration, 3) && is_te_chamb(duration, 1)) {
                // Пакет получен! Проверяем его корректность.
                if (chambBitCount >= 7 && chambBitCount <= 9) {
                    barrierCodeMain = chambRawData;
                    barrierBit = chambBitCount; // Сохраняем длину ключа
                    barrierProtocol = 13;       // ID для Chamberlain
                    newSignalReady = true;
                }
                chambState = CHAMB_RESET;
            } else {
                // Неизвестный символ, сброс
                chambState = CHAMB_RESET;
            }
        } else {
            chambState = CHAMB_RESET;
        }
        break;
    }
}