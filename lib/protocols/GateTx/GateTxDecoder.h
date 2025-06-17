#ifndef GATE_TX_DECODER_H
#define GATE_TX_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processGateTxSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Используем uint64_t для совместимости
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif