#ifndef MAGELLAN_DECODER_H
#define MAGELLAN_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processMagellanSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Здесь будет 24-битный код (S/N + событие)
extern volatile uint8_t barrierBit;        // Здесь будет 8-битная контрольная сумма CRC8
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif