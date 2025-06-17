#include "SMC5326Decoder.h"

// Отладка включена по умолчанию
// #define DEBUG_SMC5326

#ifdef DEBUG_SMC5326
#define SMC_DEB_PRINT(...) Serial.print(__VA_ARGS__)
#define SMC_DEB_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define SMC_DEB_PRINT(...)
#define SMC_DEB_PRINTLN(...)
#endif


// --- Внутренние переменные модуля ---
enum SmcState {
    SMC_RESET,
    SMC_SAVE_DURATION,
    SMC_CHECK_DURATION
};
volatile SmcState smcState = SMC_RESET;
volatile uint32_t smcRawData = 0;
volatile uint8_t smcBitCount = 0;
volatile uint32_t smcLastDuration = 0;

// Константы таймингов
#define SMC_TE_SHORT 300
#define SMC_TE_LONG 900
#define SMC_TE_DELTA 200 // Увеличим допуск
#define SMC_GUARD_MIN_US 7000 // te_short * 25

// --- Вспомогательная функция для проверки разницы ---
static bool duration_diff_smc(uint32_t d1, uint32_t d2, uint32_t delta) {
    if (d1 > d2) return (d1 - d2) < delta;
    else return (d2 - d1) < delta;
}

// --- Главная функция-обработчик (ФИНАЛЬНАЯ ИСПРАВЛЕННАЯ ЛОГИКА) ---
void processSmc5326Signal(bool level, uint32_t duration) {
    // Длинная пауза между пакетами всегда обрабатывается первой.
    if (!level && duration > SMC_GUARD_MIN_US) {
        SMC_DEB_PRINT("DEBUG: Guard Time detected. ");
        if (smcBitCount == 25) {
            SMC_DEB_PRINTLN("SUCCESS! Packet received and captured.");
            barrierCodeMain = smcRawData;
            barrierProtocol = 24; // ID для SMC5326
            newSignalReady = true;
        } else if (smcState != SMC_RESET && smcBitCount > 0) { // Проверяем, только если что-то было принято
            SMC_DEB_PRINT("Incomplete packet ("); SMC_DEB_PRINT(smcBitCount); SMC_DEB_PRINTLN(" bits), discarding.");
        }
        
        // Начинаем слушать новый пакет.
        // ИСПРАВЛЕНО: НЕ сбрасываем состояние в RESET, а сразу готовимся ловить первый импульс.
        smcState = SMC_SAVE_DURATION;
        smcRawData = 0;
        smcBitCount = 0;
        return;
    }

    switch (smcState) {
    case SMC_RESET:
        // Это состояние теперь используется только для сброса по ошибке.
        // Начало приема происходит по длинной паузе выше.
        break;

    case SMC_SAVE_DURATION:
        // Ждем ВЫСОКИЙ импульс данных
        if (level) {
            smcLastDuration = duration;
            smcState = SMC_CHECK_DURATION;
        }
        break;

    case SMC_CHECK_DURATION:
        // Ждем НИЗКИЙ импульс (паузу) и декодируем бит
        if (!level) {
            bool bit_val;
            bool success = false;
            
            // Бит '1': длинный HIGH + короткий LOW
            if (duration_diff_smc(smcLastDuration, SMC_TE_LONG, SMC_TE_DELTA * 3) && duration_diff_smc(duration, SMC_TE_SHORT, SMC_TE_DELTA)) {
                bit_val = 1; success = true;
            // Бит '0': короткий HIGH + длинный LOW
            } else if (duration_diff_smc(smcLastDuration, SMC_TE_SHORT, SMC_TE_DELTA) && duration_diff_smc(duration, SMC_TE_LONG, SMC_TE_DELTA * 3)) {
                bit_val = 0; success = true;
            }

            if (success) {
                SMC_DEB_PRINT(bit_val);
                if (smcBitCount < 25) {
                    smcRawData = (smcRawData << 1) | bit_val;
                    smcBitCount++;
                }
                smcState = SMC_SAVE_DURATION;
            } else {
                SMC_DEB_PRINTLN("\nBit timing error, resetting.");
                smcState = SMC_RESET;
            }
        } else {
            smcState = SMC_RESET;
        }
        break;
    }
}