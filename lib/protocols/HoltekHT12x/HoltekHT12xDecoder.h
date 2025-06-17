#ifndef HOLTEK_HT12X_DECODER_H
#define HOLTEK_HT12X_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processHoltekHT12xSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Для адреса (8 бит)
extern volatile uint8_t barrierBit;        // Для кнопок (4 бита)
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif