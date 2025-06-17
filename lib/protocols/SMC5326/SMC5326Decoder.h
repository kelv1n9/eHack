#ifndef SMC5326_DECODER_H
#define SMC5326_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processSmc5326Signal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Здесь будет 25-битный ключ
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif