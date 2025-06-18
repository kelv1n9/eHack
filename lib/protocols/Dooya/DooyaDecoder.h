#ifndef DOOYA_DECODER_H
#define DOOYA_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processDooyaSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Для серийного номера (24 бита)
extern volatile uint64_t barrierCodeAdd;  // Для канала (8 бит)
extern volatile uint8_t barrierBit;        // Для кода кнопки (8 бит)
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif