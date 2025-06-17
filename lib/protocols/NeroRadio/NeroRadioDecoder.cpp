#include "NeroRadioDecoder.h"

// --- Внутренние переменные модуля ---
enum NeroRadioState {
    NERO_RADIO_RESET,
    NERO_RADIO_CHECK_PREAMBLE,
    NERO_RADIO_SAVE_DURATION,
    NERO_RADIO_CHECK_DURATION
};
volatile NeroRadioState neroRadioState = NERO_RADIO_RESET;
volatile uint64_t neroRadioRawData = 0;
volatile uint8_t neroRadioBitCount = 0;
volatile uint16_t neroRadioHeaderCount = 0;
volatile uint32_t neroRadioLastDuration = 0;

// Константы таймингов
#define NERO_RADIO_TE_SHORT 200
#define NERO_RADIO_TE_LONG 400
#define NERO_RADIO_TE_DELTA 100 // Увеличим немного для надежности

// Вспомогательная функция для проверки длительности
static bool is_duration(uint32_t duration, uint32_t base) {
    return (duration > base - NERO_RADIO_TE_DELTA) && (duration < base + NERO_RADIO_TE_DELTA);
}

// --- Главная функция-обработчик ---
void processNeroRadioSignal(bool level, uint32_t duration) {
    switch (neroRadioState) {
    case NERO_RADIO_RESET:
        // Ждем первый короткий высокий импульс преамбулы
        if (level && is_duration(duration, NERO_RADIO_TE_SHORT)) {
            neroRadioHeaderCount = 1;
            neroRadioLastDuration = duration;
            neroRadioState = NERO_RADIO_CHECK_PREAMBLE;
        }
        break;

    case NERO_RADIO_CHECK_PREAMBLE:
        if (level) { 
            neroRadioLastDuration = duration;
        } else { // пришла пауза (низкий уровень)
            if (is_duration(neroRadioLastDuration, NERO_RADIO_TE_SHORT) && is_duration(duration, NERO_RADIO_TE_SHORT)) {
                // Это часть преамбулы (короткий-короткий)
                neroRadioHeaderCount++;
            } else if (neroRadioHeaderCount > 40 && is_duration(neroRadioLastDuration, NERO_RADIO_TE_SHORT * 4) && is_duration(duration, NERO_RADIO_TE_SHORT)) {
                // Это стартовый бит! Начинаем прием данных.
                neroRadioRawData = 0;
                neroRadioBitCount = 0;
                neroRadioState = NERO_RADIO_SAVE_DURATION;
            } else {
                neroRadioState = NERO_RADIO_RESET;
            }
        }
        break;

    case NERO_RADIO_SAVE_DURATION:
        if (level) {
            neroRadioLastDuration = duration;
            neroRadioState = NERO_RADIO_CHECK_DURATION;
        } else {
            neroRadioState = NERO_RADIO_RESET; 
        }
        break;

    case NERO_RADIO_CHECK_DURATION:
        if (!level) {
            bool bit_val;
            bool success = false;
            
            // Сначала проверяем, не является ли это очень длинной финальной паузой
            if (duration >= (NERO_RADIO_TE_SHORT * 10)) {
                // Да, это конец пакета. Декодируем последний бит, который был ПЕРЕД этой паузой.
                if (is_duration(neroRadioLastDuration, NERO_RADIO_TE_LONG)) {
                    bit_val = 1; success = true;
                } else if (is_duration(neroRadioLastDuration, NERO_RADIO_TE_SHORT)) {
                    bit_val = 0; success = true;
                }
                
                if(success) {
                    neroRadioRawData = (neroRadioRawData << 1) | bit_val;
                    neroRadioBitCount++;
                }

                // Проверяем, что пакет собран полностью
                if (neroRadioBitCount == 56) {
                    barrierCodeMain = neroRadioRawData;
                    barrierProtocol = 7; // Уникальный ID для Nero Radio
                    newSignalReady = true;
                }
                neroRadioState = NERO_RADIO_RESET; // В любом случае сброс
                break; // Выходим из switch
            }

            // Если это не финальная пауза, декодируем обычный бит
            if (is_duration(neroRadioLastDuration, NERO_RADIO_TE_LONG) && is_duration(duration, NERO_RADIO_TE_SHORT)) {
                bit_val = 1; success = true;
            } else if (is_duration(neroRadioLastDuration, NERO_RADIO_TE_SHORT) && is_duration(duration, NERO_RADIO_TE_LONG)) {
                bit_val = 0; success = true;
            }

            if (success) {
                neroRadioRawData = (neroRadioRawData << 1) | bit_val;
                neroRadioBitCount++;
                neroRadioState = NERO_RADIO_SAVE_DURATION;
            } else {
                neroRadioState = NERO_RADIO_RESET;
            }
        } else {
            neroRadioState = NERO_RADIO_RESET;
        }
        break;
    }
}