#include "PowerSmartDecoder.h"

// ======================= ОТЛАДКА =======================
// #define DEBUG_POWER_SMART
// =======================================================


#ifdef DEBUG_POWER_SMART
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif


// --- Внутренние переменные модуля Power Smart ---
enum PS_ManchesterState {
    PS_MANCHESTER_RESET,
    PS_MANCHESTER_WAIT_HALF_PULSE,
};
volatile PS_ManchesterState ps_manchester_state = PS_MANCHESTER_RESET;
volatile uint64_t ps_rawData = 0;
volatile uint16_t ps_bit_counter = 0;
volatile bool ps_manchester_last_level = false;

// --- Константы протокола ---
#define PS_TE_SHORT 225
// ИСПРАВЛЕНИЕ 1: Увеличиваем допуск для стабильности
#define PS_TE_DELTA 160 // Было 120, теперь диапазон [65, 385]
#define PS_PACKET_HEADER        0xFD000000AA000000ULL
#define PS_PACKET_HEADER_MASK   0xFF000000FF000000ULL
#define PROTOCOL_ID_POWERSMART  14

// --- Вспомогательная функция для проверки контрольной суммы ---
static bool check_power_smart_packet(uint64_t packet) {
    uint16_t xxxx = (uint16_t)((packet >> 40) & 0xFFFF);
    uint16_t zzzz = (uint16_t)((packet >> 8)  & 0xFFFF);
    uint8_t  yy   = (uint8_t)((packet >> 32) & 0xFF);
    uint8_t  ww   = (uint8_t)(packet & 0xFF);

    bool first_check = (xxxx == (uint16_t)(~zzzz));
    bool second_check = (yy == (uint8_t)((~ww) - 1));

    DEBUG_PRINT("    [PS Check] xxxx=0x"); DEBUG_PRINT(xxxx, HEX);
    DEBUG_PRINT(", ~zzzz=0x"); DEBUG_PRINTLN((uint16_t)(~zzzz), HEX);
    DEBUG_PRINT("    [PS Check]   yy=0x"); DEBUG_PRINT(yy, HEX);
    DEBUG_PRINT(", (~ww)-1=0x"); DEBUG_PRINTLN((uint8_t)((~ww) - 1), HEX);

    return first_check && second_check;
}

static bool advance_manchester_ps(bool level, bool* decoded_bit) {
    bool bit_is_decoded = false;
    if (ps_manchester_state == PS_MANCHESTER_RESET) {
        ps_manchester_last_level = level;
        ps_manchester_state = PS_MANCHESTER_WAIT_HALF_PULSE;
        DEBUG_PRINTLN("  [MANCHESTER] State: RESET -> WAIT_HALF_PULSE");
    } else { 
        if (level != ps_manchester_last_level) {
            *decoded_bit = ps_manchester_last_level;
            bit_is_decoded = true;
            DEBUG_PRINT("  [MANCHESTER] DECODED BIT: "); DEBUG_PRINTLN(*decoded_bit);
        } else {
            DEBUG_PRINTLN("  [MANCHESTER] Error: two same-level half-pulses. Resetting state.");
        }
        ps_manchester_state = PS_MANCHESTER_RESET;
    }
    return bit_is_decoded;
}

/**
 * @brief Главная функция-обработчик с финальными исправлениями.
 */
void processPowerSmartSignal(bool level, uint32_t duration) {
    DEBUG_PRINT("[PS RX] L:"); DEBUG_PRINT(level); DEBUG_PRINT(", D:"); DEBUG_PRINT(duration);

    bool is_short = (duration > PS_TE_SHORT - PS_TE_DELTA) && (duration < PS_TE_SHORT + PS_TE_DELTA);

    if (!is_short) {
        if (ps_bit_counter > 0) {
            DEBUG_PRINTLN("\n>>> INVALID DURATION, RESETTING PACKET <<<\n");
        }
        ps_rawData = 0;
        ps_bit_counter = 0;
        ps_manchester_state = PS_MANCHESTER_RESET;
        return;
    }

    bool decoded_data_bit;
    if (advance_manchester_ps(level, &decoded_data_bit)) {
        bool final_bit = !decoded_data_bit;
        
        ps_rawData = (ps_rawData << 1) | final_bit;
        if (ps_bit_counter < 1000) ps_bit_counter++; // Защита от переполнения
        
        DEBUG_PRINT("    [PS Push] Bit Count: "); DEBUG_PRINT(ps_bit_counter);
        DEBUG_PRINT(", Bit Val: "); DEBUG_PRINT(final_bit);
        DEBUG_PRINT(" -> RawData: 0x");
        DEBUG_PRINT((uint32_t)(ps_rawData >> 32), HEX);
        DEBUG_PRINTLN((uint32_t)ps_rawData, HEX);
        
        // ИСПРАВЛЕНИЕ 2: Убираем лишний сброс. Проверка должна быть "скользящей".
        if (ps_bit_counter >= 64) {
            if ((ps_rawData & PS_PACKET_HEADER_MASK) == PS_PACKET_HEADER) {
                DEBUG_PRINTLN("      ====> HEADER FOUND! Validating checksum...");
                if (check_power_smart_packet(ps_rawData)) {
                    DEBUG_PRINTLN("        ====> SUCCESS! Packet is valid! <====");
                    barrierCodeMain = ps_rawData;
                    barrierBit = 64;
                    barrierProtocol = PROTOCOL_ID_POWERSMART;
                    newSignalReady = true;
                    
                    // Сбрасываем только после полного успеха
                    ps_rawData = 0;
                    ps_bit_counter = 0;
                    ps_manchester_state = PS_MANCHESTER_RESET;
                } else {
                    DEBUG_PRINTLN("        ====> ERROR: Checksum FAILED! <====");
                }
            } 
            // Больше не сбрасываем здесь, если заголовок не найден.
            // Просто продолжаем сдвигать биты.
        }
    } else {
       DEBUG_PRINTLN(""); 
    }
}