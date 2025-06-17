#ifndef LINEAR_DECODER_H
#define LINEAR_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processLinearSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain;
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif