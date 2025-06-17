#include "NiceFloDecoder.h" // Подключаем наш заголовочный файл

// --- Определение глобальных переменных, объявленных в .h файле ---
// Здесь мы создаем эти переменные в памяти
extern volatile boolean newSignalReady;

// --- Внутренние переменные модуля (невидимы для других файлов) ---
enum NiceDecoderState {
  NICE_STATE_RESET,
  NICE_STATE_WAIT_START_BIT,
  NICE_STATE_READ_DATA
};

volatile NiceDecoderState niceState = NICE_STATE_RESET;
volatile uint32_t niceCode = 0;
volatile uint8_t niceCounter = 0;
volatile uint32_t niceLastLowDuration = 0;

// Константы
#define NICE_TE_SHORT 700
#define NICE_TE_LONG 1400
#define NICE_TE_DELTA 200
#define NICE_HEADER_DURATION (NICE_TE_SHORT * 36)

// --- Реализация функций ---

// Вспомогательная функция (видна только внутри этого файла)
static boolean CheckValueNice(uint16_t base, uint16_t value) {
  return (value > base - NICE_TE_DELTA) && (value < base + NICE_TE_DELTA);
}

// Реализация главной функции-декодера
void processNiceSignal(bool level, uint32_t duration) {
  switch (niceState) {
    case NICE_STATE_RESET:
      if (!level && (duration > NICE_HEADER_DURATION - (NICE_TE_DELTA * 10))) {
        niceState = NICE_STATE_WAIT_START_BIT;
      }
      break;

    case NICE_STATE_WAIT_START_BIT:
      if (level && CheckValueNice(NICE_TE_SHORT, duration)) {
        niceCode = 0;
        niceCounter = 0;
        niceState = NICE_STATE_READ_DATA;
      } else {
        niceState = NICE_STATE_RESET;
      }
      break;

    case NICE_STATE_READ_DATA:
      if (!level) {
        if (duration > 2500 && (niceCounter == 12 || niceCounter == 24)) {
            barrierProtocol = 0;
            barrierCodeMain = niceCode;
            barrierBit = niceCounter;
            newSignalReady = true;
            niceState = NICE_STATE_RESET;
        } else {
            niceLastLowDuration = duration;
        }
      } else {
        if (CheckValueNice(NICE_TE_LONG, niceLastLowDuration) && CheckValueNice(NICE_TE_SHORT, duration)) {
          niceCode = (niceCode << 1) | 1;
          niceCounter++;
        } else if (CheckValueNice(NICE_TE_SHORT, niceLastLowDuration) && CheckValueNice(NICE_TE_LONG, duration)) {
          niceCode = (niceCode << 1) | 0;
          niceCounter++;
        } else {
          niceState = NICE_STATE_RESET;
        }
        if (niceCounter >= 25) {
            niceState = NICE_STATE_RESET;
        }
      }
      break;
  }
}