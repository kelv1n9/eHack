#ifndef CAME_TWEE_DECODER_H
#define CAME_TWEE_DECODER_H

#include <Arduino.h>

// --- Объявления функций ---
void processCameTweeSignal(bool level, uint32_t duration);

// --- Объявления общих переменных (используются в .ino файле) ---
// Мы по-прежнему используем общие переменные для унификации
extern volatile uint32_t barrierCodeMain; // Здесь будет 10-битный DIP-код
extern volatile uint8_t barrierBit;        // Здесь будет 4-битный код кнопки
extern volatile uint8_t barrierProtocol;

// --- Объявление флага захвата для этого модуля ---
extern volatile boolean cameTweeCaptured;

#endif