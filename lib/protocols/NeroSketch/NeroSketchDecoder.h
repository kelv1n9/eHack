#ifndef NERO_SKETCH_DECODER_H
#define NERO_SKETCH_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processNeroSketchSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Используем 64-битную переменную для 40-битного кода
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif