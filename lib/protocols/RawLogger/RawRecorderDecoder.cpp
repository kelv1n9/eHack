#include "RawRecorderDecoder.h"

// --- Константы ---
// Игнорируем очень короткие импульсы (помехи)
#define RAW_MIN_DURATION 80 
// Игнорируем очень длинные паузы между передачами, чтобы не засорять лог
#define RAW_MAX_DURATION 50000 

/**
 * @brief Просто выводит в Serial порт уровень и длительность каждого принятого импульса
 * в формате, совместимом с нашим передатчиком BinRAW.
 */
void processRawRecordSignal(bool level, uint32_t duration) {
    // Игнорируем слишком короткие и слишком длинные импульсы
    if (duration < RAW_MIN_DURATION || duration > RAW_MAX_DURATION) {
        // Выводим перевод строки, чтобы разделить пакеты в логе
        Serial.println(); 
        return;
    }

    // Если это была пауза (низкий уровень), ставим впереди минус
    if (!level) {
        Serial.print("-");
    }
    // Печатаем саму длительность
    Serial.print(duration);
    // Ставим запятую и пробел для удобного копирования
    Serial.print(", ");
}