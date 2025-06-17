#ifndef POWER_SMART_DECODER_H
#define POWER_SMART_DECODER_H

#include <Arduino.h>

// --- Объявление главной функции-обработчика ---
void processPowerSmartSignal(bool level, uint32_t duration);

// --- Ссылки на ОБЩИЕ переменные для результата ---
// Эти переменные должны быть определены в вашем главном .ino файле
extern volatile uint64_t barrierCodeMain; // Для хранения полного 64-битного пакета
extern volatile uint8_t  barrierBit;      // Для длины ключа (всегда 64)
extern volatile uint8_t  barrierProtocol; // Для ID протокола
extern volatile boolean newSignalReady;   // Флаг готовности нового сигнала

#endif // POWER_SMART_DECODER_H