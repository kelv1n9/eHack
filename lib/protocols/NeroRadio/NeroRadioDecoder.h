#ifndef NERO_RADIO_DECODER_H
#define NERO_RADIO_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processNeroRadioSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Используем 64-битную переменную для 56-битного кода
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif