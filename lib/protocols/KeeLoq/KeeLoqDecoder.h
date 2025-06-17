#ifndef KEELOQ_DECODER_H
#define KEELOQ_DECODER_H

#include <Arduino.h>

// --- Объявления функций ---
void processKeeloqSignal(bool level, uint32_t duration);

// --- Объявления общих переменных ---
// Используем стандартные переменные для S/N и кнопки
extern volatile uint64_t  barrierCodeMain; // Здесь будет серийный номер (28 бит)
extern volatile uint8_t barrierBit;       // Здесь будет код кнопки (4 бита)
extern volatile uint8_t barrierProtocol;
// Добавляем отдельную переменную для счетчика KeeLoq
extern volatile uint16_t keeloqCounter;   // Здесь будет счетчик (16 бит)

// --- Объявление флага захвата для этого модуля ---
extern volatile boolean keeloqCaptured;

#endif