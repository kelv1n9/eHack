#ifndef BETT_DECODER_H
#define BETT_DECODER_H

#include <Arduino.h>

// --- Объявление главной функции-обработчика ---
void processBettSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные для результата ---
extern volatile uint64_t barrierCodeMain; // Для 18-битного ключа
extern volatile uint8_t  barrierBit;      // Для длины ключа (всегда 18)
extern volatile uint8_t  barrierProtocol;
extern volatile boolean newSignalReady;

#endif // BETT_DECODER_H