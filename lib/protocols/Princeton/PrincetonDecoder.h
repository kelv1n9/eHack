#ifndef PRINCETON_DECODER_H
#define PRINCETON_DECODER_H

#include <Arduino.h>

// --- Объявления функций ---
void processPrincetonSignal(bool level, uint32_t duration);

// --- Объявления общих переменных ---
extern volatile boolean princetonCaptured;
// Используем те же глобальные переменные `barrier...` для унификации
extern volatile uint64_t  barrierCodeMain;
extern volatile uint8_t barrierBit;
extern volatile uint8_t barrierProtocol;

#endif