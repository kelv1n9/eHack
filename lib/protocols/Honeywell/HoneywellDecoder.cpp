#include "HoneywellDecoder.h"

// --- Внутренние переменные модуля ---
enum HoneywellState {
    HW_RESET,
    HW_SAVE_PULSE,
    HW_CHECK_PAUSE
};
volatile HoneywellState hw_state = HW_RESET;
volatile uint64_t hw_rawData = 0;
volatile uint8_t hw_bitCount = 0;
volatile uint32_t hw_lastDuration = 0;

// Константы таймингов
#define HW_TE_SHORT 160
#define HW_TE_LONG 320
#define HW_TE_STOP (HW_TE_SHORT * 3) // 480
#define HW_TE_PREAMBLE (HW_TE_SHORT * 3) // 480
#define HW_TE_DELTA 80

// --- Вспомогательная функция для проверки длительности (ИСПРАВЛЕНО) ---
static bool is_duration_hw(uint32_t duration, uint32_t base) {
    // Используем фиксированный допуск, а не плавающий
    return (duration > base - HW_TE_DELTA) && (duration < base + HW_TE_DELTA);
}

// Проверка бита четности
static bool check_honeywell_parity(uint64_t data) {
    uint8_t set_bits_count = 0;
    for (int i = 1; i < 48; i++) {
        if ((data >> i) & 1ULL) {
            set_bits_count++;
        }
    }
    uint8_t calculated_parity = set_bits_count & 1;
    uint8_t received_parity = data & 1;
    return (calculated_parity == received_parity);
}

// --- Главная функция-обработчик ---
void processHoneywellSignal(bool level, uint32_t duration) {
    switch (hw_state) {
    case HW_RESET:
        // Шаг 1: Ждем короткую паузу-преамбулу
        if (!level && is_duration_hw(duration, HW_TE_PREAMBLE)) {
            hw_rawData = 0;
            hw_bitCount = 0;
            hw_state = HW_SAVE_PULSE;
        }
        break;

    case HW_SAVE_PULSE:
        // Шаг 2: Ждем ВЫСОКИЙ импульс
        if (level) {
            // Проверяем, не является ли это стоп-импульсом
            if (is_duration_hw(duration, HW_TE_STOP)) {
                if (hw_bitCount == 48 && check_honeywell_parity(hw_rawData)) {
                    barrierCodeMain = hw_rawData;
                    barrierProtocol = 19;
                    newSignalReady = true;
                }
                hw_state = HW_RESET; // В любом случае сброс
                break;
            }
            // Если не стоп-бит, то это данные
            hw_lastDuration = duration;
            hw_state = HW_CHECK_PAUSE;
        } else {
            hw_state = HW_RESET;
        }
        break;

    case HW_CHECK_PAUSE:
        // Шаг 3: Ждем паузу (LOW) и декодируем бит
        if (!level) {
            bool bit_val;
            bool success = false;
            // Бит '1': длинный HIGH + короткий LOW
            if (is_duration_hw(hw_lastDuration, HW_TE_LONG) && is_duration_hw(duration, HW_TE_SHORT)) {
                bit_val = 1; success = true;
            // Бит '0': короткий HIGH + длинный LOW
            } else if (is_duration_hw(hw_lastDuration, HW_TE_SHORT) && is_duration_hw(duration, HW_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                if (hw_bitCount < 48) {
                    hw_rawData = (hw_rawData << 1) | bit_val;
                    hw_bitCount++;
                }
                hw_state = HW_SAVE_PULSE; 
            } else {
                hw_state = HW_RESET;
            }
        } else {
            hw_state = HW_RESET;
        }
        break;
    }
}