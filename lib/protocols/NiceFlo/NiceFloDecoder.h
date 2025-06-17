#ifndef NICE_FLO_DECODER_H // Это "стражник", чтобы файл не подключился дважды
#define NICE_FLO_DECODER_H

#include <Arduino.h> // Подключаем стандартные типы Arduino

// --- Объявления функций ---
// Мы "обещаем", что где-то в проекте есть такая функция
void processNiceSignal(bool level, uint32_t duration);

// --- Объявления общих переменных ---
// Ключевое слово 'extern' говорит, что эти переменные созданы где-то в другом месте,
// но мы хотим иметь к ним доступ.
extern volatile boolean niceCaptured;
extern volatile uint64_t  barrierCodeMain;
extern volatile uint8_t barrierBit;
extern volatile uint8_t barrierProtocol;

#endif