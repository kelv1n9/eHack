#ifndef CAME_DECODER_H
#define CAME_DECODER_H

#include <Arduino.h>

// --- Объявления функций ---
void processCameSignal(bool level, uint32_t duration);

// --- Объявления общих переменных (используются в .ino файле) ---
extern volatile boolean cameCaptured;
// Мы используем те же глобальные переменные `barrier...` для всех протоколов,
// чтобы унифицировать обработку в главном цикле.
extern volatile uint64_t  barrierCodeMain;
extern volatile uint8_t barrierBit;
extern volatile uint8_t barrierProtocol;

#endif