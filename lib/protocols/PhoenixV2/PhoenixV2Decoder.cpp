#include "PhoenixV2Decoder.h"

// Раскомментируйте для отладки в Serial Monitor
// #define DEBUG_PHOENIX_V2

#ifdef DEBUG_PHOENIX_V2
#define PH_DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define PH_DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define PH_DEBUG_PRINT(...)
#define PH_DEBUG_PRINTLN(...)
#endif


// --- Внутренние переменные и состояния модуля Phoenix V2 ---
enum PhoenixV2State {
    P_STATE_RESET,            // Ожидание преамбулы (~25.6 мс)
    P_STATE_WAIT_START_BIT,   // Ожидание стартового импульса (~2.5 мс)
    P_STATE_WAIT_PAUSE,       // Ожидание паузы перед импульсом
    P_STATE_WAIT_PULSE,       // Ожидание импульса и определение бита
};

volatile PhoenixV2State ph_state = P_STATE_RESET;
volatile uint64_t ph_rawData = 0;
volatile uint8_t  ph_bit_count = 0;
volatile uint32_t ph_last_pause_dur = 0;

// --- Константы протокола ---
#define P_TE_SHORT 427
#define P_TE_LONG 853
#define P_TE_DELTA 150 

#define P_PREAMBLE_US (P_TE_SHORT * 60)
// ИСПРАВЛЕНИЕ: Увеличиваем допуск для преамбулы, чтобы соответствовать реальному сигналу
#define P_PREAMBLE_DELTA (P_TE_DELTA * 40) // Было * 30
#define P_START_PULSE_US (P_TE_SHORT * 6)
#define P_START_PULSE_DELTA (P_TE_DELTA * 4)
#define P_RESYNC_PAUSE_US (P_TE_SHORT * 10)

#define PROTOCOL_ID_PHOENIX_V2 14

/**
 * @brief Главная функция-обработчик для протокола Phoenix V2.
 */
void processPhoenixV2Signal(bool level, uint32_t duration) {
    PH_DEBUG_PRINT("[PhoenixV2 RX] State: "); PH_DEBUG_PRINT(ph_state);
    PH_DEBUG_PRINT(", L:"); PH_DEBUG_PRINT(level);
    PH_DEBUG_PRINT(", D:"); PH_DEBUG_PRINTLN(duration);

    switch (ph_state) {
        case P_STATE_RESET:
            // 1. Ищем длинную паузу-преамбулу
            if (!level && (abs((int)duration - (int)P_PREAMBLE_US) < P_PREAMBLE_DELTA)) {
                PH_DEBUG_PRINTLN("  [PhoenixV2] Preamble found, waiting for start bit.");
                ph_state = P_STATE_WAIT_START_BIT;
            }
            break;

        case P_STATE_WAIT_START_BIT:
            // 2. Ожидаем стартовый импульс
            if (level && (abs((int)duration - (int)P_START_PULSE_US) < P_START_PULSE_DELTA)) {
                PH_DEBUG_PRINTLN("  [PhoenixV2] Start bit found, starting decode.");
                ph_rawData = 0;
                ph_bit_count = 0;
                ph_state = P_STATE_WAIT_PAUSE;
            } else {
                ph_state = P_STATE_RESET; 
            }
            break;

        case P_STATE_WAIT_PAUSE:
            // 3. Ожидаем паузу
            if (!level) {
                // Проверяем на конец передачи (пауза между повторами)
                if (duration > P_RESYNC_PAUSE_US) {
                    PH_DEBUG_PRINTLN("  [PhoenixV2] End of packet / Re-sync pause detected.");
                    if (ph_bit_count == 52) {
                        PH_DEBUG_PRINTLN("      ====> 52 BITS, PACKET VALID <====");
                        barrierCodeMain = ph_rawData;
                        barrierBit = 52;
                        barrierProtocol = PROTOCOL_ID_PHOENIX_V2;
                        newSignalReady = true;
                    }
                    // После паузы-разделителя снова ждем стартовый бит
                    ph_state = P_STATE_WAIT_START_BIT;
                } else {
                    // Это обычная пауза, сохраняем ее длительность и ждем импульс
                    ph_last_pause_dur = duration;
                    ph_state = P_STATE_WAIT_PULSE;
                }
            } else {
                PH_DEBUG_PRINTLN("  [PhoenixV2] Error: Expected pause, got pulse. Resetting.");
                ph_state = P_STATE_RESET;
            }
            break;

        case P_STATE_WAIT_PULSE:
            // 4. Ожидаем импульс и по паре "пауза-импульс" определяем бит
            if (level) {
                bool original_bit;
                // Сигнал '0' (короткая пауза + длинный импульс) -> исходный бит '1'
                if ((abs((int)ph_last_pause_dur - P_TE_SHORT) < P_TE_DELTA) &&
                    (abs((int)duration - P_TE_LONG) < P_TE_DELTA * 3)) {
                    original_bit = 1; 
                // Сигнал '1' (длинная пауза + короткий импульс) -> исходный бит '0'
                } else if ((abs((int)ph_last_pause_dur - P_TE_LONG) < P_TE_DELTA * 3) &&
                           (abs((int)duration - P_TE_SHORT) < P_TE_DELTA)) {
                    original_bit = 0;
                } else {
                    PH_DEBUG_PRINTLN("  [PhoenixV2] Invalid pulse/pause pair, resetting.");
                    ph_state = P_STATE_RESET;
                    break;
                }
                
                ph_rawData = (ph_rawData << 1) | original_bit;
                ph_bit_count++;
                
                PH_DEBUG_PRINT("    [PhoenixV2 Push] Bit Count: "); PH_DEBUG_PRINT(ph_bit_count);
                PH_DEBUG_PRINT(", Original Bit Val: "); PH_DEBUG_PRINT(original_bit);
                PH_DEBUG_PRINT(" -> RawData: 0x");
                PH_DEBUG_PRINT((uint32_t)(ph_rawData >> 32), HEX);
                PH_DEBUG_PRINTLN((uint32_t)ph_rawData, HEX);
                
                if(ph_bit_count == 52) {
                     PH_DEBUG_PRINTLN("      ====> 52 BITS RECEIVED, waiting for re-sync pause to validate. <====");
                }

                ph_state = P_STATE_WAIT_PAUSE;
            } else {
                PH_DEBUG_PRINTLN("  [PhoenixV2] Error: Expected pulse, got pulse. Resetting.");
                ph_state = P_STATE_RESET;
            }
            break;
    }
}