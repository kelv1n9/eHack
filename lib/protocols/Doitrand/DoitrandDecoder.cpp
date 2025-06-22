#include "DoitrandDecoder.h"

// Раскомментируйте для отладки в Serial Monitor
// #define DEBUG_DOITRAND

#ifdef DEBUG_DOITRAND
#define DT_DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DT_DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DT_DEBUG_PRINT(...)
#define DT_DEBUG_PRINTLN(...)
#endif


// --- Внутренние переменные и состояния модуля Doitrand ---
enum DoitrandState {
    D_STATE_RESET,            // Ожидание преамбулы (~25 мс)
    D_STATE_WAIT_START_BIT,   // Ожидание стартового импульса (~700 мкс)
    D_STATE_WAIT_PAUSE,       // Ожидание паузы перед импульсом
    D_STATE_WAIT_PULSE,       // Ожидание импульса и определение бита
};

volatile DoitrandState doitrand_state = D_STATE_RESET;
volatile uint64_t doitrand_rawData = 0;
volatile uint8_t  doitrand_bit_count = 0;
volatile uint32_t doitrand_last_pause_dur = 0;

// --- Константы протокола ---
#define D_TE_SHORT 400
#define D_TE_LONG 1100
#define D_TE_DELTA 170

#define D_PREAMBLE_US (D_TE_SHORT * 62)
#define D_PREAMBLE_DELTA (D_TE_DELTA * 30)
#define D_START_PULSE_US (D_TE_SHORT * 2)
#define D_START_PULSE_DELTA (D_TE_DELTA * 3)
#define D_RESYNC_PAUSE_US (D_TE_SHORT * 10)

#define PROTOCOL_ID_DOITRAND 13

/**
 * @brief Главная функция-обработчик для протокола Doitrand.
 */
void processDoitrandSignal(bool level, uint32_t duration) {
    DT_DEBUG_PRINT("[Doitrand RX] State: "); DT_DEBUG_PRINT(doitrand_state);
    DT_DEBUG_PRINT(", L:"); DT_DEBUG_PRINT(level);
    DT_DEBUG_PRINT(", D:"); DT_DEBUG_PRINTLN(duration);

    switch (doitrand_state) {
        case D_STATE_RESET:
            // 1. Ищем длинную паузу-преамбулу
            if (!level && (abs((int)duration - (int)D_PREAMBLE_US) < D_PREAMBLE_DELTA)) {
                DT_DEBUG_PRINTLN("  [Doitrand] Preamble found, waiting for start bit.");
                doitrand_state = D_STATE_WAIT_START_BIT;
            }
            break;

        case D_STATE_WAIT_START_BIT:
            // 2. Ожидаем стартовый импульс
            if (level && (abs((int)duration - (int)D_START_PULSE_US) < D_START_PULSE_DELTA)) {
                DT_DEBUG_PRINTLN("  [Doitrand] Start bit found, starting decode.");
                doitrand_rawData = 0;
                doitrand_bit_count = 0;
                doitrand_state = D_STATE_WAIT_PAUSE;
            } else {
                doitrand_state = D_STATE_RESET; 
            }
            break;

        case D_STATE_WAIT_PAUSE:
            // 3. Ожидаем паузу
            if (!level) {
                // Проверяем на конец передачи (пауза между повторами)
                if (duration > D_RESYNC_PAUSE_US) {
                    DT_DEBUG_PRINTLN("  [Doitrand] End of packet / Re-sync pause detected.");
                    if (doitrand_bit_count == 37) {
                        DT_DEBUG_PRINTLN("      ====> 37 BITS, PACKET VALID <====");
                        barrierCodeMain = doitrand_rawData;
                        barrierBit = 37;
                        barrierProtocol = PROTOCOL_ID_DOITRAND;
                        newSignalReady = true;
                    }
                    // После паузы-разделителя снова ждем стартовый бит
                    doitrand_state = D_STATE_WAIT_START_BIT;
                } else {
                    // Это обычная пауза, сохраняем ее длительность и ждем импульс
                    doitrand_last_pause_dur = duration;
                    doitrand_state = D_STATE_WAIT_PULSE;
                }
            } else {
                DT_DEBUG_PRINTLN("  [Doitrand] Error: Expected pause, got pulse. Resetting.");
                doitrand_state = D_STATE_RESET;
            }
            break;

        case D_STATE_WAIT_PULSE:
            // 4. Ожидаем импульс и по паре "пауза-импульс" определяем бит
            if (level) {
                bool bit_val;
                // '0' = короткая пауза + длинный импульс
                if ((abs((int)doitrand_last_pause_dur - D_TE_SHORT) < D_TE_DELTA) &&
                    (abs((int)duration - D_TE_LONG) < D_TE_DELTA * 3)) {
                    bit_val = 0;
                // '1' = длинная пауза + короткий импульс
                } else if ((abs((int)doitrand_last_pause_dur - D_TE_LONG) < D_TE_DELTA * 3) &&
                           (abs((int)duration - D_TE_SHORT) < D_TE_DELTA)) {
                    bit_val = 1;
                } else {
                    DT_DEBUG_PRINTLN("  [Doitrand] Invalid pulse/pause pair, resetting.");
                    doitrand_state = D_STATE_RESET;
                    break;
                }
                
                doitrand_rawData = (doitrand_rawData << 1) | bit_val;
                doitrand_bit_count++;
                
                DT_DEBUG_PRINT("    [Doitrand Push] Bit Count: "); DT_DEBUG_PRINT(doitrand_bit_count);
                DT_DEBUG_PRINT(", Bit Val: "); DT_DEBUG_PRINT(bit_val);
                DT_DEBUG_PRINT(" -> RawData: 0x");
                DT_DEBUG_PRINT((uint32_t)(doitrand_rawData >> 32), HEX);
                DT_DEBUG_PRINTLN((uint32_t)doitrand_rawData, HEX);

                // Возвращаемся в ожидание следующей паузы
                doitrand_state = D_STATE_WAIT_PAUSE;
            } else {
                DT_DEBUG_PRINTLN("  [Doitrand] Error: Expected pulse, got pause. Resetting.");
                doitrand_state = D_STATE_RESET;
            }
            break;
    }
}