#ifndef ANSONIC_DECODER_H
#define ANSONIC_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processAnsonicSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Здесь будет 10-битный DIP-код
extern volatile uint8_t barrierBit;        // Здесь будет 2-битный код кнопки
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif