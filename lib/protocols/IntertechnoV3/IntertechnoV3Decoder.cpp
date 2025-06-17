#include "IntertechnoV3Decoder.h"

// Раскомментируй следующую строку, чтобы включить подробный вывод в Serial
// #define DEBUG_INTERTECHNO_V3

#ifdef DEBUG_INTERTECHNO_V3
#define ITV3_DEB_PRINT(...) Serial.print(__VA_ARGS__)
#define ITV3_DEB_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define ITV3_DEB_PRINT(...)
#define ITV3_DEB_PRINTLN(...)
#endif

// --- Внутренние переменные модуля ---
enum IntertechnoV3State {
    ITV3_RESET,
    ITV3_WAIT_PREAMBLE_END,
    ITV3_WAIT_SYNC_PULSE,
    ITV3_WAIT_SYNC_PAUSE,
    ITV3_DECODE_PULSE_1,
    ITV3_DECODE_PAUSE_1,
    ITV3_DECODE_PULSE_2,
    ITV3_DECODE_PAUSE_2,
};
volatile IntertechnoV3State itv3_state = ITV3_RESET;
volatile uint64_t itv3_rawData = 0;
volatile uint8_t itv3_bitCount = 0;
volatile uint32_t itv3_lastDurations[3]; // Буфер для 3-х последних длительностей

// Константы
#define ITV3_TE_SHORT 275
#define ITV3_TE_LONG 1375
#define ITV3_TE_DELTA 150

// --- Вспомогательная функция для проверки длительности ---
static bool is_duration_itv3(uint32_t duration, uint8_t multiplier) {
    uint32_t base = ITV3_TE_SHORT * multiplier;
    uint32_t delta = ITV3_TE_DELTA * multiplier;
    return (duration > base - delta) && (duration < base + delta);
}

// --- Главная функция-обработчик ---
void processIntertechnoV3Signal(bool level, uint32_t duration) {
    // Конец пакета определяется длинной паузой в любом состоянии
    if (!level && duration > (ITV3_TE_SHORT * 40)) {
        ITV3_DEB_PRINT("-> End of transmission detected. Bits received: "); ITV3_DEB_PRINTLN(itv3_bitCount);
        if (itv3_bitCount == 32 || itv3_bitCount == 36) {
             // Здесь можно добавить проверку контрольной суммы, если она нужна
             barrierCodeMain = itv3_rawData;
             barrierBit = itv3_bitCount;
             barrierProtocol = 21; // ID для Intertechno V3
             newSignalReady = true;
        }
        itv3_state = ITV3_RESET;
        return;
    }


    switch (itv3_state) {
    case ITV3_RESET:
        // Шаг 1: Ищем начало преамбулы - короткий высокий импульс
        if (level && is_duration_itv3(duration, 1)) {
            itv3_state = ITV3_WAIT_PREAMBLE_END;
            ITV3_DEB_PRINTLN("DEBUG: Got preamble start pulse -> WAIT_PREAMBLE_END");
        }
        break;

    case ITV3_WAIT_PREAMBLE_END:
        // Шаг 2: Ждем ОЧЕНЬ длинную паузу преамбулы
        if (!level && is_duration_itv3(duration, 38)) {
            itv3_state = ITV3_WAIT_SYNC_PULSE;
            ITV3_DEB_PRINTLN("DEBUG: Got preamble pause -> WAIT_SYNC_PULSE");
        } else {
            itv3_state = ITV3_RESET;
        }
        break;

     case ITV3_WAIT_SYNC_PULSE:
        // Шаг 3: Ждем короткий высокий импульс синхронизации
        if (level && is_duration_itv3(duration, 1)) {
            itv3_state = ITV3_WAIT_SYNC_PAUSE;
            ITV3_DEB_PRINTLN("DEBUG: Got sync pulse -> WAIT_SYNC_PAUSE");
        } else {
            itv3_state = ITV3_RESET;
        }
        break;
    
    case ITV3_WAIT_SYNC_PAUSE:
        // Шаг 4: Ждем паузу синхронизации
        if (!level && is_duration_itv3(duration, 10)) {
            ITV3_DEB_PRINTLN("DEBUG: Sync OK! -> DECODE_PULSE_1");
            itv3_rawData = 0;
            itv3_bitCount = 0;
            itv3_state = ITV3_DECODE_PULSE_1;
        } else {
            itv3_state = ITV3_RESET;
        }
        break;

    // --- Начало декодирования 4-частного символа ---
    case ITV3_DECODE_PULSE_1:
        if (level && is_duration_itv3(duration, 1)) {
            itv3_lastDurations[0] = duration;
            itv3_state = ITV3_DECODE_PAUSE_1;
        } else { itv3_state = ITV3_RESET; }
        break;
    case ITV3_DECODE_PAUSE_1:
        if (!level) {
            itv3_lastDurations[1] = duration;
            itv3_state = ITV3_DECODE_PULSE_2;
        } else { itv3_state = ITV3_RESET; }
        break;
    case ITV3_DECODE_PULSE_2:
        if (level && is_duration_itv3(duration, 1)) {
            itv3_lastDurations[2] = duration;
            itv3_state = ITV3_DECODE_PAUSE_2;
        } else { itv3_state = ITV3_RESET; }
        break;
    case ITV3_DECODE_PAUSE_2:
        if (!level) {
            bool bit_val;
            bool success = false;
            // Анализируем 4 собранных импульса
            // Бит '0': H(1), L(1), H(1), L(5)
            if (is_duration_itv3(itv3_lastDurations[1], 1) && is_duration_itv3(duration, 5)) {
                bit_val = 0; success = true;
            // Бит '1': H(1), L(5), H(1), L(1)
            } else if (is_duration_itv3(itv3_lastDurations[1], 5) && is_duration_itv3(duration, 1)) {
                bit_val = 1; success = true;
            // Бит 'Dimm': H(1), L(1), H(1), L(1)
            } else if (is_duration_itv3(itv3_lastDurations[1], 1) && is_duration_itv3(duration, 1)) {
                // В 36-битной версии это особый бит, в 32-битной - ошибка
                // Для простоты будем считать его как 0, т.к. он есть в 36-битной версии
                bit_val = 0; success = true; 
            }

            if (success) {
                ITV3_DEB_PRINT(bit_val);
                if (itv3_bitCount < 36) {
                    itv3_rawData = (itv3_rawData << 1) | bit_val;
                    itv3_bitCount++;
                }
                itv3_state = ITV3_DECODE_PULSE_1; // Готовы к следующему биту
            } else {
                ITV3_DEB_PRINTLN("\nBit timing error, resetting.");
                itv3_state = ITV3_RESET;
            }
        } else {
            itv3_state = ITV3_RESET;
        }
        break;
    }
}