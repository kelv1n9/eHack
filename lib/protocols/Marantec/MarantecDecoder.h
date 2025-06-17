#ifndef MARANTEC_DECODER_H
#define MARANTEC_DECODER_H

#include <Arduino.h>

// --- Объявление главной функции-обработчика ---
void processMarantecSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные для результата ---
extern volatile uint64_t barrierCodeMain; // Для хранения 49-битного ключа
extern volatile uint8_t  barrierBit;      // Для длины ключа (всегда 49)
extern volatile uint8_t  barrierProtocol;
extern volatile boolean newSignalReady;

#endif // MARANTEC_DECODER_H