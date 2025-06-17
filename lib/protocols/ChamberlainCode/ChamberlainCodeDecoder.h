#ifndef CHAMBERLAIN_CODE_DECODER_H
#define CHAMBERLAIN_CODE_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processChamberlainSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Здесь будет 7/8/9-битный DIP-код
extern volatile uint8_t barrierBit;        // Здесь мы сохраним длину ключа (7, 8 или 9)
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif