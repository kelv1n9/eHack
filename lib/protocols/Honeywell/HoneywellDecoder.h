#ifndef HONEYWELL_DECODER_H
#define HONEYWELL_DECODER_H

#include <Arduino.h>

// --- Объявление функции ---
void processHoneywellSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные ---
extern volatile uint64_t barrierCodeMain; // Для всего 48-битного ключа
extern volatile uint8_t barrierProtocol;
extern volatile boolean newSignalReady;

#endif