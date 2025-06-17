#include "AnsonicDecoder.h"

// !!! Для включения отладки раскомментируйте следующую строку !!!
// #define DEBUG_ANSONIC

#ifdef DEBUG_ANSONIC
#define ANSONIC_DEB_PRINT(...) Serial.print(__VA_ARGS__)
#define ANSONIC_DEB_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define ANSONIC_DEB_PRINT(...)
#define ANSONIC_DEB_PRINTLN(...)
#endif


// --- Внутренние переменные модуля ---
enum AnsonicState {
    ANSONIC_RESET,
    ANSONIC_FOUND_START_BIT,
    ANSONIC_SAVE_DURATION,
    ANSONIC_CHECK_DURATION
};
volatile AnsonicState ansonicState = ANSONIC_RESET;
volatile uint16_t ansonicRawData = 0; // 12 бит влезают в uint16_t
volatile uint8_t ansonicBitCount = 0;
volatile uint32_t ansonicLastDuration = 0;

// Константы таймингов
#define ANSONIC_TE_SHORT 555
#define ANSONIC_TE_LONG 1111
#define ANSONIC_TE_DELTA 150
#define ANSONIC_PREAMBLE_US (ANSONIC_TE_SHORT * 35)
#define ANSONIC_PREAMBLE_DELTA (ANSONIC_TE_DELTA * 35)

// --- Вспомогательная функция для проверки разницы ---
static bool duration_diff(uint32_t d1, uint32_t d2, uint32_t delta) {
    if (d1 > d2) return (d1 - d2) < delta;
    else return (d2 - d1) < delta;
}

// --- Главная функция-обработчик ---
void processAnsonicSignal(bool level, uint32_t duration) {
    switch (ansonicState) {
    case ANSONIC_RESET:
        // Шаг 1: Ждем длинную паузу преамбулы
        if (!level && duration_diff(duration, ANSONIC_PREAMBLE_US, ANSONIC_PREAMBLE_DELTA)) {
            ANSONIC_DEB_PRINTLN("DEBUG: Preamble found -> FOUND_START_BIT");
            ansonicState = ANSONIC_FOUND_START_BIT;
        }
        break;

    case ANSONIC_FOUND_START_BIT:
        // Шаг 2: Ждем короткий высокий стартовый импульс
        if (level && duration_diff(duration, ANSONIC_TE_SHORT, ANSONIC_TE_DELTA)) {
            ANSONIC_DEB_PRINTLN("DEBUG: Start bit found -> SAVE_DURATION");
            ansonicRawData = 0;
            ansonicBitCount = 0;
            ansonicState = ANSONIC_SAVE_DURATION;
        } else if(!level) { // Если пришла пауза, остаемся в этом состоянии
             // Ничего не делаем, ждем импульс
        } else { // Если пришел импульс не той длины, сброс
            ansonicState = ANSONIC_RESET;
        }
        break;

    case ANSONIC_SAVE_DURATION:
        // Шаг 3: Ждем паузу (LOW), которая начинает каждый бит данных
        if (!level) {
            // Конец пакета определяется по паузе > 4*te_short
            if (duration >= (ANSONIC_TE_SHORT * 4)) {
                ANSONIC_DEB_PRINT("DEBUG: End of packet detected. Bits: "); ANSONIC_DEB_PRINTLN(ansonicBitCount);
                if (ansonicBitCount == 12) {
                    barrierCodeMain = ansonicRawData & 0xFFF;
                    barrierProtocol = 23; // Уникальный ID для Ansonic
                    newSignalReady = true;
                }
                ansonicState = ANSONIC_FOUND_START_BIT; // Готовы к следующему пакету в серии
                break;
            }
            ansonicLastDuration = duration;
            ansonicState = ANSONIC_CHECK_DURATION;
        } else {
             ansonicState = ANSONIC_RESET;
        }
        break;

    case ANSONIC_CHECK_DURATION:
        // Шаг 4: Ждем импульс (HIGH) и декодируем бит
        if (level) {
            bool bit_val;
            bool success = false;
            
            // Бит '1': Короткая пауза + длинный импульс
            if (duration_diff(ansonicLastDuration, ANSONIC_TE_SHORT, ANSONIC_TE_DELTA) && duration_diff(duration, ANSONIC_TE_LONG, ANSONIC_TE_DELTA)) {
                bit_val = 1; success = true;
            // Бит '0': Длинная пауза + короткий импульс
            } else if (duration_diff(ansonicLastDuration, ANSONIC_TE_LONG, ANSONIC_TE_DELTA) && duration_diff(duration, ANSONIC_TE_SHORT, ANSONIC_TE_DELTA)) {
                bit_val = 0; success = true;
            }

            if (success) {
                ANSONIC_DEB_PRINT(bit_val);
                if (ansonicBitCount < 12) {
                    ansonicRawData = (ansonicRawData << 1) | bit_val;
                    ansonicBitCount++;
                }
                ansonicState = ANSONIC_SAVE_DURATION;
            } else {
                ANSONIC_DEB_PRINTLN("\nBit timing error, resetting.");
                ansonicState = ANSONIC_RESET;
            }
        } else {
            ansonicState = ANSONIC_RESET;
        }
        break;
    }
}