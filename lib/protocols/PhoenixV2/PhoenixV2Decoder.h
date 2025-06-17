#ifndef PHOENIX_V2_DECODER_H
#define PHOENIX_V2_DECODER_H

#include <Arduino.h>

// --- Объявление главной функции-обработчика ---
void processPhoenixV2Signal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные для результата ---
extern volatile uint64_t barrierCodeMain; // Для 52-битного ключа
extern volatile uint8_t  barrierBit;      // Для длины ключа (всегда 52)
extern volatile uint8_t  barrierProtocol;
extern volatile boolean newSignalReady;

#endif // PHOENIX_V2_DECODER_H