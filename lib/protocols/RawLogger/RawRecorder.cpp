#include "RawRecorder.h"

// --- Подключаем наше общее хранилище из .ino файла ---
#define RAW_BUFFER_SIZE 256
int32_t rawSignalBuffer[RAW_BUFFER_SIZE];
volatile uint16_t rawSignalLen = 0;
#define GD0_PIN_CC 19
// ----------------------------------------------------

// Константы для фильтрации шума
#define RAW_MIN_DURATION 80 
#define RAW_MAX_DURATION 50000 
#define PROTOCOL = 18

void processRawRecord(bool level, uint32_t duration) {
    // Если буфер уже заполнен, ничего не делаем
    if (rawSignalLen >= RAW_BUFFER_SIZE) {
        return;
    }
    
    // Игнорируем шум и слишком длинные паузы
    if (duration < RAW_MIN_DURATION || duration > RAW_MAX_DURATION) {
        return;
    }

    // Записываем данные в буфер
    if (level) {
        rawSignalBuffer[rawSignalLen] = duration;
    } else {
        rawSignalBuffer[rawSignalLen] = -duration;
    }
    rawSignalLen++;
}

void playRawSignal() {
    if (rawSignalLen == 0) {
        Serial.println("Buffer is empty. Nothing to play.");
        return;
    }

    Serial.print("Playing back ");
    Serial.print(rawSignalLen);
    Serial.println(" pulses...");

    // Выключаем прерывания на время отправки для точности таймингов
    noInterrupts();

    for (uint16_t i = 0; i < rawSignalLen; i++) {
        int32_t duration_us = rawSignalBuffer[i];
        
        if (duration_us > 0) {
            digitalWrite(GD0_PIN_CC, HIGH);
            delayMicroseconds(duration_us);
        } else {
            digitalWrite(GD0_PIN_CC, LOW);
            delayMicroseconds(-duration_us);
        }
    }
    digitalWrite(GD0_PIN_CC, LOW);

    // Включаем прерывания обратно
    interrupts();
    Serial.println("Playback finished.");
}