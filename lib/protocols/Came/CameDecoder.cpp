#include "CameDecoder.h"

// --- Определение общих переменных ---
extern volatile boolean newSignalReady; 

// --- Внутренние переменные модуля ---
enum CameDecoderState {
  CAME_STATE_RESET,
  CAME_STATE_WAIT_START_BIT,
  CAME_STATE_READ_DATA
};
volatile CameDecoderState cameState = CAME_STATE_RESET;
volatile uint32_t cameCode = 0;
volatile uint8_t cameCounter = 0;
volatile uint32_t cameLastLowDuration = 0;

// Константы
#define CAME_TE_SHORT 320
#define CAME_TE_LONG 640
#define CAME_TE_DELTA 150 // Допустимая погрешность

// Длительность заголовка для CAME довольно большая
#define CAME_HEADER_MIN_DURATION 10000

// --- Реализация функций ---
static boolean CheckValueCame(uint16_t base, uint16_t value) {
  return (value > base - CAME_TE_DELTA) && (value < base + CAME_TE_DELTA);
}

void processCameSignal(bool level, uint32_t duration) {
  switch (cameState) {
    case CAME_STATE_RESET:
      // В состоянии сброса ищем только заголовок: очень длинную паузу (LOW)
      if (!level && duration > CAME_HEADER_MIN_DURATION) {
        cameState = CAME_STATE_WAIT_START_BIT;
      }
      break;

    case CAME_STATE_WAIT_START_BIT:
      // Ищем стартовый бит: короткий импульс (HIGH)
      if (level && CheckValueCame(CAME_TE_SHORT, duration)) {
        cameCode = 0;
        cameCounter = 0;
        cameState = CAME_STATE_READ_DATA;
      } else {
        // Если пришло что-то другое, это не CAME пакет, сбрасываемся.
        // Но не сразу, а если это не еще один длинный импульс заголовка.
        if (!level && duration > CAME_HEADER_MIN_DURATION) {
            // Остаемся в этом же состоянии
        } else {
           cameState = CAME_STATE_RESET;
        }
      }
      break;

    case CAME_STATE_READ_DATA:
      if (!level) { // Если это пауза (LOW)
        // Проверяем, не является ли эта пауза концом пакета
        if (duration > CAME_TE_SHORT * 4) { // Конец пакета (пауза > ~1280us)
            if (cameCounter == 12 || cameCounter == 24) {
                // Пакет успешно принят!
                barrierProtocol = 1; // Protocol for CAME
                barrierCodeMain = cameCode;
                barrierBit = cameCounter;
                newSignalReady = true;
            }
            // Сбрасываем автомат для приема следующего
            cameState = CAME_STATE_RESET;
        } else {
            // Это обычная пауза между битами, сохраняем ее длительность
            cameLastLowDuration = duration;
        }
      } else { // Если это импульс (HIGH)
        // Декодируем бит, используя сохраненную длительность паузы и текущую длительность импульса
        if (CheckValueCame(CAME_TE_SHORT, cameLastLowDuration) && CheckValueCame(CAME_TE_LONG, duration)) {
          // Бит "0": короткая пауза + длинный импульс
          cameCode = (cameCode << 1) | 0;
          cameCounter++;
        } else if (CheckValueCame(CAME_TE_LONG, cameLastLowDuration) && CheckValueCame(CAME_TE_SHORT, duration)) {
          // Бит "1": длинная пауза + короткий импульс
          cameCode = (cameCode << 1) | 1;
          cameCounter++;
        } else {
          // Последовательность нарушена, сбрасываем автомат
          cameState = CAME_STATE_RESET;
        }

        // Защита от переполнения
        if (cameCounter > 24) {
            cameState = CAME_STATE_RESET;
        }
      }
      break;
  }
}