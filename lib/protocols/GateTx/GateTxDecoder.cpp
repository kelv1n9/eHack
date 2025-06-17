#include "GateTxDecoder.h"

// --- Внутренние переменные модуля ---
enum GateTxState {
    GATE_TX_RESET,
    GATE_TX_WAIT_START_BIT,
    GATE_TX_SAVE_PAUSE,
    GATE_TX_CHECK_PULSE
};
volatile GateTxState gateTxState = GATE_TX_RESET;
volatile uint32_t gateTxRawData = 0;
volatile uint8_t gateTxBitCount = 0;
volatile uint32_t gateTxLastPauseDuration = 0;

// Константы таймингов
#define GATETX_TE_SHORT 350
#define GATETX_TE_LONG 700
#define GATETX_TE_DELTA 150 

// --- Вспомогательная функция для проверки длительности ---
static bool is_duration(uint32_t duration, uint32_t base) {
    uint32_t d = (base == GATETX_TE_SHORT) ? GATETX_TE_DELTA : (GATETX_TE_DELTA);
    return (duration > base - d) && (duration < base + d);
}

// --- Главная функция-обработчик ---
void processGateTxSignal(bool level, uint32_t duration) {
    switch (gateTxState) {
    case GATE_TX_RESET:
        // ИСПРАВЛЕНО: Ждем любую длинную паузу (> 4000 мкс), чтобы начать.
        // Это делает приемник гораздо более устойчивым.
        if (!level && (duration > 4000)) {
            gateTxState = GATE_TX_WAIT_START_BIT;
        }
        break;

    case GATE_TX_WAIT_START_BIT:
        // Ждем длинный высокий стартовый импульс.
        if (level && is_duration(duration, GATETX_TE_LONG)) {
            gateTxRawData = 0;
            gateTxBitCount = 0;
            gateTxState = GATE_TX_SAVE_PAUSE;
        } else {
            gateTxState = GATE_TX_RESET;
        }
        break;

    case GATE_TX_SAVE_PAUSE:
        // Ждем паузу (LOW), которая начинает каждый бит данных.
        if (!level) {
            // Длинная пауза в этом состоянии означает конец пакета
             if (duration > (GATETX_TE_LONG * 4)) {
                if(gateTxBitCount == 24) {
                    barrierCodeMain = gateTxRawData;
                    barrierProtocol = 8;
                    newSignalReady = true;
                }
                gateTxState = GATE_TX_RESET; // Готовимся к следующей преамбуле
                break;
            }
            gateTxLastPauseDuration = duration;
            gateTxState = GATE_TX_CHECK_PULSE;
        } else {
             gateTxState = GATE_TX_RESET;
        }
        break;

    case GATE_TX_CHECK_PULSE:
        // Ждем импульс (HIGH) и декодируем бит по паре "пауза-импульс".
        if (level) {
            bool bit_val;
            bool success = false;
            if (is_duration(gateTxLastPauseDuration, GATETX_TE_LONG) && is_duration(duration, GATETX_TE_SHORT)) {
                bit_val = 1; success = true; // длинная пауза + короткий импульс
            } else if (is_duration(gateTxLastPauseDuration, GATETX_TE_SHORT) && is_duration(duration, GATETX_TE_LONG)) {
                bit_val = 0; success = true; // короткая пауза + длинный импульс
            }

            if (success) {
                if (gateTxBitCount < 24) {
                    gateTxRawData = (gateTxRawData << 1) | bit_val;
                    gateTxBitCount++;
                }
                gateTxState = GATE_TX_SAVE_PAUSE;
            } else {
                gateTxState = GATE_TX_RESET;
            }
        } else {
            gateTxState = GATE_TX_RESET;
        }
        break;
    }
}