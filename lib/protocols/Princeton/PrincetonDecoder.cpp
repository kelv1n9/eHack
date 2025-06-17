#include "PrincetonDecoder.h"

// --- Определение общих переменных ---
extern volatile boolean newSignalReady;

// --- Внутренние переменные модуля, максимально приближенные к Flipper ---
enum PrincetonState {
    RESET,
    SAVE_DURATION,
    CHECK_DURATION
};
volatile PrincetonState princetonState = RESET;
volatile uint32_t princetonCode = 0;
volatile uint8_t princetonCounter = 0;
volatile uint32_t princetonLastDuration = 0; // В Flipper это te_last
volatile uint32_t princetonLastData = 0;

// Константы
#define PRINCETON_TE_SHORT 390
#define PRINCETON_TE_LONG 1170
#define PRINCETON_TE_DELTA 300
// Длительность преамбулы/паузы из кода Flipper (36 * 390 = 14040 мкс)
#define PRINCETON_PREAMBLE_DURATION 14040
#define PRINCETON_PREAMBLE_DELTA (PRINCETON_TE_DELTA * 36)

// --- Реализация функций ---
static boolean CheckValuePrinceton(uint32_t base, uint32_t value)
{
    // Увеличим дельту для длинных импульсов, как это сделано во Flipper
    uint32_t delta = (base == PRINCETON_TE_SHORT) ? PRINCETON_TE_DELTA : PRINCETON_TE_DELTA * 3;
    return (value > base - delta) && (value < base + delta);
}

/**
 * @brief Новая версия обработчика, точно повторяющая логику Flipper
 * @param level Уровень, который был ВО ВРЕМЯ замеряемого импульса
 * @param duration Длительность импульса в микросекундах
 */
void processPrincetonSignal(bool level, uint32_t duration) {
    switch (princetonState) {
    case RESET:
        // Шаг 1: Ждем только преамбулу (очень длинную паузу)
        if (!level && (duration > PRINCETON_PREAMBLE_DURATION - PRINCETON_PREAMBLE_DELTA)) {
            princetonCode = 0;
            princetonCounter = 0;
            princetonLastData = 0;
            princetonState = SAVE_DURATION;
        }
        break;

    case SAVE_DURATION:
        // Шаг 2: Ожидаем ВЫСОКИЙ импульс. Если пришел - сохраняем его длительность.
        if (level) {
            princetonLastDuration = duration;
            princetonState = CHECK_DURATION;
        } else {
            // Если пришла пауза, когда мы ждали импульс - это ошибка.
            // Исключение - если это снова преамбула.
             if (duration > PRINCETON_PREAMBLE_DURATION - PRINCETON_PREAMBLE_DELTA) {
                princetonState = SAVE_DURATION; // Остаемся в ожидании, но сбрасываем счетчики
                princetonCode = 0;
                princetonCounter = 0;
             } else {
                princetonState = RESET;
             }
        }
        break;

    case CHECK_DURATION:
        // Шаг 3: Ожидаем НИЗКИЙ импульс (паузу). Декодируем бит на основе
        // сохраненной длительности ВЫСОКОГО и текущей длительности НИЗКОГО.
        if (!level) {
            // Проверяем, не является ли эта пауза концом пакета
            if (duration >= (PRINCETON_TE_LONG * 2)) { // Пауза > ~2340 мкс
                if (princetonCounter == 24) {
                    // Используем УПРОЩЕННУЮ логику (срабатывает с первого раза)
                    barrierProtocol = 2;
                    barrierCodeMain = princetonCode;
                    barrierBit = princetonCounter;
                    newSignalReady = true;
                }
                // Сбрасываемся для приема следующего пакета
                princetonCode = 0;
                princetonCounter = 0;
                // ВАЖНО: переходим в ожидание следующего ВЫСОКОГО импульса
                princetonState = SAVE_DURATION; 
                break;
            }

            // Если это не конец пакета, а обычная пауза, декодируем бит
            if (CheckValuePrinceton(PRINCETON_TE_LONG, princetonLastDuration) && CheckValuePrinceton(PRINCETON_TE_SHORT, duration)) {
                // Бит "1": длинный HIGH + короткий LOW
                princetonCode = (princetonCode << 1) | 1;
                princetonCounter++;
                princetonState = SAVE_DURATION; // Возвращаемся на шаг 2
            } else if (CheckValuePrinceton(PRINCETON_TE_SHORT, princetonLastDuration) && CheckValuePrinceton(PRINCETON_TE_LONG, duration)) {
                // Бит "0": короткий HIGH + длинный LOW
                princetonCode = (princetonCode << 1) | 0;
                princetonCounter++;
                princetonState = SAVE_DURATION; // Возвращаемся на шаг 2
            } else {
                // Ошибка в последовательности, полный сброс
                princetonState = RESET;
            }
        } else {
            // Если пришел ВЫСОКИЙ, когда мы ждали НИЗКИЙ - это ошибка
            princetonState = RESET;
        }
        break;
    }
}