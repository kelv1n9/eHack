#include "LinearDecoder.h"

// --- Внутренние переменные модуля ---
enum LinearState {
    LINEAR_RESET,
    LINEAR_SAVE_DURATION,
    LINEAR_CHECK_DURATION
};
volatile LinearState linearState = LINEAR_RESET;
volatile uint16_t linearRawData = 0;
volatile uint8_t linearBitCount = 0;
volatile uint32_t linearLastDuration = 0;

// Константы таймингов
#define LINEAR_TE_SHORT 500
#define LINEAR_TE_LONG 1500
#define LINEAR_TE_DELTA 150 
#define LINEAR_GUARD_TIME_MIN (LINEAR_TE_SHORT * 5) // Пауза, которая точно длиннее битовой

// --- Вспомогательная функция для проверки длительности ---
static bool is_duration(uint32_t duration, uint32_t base) {
    return (duration > base - LINEAR_TE_DELTA) && (duration < base + LINEAR_TE_DELTA);
}

// --- Главная функция-обработчик ---
void processLinearSignal(bool level, uint32_t duration) {
    // Длинная пауза между пакетами всегда сбрасывает автомат
    if (!level && duration > 20000) {
        linearState = LINEAR_SAVE_DURATION;
        linearRawData = 0;
        linearBitCount = 0;
        return;
    }

    switch (linearState) {
    case LINEAR_RESET:
        // Ждем длинную паузу, чтобы начать (обработано выше)
        break;

    case LINEAR_SAVE_DURATION:
        // Ждем ВЫСОКИЙ импульс данных
        if (level) {
            linearLastDuration = duration;
            linearState = LINEAR_CHECK_DURATION;
        } else {
            // Если после синхронизации пришла не высокая посылка, а низкая, то это ошибка
            linearState = LINEAR_RESET;
        }
        break;

    case LINEAR_CHECK_DURATION:
        // Ждем НИЗКИЙ импульс (паузу) и декодируем бит
        if (!level) {
            bool bit_val;
            bool success = false;

            if (is_duration(linearLastDuration, LINEAR_TE_LONG) && is_duration(duration, LINEAR_TE_SHORT)) {
                bit_val = 1; success = true; // Длинный HIGH + короткий LOW
            } else if (is_duration(linearLastDuration, LINEAR_TE_SHORT) && is_duration(duration, LINEAR_TE_LONG)) {
                bit_val = 0; success = true; // Короткий HIGH + длинный LOW
            }
            
            // ИСПРАВЛЕНИЕ: Специальная проверка для ПОСЛЕДНЕГО бита с его длинной паузой
            else if (duration > LINEAR_GUARD_TIME_MIN) {
                if(is_duration(linearLastDuration, LINEAR_TE_LONG)) {
                    bit_val = 1; success = true;
                } else if (is_duration(linearLastDuration, LINEAR_TE_SHORT)) {
                    bit_val = 0; success = true;
                }
                
                if (success) { // Если последний бит распознан
                    if (linearBitCount < 10) {
                        linearRawData = (linearRawData << 1) | bit_val;
                        linearBitCount++;
                    }
                    if (linearBitCount == 10) {
                        barrierCodeMain = linearRawData;
                        barrierProtocol = 9; // Уникальный ID для Linear
                        newSignalReady = true;
                    }
                }
                linearState = LINEAR_RESET; // В любом случае сброс
                return; // Выходим, пакет обработан
            }


            if (success) {
                 if (linearBitCount < 10) {
                    linearRawData = (linearRawData << 1) | bit_val;
                    linearBitCount++;
                }
                linearState = LINEAR_SAVE_DURATION;
            } else {
                linearState = LINEAR_RESET;
            }
        } else {
            linearState = LINEAR_RESET;
        }
        break;
    }
}