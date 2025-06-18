#include "HoltekHT12xDecoder.h"

// !!! Для включения/выключения отладки - следующая строка !!!
// #define DEBUG_HOLTEK_HT12X

#ifdef DEBUG_HOLTEK_HT12X
#define HT12X_DEB_PRINT(...) Serial.print(__VA_ARGS__)
#define HT12X_DEB_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define HT12X_DEB_PRINT(...)
#define HT12X_DEB_PRINTLN(...)
#endif


// --- Внутренние переменные модуля ---
enum HoltekHT12xState {
    HT12X_RESET,
    HT12X_WAIT_START_BIT,
    HT12X_SAVE_PAUSE,
    HT12X_CHECK_PULSE
};
volatile HoltekHT12xState ht12x_state = HT12X_RESET;
volatile uint16_t ht12x_rawData = 0;
volatile uint8_t ht12x_bitCount = 0;
volatile uint32_t ht12x_lastPauseDuration = 0;

// Константы таймингов
#define HT12X_TE_SHORT 320
#define HT12X_TE_LONG 640
#define HT12X_TE_DELTA 200 // Используем щедрый допуск
#define HT12X_PREAMBLE_MIN_US 10000 

// --- Вспомогательная функция для проверки разницы ---
static bool duration_diff_ht12x(uint32_t d1, uint32_t d2, uint32_t delta) {
    if (d1 > d2) return (d1 - d2) < delta;
    else return (d2 - d1) < delta;
}

// --- Главная функция-обработчик ---
void processHoltekHT12xSignal(bool level, uint32_t duration) {
    switch (ht12x_state) {
    case HT12X_RESET:
        // Шаг 1: Ждем длинную паузу преамбулы
        if (!level && duration > HT12X_PREAMBLE_MIN_US) {
            HT12X_DEB_PRINTLN("DEBUG: Preamble found -> WAIT_START_BIT");
            ht12x_state = HT12X_WAIT_START_BIT;
        }
        break;

    case HT12X_WAIT_START_BIT:
        // Шаг 2: Ждем короткий высокий стартовый импульс
        if (level && duration_diff_ht12x(duration, HT12X_TE_SHORT, HT12X_TE_DELTA)) {
            HT12X_DEB_PRINTLN("DEBUG: Start bit found -> SAVE_PAUSE");
            ht12x_rawData = 0;
            ht12x_bitCount = 0;
            ht12x_state = HT12X_SAVE_PAUSE;
        } else {
            ht12x_state = HT12X_RESET;
        }
        break;

    case HT12X_SAVE_PAUSE:
        // Шаг 3: Ждем паузу (LOW), которая начинает каждый бит данных
        if (!level) {
            // Конец пакета определяется по паузе > 4*te_short (1280)
            if (duration > (HT12X_TE_SHORT * 4)) {
                 HT12X_DEB_PRINT("DEBUG: End of packet detected. Bits: "); HT12X_DEB_PRINTLN(ht12x_bitCount);
                if (ht12x_bitCount == 12) {
                    // Разбираем данные: 8 бит адреса + 4 бита данных/кнопки
                    barrierCodeMain = (ht12x_rawData >> 4) & 0xFF; 
                    barrierBit = ht12x_rawData & 0x0F;
                    barrierProtocol = 25; // ID для Holtek HT12x
                    newSignalReady = true;
                }
                ht12x_state = HT12X_RESET;
                break;
            }
            ht12x_lastPauseDuration = duration;
            ht12x_state = HT12X_CHECK_PULSE;
        } else {
             ht12x_state = HT12X_RESET;
        }
        break;

    case HT12X_CHECK_PULSE:
        // Шаг 4: Ждем импульс (HIGH) и декодируем бит по паре "пауза-импульс".
        if (level) {
            bool bit_val;
            bool success = false;
            
            // Логика, соответствующая ПЕРЕДАТЧИКУ:
            // Бит '0': Короткая пауза + длинный импульс
            if (duration_diff_ht12x(ht12x_lastPauseDuration, HT12X_TE_SHORT, HT12X_TE_DELTA) && duration_diff_ht12x(duration, HT12X_TE_LONG, HT12X_TE_DELTA*2)) {
                bit_val = 0; success = true;
            // Бит '1': Длинная пауза + короткий импульс
            } else if (duration_diff_ht12x(ht12x_lastPauseDuration, HT12X_TE_LONG, HT12X_TE_DELTA*2) && duration_diff_ht12x(duration, HT12X_TE_SHORT, HT12X_TE_DELTA)) {
                bit_val = 1; success = true;
            }

            if (success) {
                HT12X_DEB_PRINT(bit_val);
                if (ht12x_bitCount < 12) {
                    ht12x_rawData = (ht12x_rawData << 1) | bit_val;
                    ht12x_bitCount++;
                }
                ht12x_state = HT12X_SAVE_PAUSE;
            } else {
                HT12X_DEB_PRINTLN("\nBit timing error, resetting.");
                ht12x_state = HT12X_RESET;
            }
        } else {
            ht12x_state = HT12X_RESET;
        }
        break;
    }
}