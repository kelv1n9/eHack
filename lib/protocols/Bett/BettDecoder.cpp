#include "BettDecoder.h"

// Раскомментируйте для отладки в Serial Monitor
// #define DEBUG_BETT

#ifdef DEBUG_BETT
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif


// --- Внутренние переменные и состояния модуля BETT ---
enum BettState {
    B_STATE_RESET,        // Ожидание длинной паузы-заголовка
    B_STATE_WAIT_PULSE,   // Ожидание импульса (длинного или короткого)
    B_STATE_WAIT_PAUSE,   // Ожидание паузы (длинной или короткой)
};

volatile BettState bett_state = B_STATE_RESET;
volatile uint32_t bett_rawData = 0;
volatile uint8_t  bett_bit_count = 0;

// --- Константы протокола ---
#define B_TE_SHORT 340
#define B_TE_LONG 2000
#define B_TE_DELTA 170  // Допуск для таймингов

// Пауза-заголовок ~15мс
#define B_SYNC_PAUSE_US (B_TE_SHORT * 44) 
#define B_SYNC_DELTA_US (B_TE_DELTA * 15)

#define PROTOCOL_ID_BETT 16 // Уникальный ID для этого протокола

/**
 * @brief Проверяет, завершен ли пакет, и сохраняет результат.
 */
static void check_and_finalize_bett_packet() {
    if (bett_bit_count == 18) {
        DEBUG_PRINTLN("      ====> 18 BITS RECEIVED, PACKET COMPLETE! <====");
        barrierCodeMain = bett_rawData;
        barrierBit = 18;
        barrierProtocol = PROTOCOL_ID_BETT;
        newSignalReady = true;
    } else {
        DEBUG_PRINT("      ====> PACKET INCOMPLETE (");
        DEBUG_PRINT(bett_bit_count);
        DEBUG_PRINTLN(" bits) <====");
    }
}


/**
 * @brief Главная функция-обработчик для протокола BETT.
 */
void processBettSignal(bool level, uint32_t duration) {
    DEBUG_PRINT("[BETT RX] State: "); DEBUG_PRINT(bett_state);
    DEBUG_PRINT(", L:"); DEBUG_PRINT(level);
    DEBUG_PRINT(", D:"); DEBUG_PRINTLN(duration);

    switch (bett_state) {
        case B_STATE_RESET:
            // Ищем длинную паузу-заголовок
            if (!level && (abs((int)duration - (int)B_SYNC_PAUSE_US) < B_SYNC_DELTA_US)) {
                DEBUG_PRINTLN("  [BETT] Sync pause found, starting decode.");
                bett_rawData = 0;
                bett_bit_count = 0;
                bett_state = B_STATE_WAIT_PULSE;
            }
            break;

        case B_STATE_WAIT_PULSE:
            // Ожидаем импульс
            if (level) {
                bool bit_val;
                if (abs((int)duration - (int)B_TE_LONG) < B_TE_DELTA * 3) {
                    bit_val = 1; // Длинный импульс
                } else if (abs((int)duration - (int)B_TE_SHORT) < B_TE_DELTA) {
                    bit_val = 0; // Короткий импульс
                } else {
                    DEBUG_PRINTLN("  [BETT] Invalid pulse duration, resetting.");
                    bett_state = B_STATE_RESET;
                    break;
                }
                
                bett_rawData = (bett_rawData << 1) | bit_val;
                bett_bit_count++;
                
                DEBUG_PRINT("    [BETT Push] Bit Count: "); DEBUG_PRINT(bett_bit_count);
                DEBUG_PRINT(", Bit Val: "); DEBUG_PRINT(bit_val);
                DEBUG_PRINT(" -> RawData: 0x"); DEBUG_PRINTLN(bett_rawData, HEX);

                bett_state = B_STATE_WAIT_PAUSE;
            } else {
                // Если пришла пауза, когда ждали импульс - это ошибка
                DEBUG_PRINTLN("  [BETT] Error: Expected pulse, got pause. Resetting.");
                bett_state = B_STATE_RESET;
            }
            break;

        case B_STATE_WAIT_PAUSE:
            // Ожидаем паузу
            if (!level) {
                // Сначала проверяем на длинную паузу-разделитель
                if (abs((int)duration - (int)B_SYNC_PAUSE_US) < B_SYNC_DELTA_US) {
                    DEBUG_PRINTLN("  [BETT] Re-sync pause found. Finalizing packet.");
                    check_and_finalize_bett_packet();
                    
                    // Начинаем новый пакет
                    bett_rawData = 0;
                    bett_bit_count = 0;
                    bett_state = B_STATE_WAIT_PULSE;
                    break;
                }

                // Если это не разделитель, проверяем, валидна ли обычная пауза
                bool pause_ok = (abs((int)duration - (int)B_TE_SHORT) < B_TE_DELTA) ||
                                (abs((int)duration - (int)B_TE_LONG) < B_TE_DELTA * 3);
                
                if (pause_ok) {
                    // Пауза корректна, ждем следующий импульс
                    bett_state = B_STATE_WAIT_PULSE;
                } else {
                    DEBUG_PRINTLN("  [BETT] Invalid pause duration, resetting.");
                    bett_state = B_STATE_RESET;
                }
            } else {
                // Если пришел импульс, когда ждали паузу - это ошибка
                DEBUG_PRINTLN("  [BETT] Error: Expected pause, got pulse. Resetting.");
                bett_state = B_STATE_RESET;
            }
            break;
    }
}