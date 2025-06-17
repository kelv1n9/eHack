#ifndef DOITRAND_DECODER_H
#define DOITRAND_DECODER_H

#include <Arduino.h>

// --- Объявление главной функции-обработчика ---
void processDoitrandSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные для результата ---
extern volatile uint64_t barrierCodeMain;
extern volatile uint8_t  barrierBit;
extern volatile uint8_t  barrierProtocol;
extern volatile boolean newSignalReady;

#endif // DOITRAND_DECODER_H