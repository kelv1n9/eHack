#include "MarantecDecoder.h"

// ======================= ОТЛАДКА =======================
// #define DEBUG_MARANTEC
// =======================================================

#ifdef DEBUG_MARANTEC
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

// --- Внутренние переменные и состояния модуля Marantec ---
enum MarantecState {
    M_STATE_RESET,      // Ожидание длинной паузы-заголовка
    M_STATE_DECODING,   // Прием и декодирование бит
};

enum M_ManchesterEvent {
    M_EVENT_RESET,
    M_EVENT_SHORT_LOW,
    M_EVENT_LONG_LOW,
    M_EVENT_SHORT_HIGH,
    M_EVENT_LONG_HIGH,
    M_EVENT_RE_SYNC, // Специальное событие для ресинхронизации
};

enum M_ManchesterState {
    M_MANCHESTER_RESET,
    M_MANCHESTER_WAIT,
    M_MANCHESTER_GOT_HALF_0,
    M_MANCHESTER_GOT_HALF_1,
};

volatile MarantecState marantec_state = M_STATE_RESET;
volatile M_ManchesterState marantec_manchester_state = M_MANCHESTER_RESET;
volatile uint64_t marantec_rawData = 0;
volatile uint8_t  marantec_bit_count = 0;

// --- Константы протокола ---
#define M_TE_SHORT 1000
#define M_TE_LONG  2000
#define M_TE_DELTA 350  // Увеличенный допуск для стабильности
#define M_HEADER_PAUSE_US 10000
#define M_RESYNC_PAUSE_US 4200
#define PROTOCOL_ID_MARANTEC 12

/**
 * @brief Точный порт манчестер-декодера Flipper, управляемый событиями.
 */
static bool advance_manchester_marantec(M_ManchesterEvent event, bool* decoded_bit) {
    bool bit_is_decoded = false;
    switch (marantec_manchester_state) {
        case M_MANCHESTER_RESET:
            // Сбрасываемся и сразу переходим в ожидание
            marantec_manchester_state = M_MANCHESTER_WAIT;
            // fallthrough
        case M_MANCHESTER_WAIT:
            if (event == M_EVENT_SHORT_LOW) marantec_manchester_state = M_MANCHESTER_GOT_HALF_0;
            else if (event == M_EVENT_SHORT_HIGH) marantec_manchester_state = M_MANCHESTER_GOT_HALF_1;
            else if (event == M_EVENT_LONG_LOW) { *decoded_bit = 0; bit_is_decoded = true; }
            else if (event == M_EVENT_LONG_HIGH) { *decoded_bit = 1; bit_is_decoded = true; }
            break;
        case M_MANCHESTER_GOT_HALF_0: // Была короткая пауза
            if (event == M_EVENT_SHORT_HIGH) { *decoded_bit = 0; bit_is_decoded = true; } // L->H = 0
            marantec_manchester_state = M_MANCHESTER_WAIT;
            break;
        case M_MANCHESTER_GOT_HALF_1: // Был короткий импульс
            if (event == M_EVENT_SHORT_LOW) { *decoded_bit = 1; bit_is_decoded = true; } // H->L = 1
            marantec_manchester_state = M_MANCHESTER_WAIT;
            break;
    }
    return bit_is_decoded;
}

static void check_and_finalize_packet() {
    if (marantec_bit_count == 49) {
        DEBUG_PRINTLN("      ====> 49 BITS RECEIVED, PACKET COMPLETE! <====");
        barrierCodeMain = marantec_rawData;
        barrierBit = 49;
        barrierProtocol = PROTOCOL_ID_MARANTEC;
        newSignalReady = true;
    }
}

/**
 * @brief Главная функция-обработчик для протокола Marantec.
 */
void processMarantecSignal(bool level, uint32_t duration) {
    // 1. Ищем заголовок, если мы в состоянии сброса
    if (marantec_state == M_STATE_RESET) {
        if (!level && (abs((int)duration - (int)M_HEADER_PAUSE_US) < M_TE_DELTA * 8)) {
            DEBUG_PRINTLN("\n>>> MARANTEC HEADER FOUND (10ms pause) <<<\n");
            marantec_state = M_STATE_DECODING;
            marantec_rawData = 1; // Первый бит всегда 1
            marantec_bit_count = 1;
            marantec_manchester_state = M_MANCHESTER_RESET;
        }
        return; // Если не заголовок, ничего не делаем
    }

    // 2. Если мы в состоянии декодирования, анализируем сигнал
    M_ManchesterEvent event = M_EVENT_RESET;
    if (!level) { // Паузы
        if (abs((int)duration - M_TE_SHORT) < M_TE_DELTA) event = M_EVENT_SHORT_LOW;
        else if (abs((int)duration - M_TE_LONG) < M_TE_DELTA) event = M_EVENT_LONG_LOW;
        else if (duration > M_RESYNC_PAUSE_US) event = M_EVENT_RE_SYNC;
    } else { // Импульсы
        if (abs((int)duration - M_TE_SHORT) < M_TE_DELTA) event = M_EVENT_SHORT_HIGH;
        else if (abs((int)duration - M_TE_LONG) < M_TE_DELTA) event = M_EVENT_LONG_HIGH;
    }

    DEBUG_PRINT("[Marantec RX] L:"); DEBUG_PRINT(level);
    DEBUG_PRINT(" D:"); DEBUG_PRINT(duration);
    DEBUG_PRINT(" -> Ev: "); DEBUG_PRINTLN(event);

    if (event == M_EVENT_RESET) {
        DEBUG_PRINTLN(">>> INVALID DURATION, RESETTING PROTOCOL <<<");
        marantec_state = M_STATE_RESET;
        return;
    }

    if (event == M_EVENT_RE_SYNC) {
        DEBUG_PRINTLN(">>> RE-SYNC PAUSE, finalizing old packet and starting new. <<<");
        check_and_finalize_packet(); // Проверяем, не собрался ли пакет перед паузой
        // Начинаем новый пакет
        marantec_rawData = 1;
        marantec_bit_count = 1;
        marantec_manchester_state = M_MANCHESTER_RESET;
        return;
    }

    // Передаем событие в декодер манчестера
    bool decoded_bit;
    if (advance_manchester_marantec(event, &decoded_bit)) {
        marantec_rawData = (marantec_rawData << 1) | decoded_bit;
        marantec_bit_count++;
        DEBUG_PRINT("    [Marantec Push] Bit Count: "); DEBUG_PRINT(marantec_bit_count);
        DEBUG_PRINT(", Bit Val: "); DEBUG_PRINT(decoded_bit);
        DEBUG_PRINT(" -> RawData: 0x");
        DEBUG_PRINT((uint32_t)(marantec_rawData >> 32), HEX);
        DEBUG_PRINTLN((uint32_t)marantec_rawData, HEX);

        check_and_finalize_packet();
    }
}