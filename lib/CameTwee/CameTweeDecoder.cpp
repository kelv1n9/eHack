#include "CameTweeDecoder.h"

// --- Объявление-прототип функции (Форвард-декларация) ---
// Эта строка говорит компилятору: "Не волнуйся, такая функция существует, ее код будет ниже"
void manchester_decoder_add_pulse(bool level);
// -------------------------------------------------------------------------------------

// --- Определение флага захвата ---
volatile boolean cameTweeCaptured = false;

// --- Внутренние переменные модуля ---
enum CameTweeState {
    TWEE_RESET,
    TWEE_DECODING,
};
volatile CameTweeState tweeState = TWEE_RESET;
volatile uint64_t tweeRawData = 0;
volatile uint8_t tweeCounter = 0;

// Константы
#define TWEE_HALF_BIT_PERIOD 500
#define TWEE_FULL_BIT_PERIOD 1000
#define TWEE_DELTA 300 // Погрешность
#define TWEE_INTER_PACKET_MIN_US 40000 // Минимальная пауза между пакетами

// Таблица для расшифровки
static const uint32_t came_twee_magic_numbers_xor[15] = {
    0x0E0E0E00, 0x1D1D1D11, 0x2C2C2C22, 0x3B3B3B33, 0x4A4A4A44,
    0x59595955, 0x68686866, 0x77777777, 0x86868688, 0x95959599,
    0xA4A4A4AA, 0xB3B3B3BB, 0xC2C2C2CC, 0xD1D1D1DD, 0xE0E0E0EE,
};

// --- Вспомогательные функции ---

// Проверка длительности
static bool is_duration_correct(uint32_t duration, uint32_t base) {
    return (duration > base - TWEE_DELTA) && (duration < base + TWEE_DELTA);
}

// Реверс бит
static uint16_t reverse16(uint16_t x) {
    x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
    x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
    x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
    return((x >> 8) | (x << 8));
}

// Функция расшифровки
static void decryptCameTwee(uint64_t rawData) {
    uint8_t cnt_parcel = (uint8_t)(rawData & 0xF);
    if (cnt_parcel > 14) return;

    uint32_t data = (uint32_t)(rawData & 0x0FFFFFFFF);
    data = (data ^ came_twee_magic_numbers_xor[cnt_parcel]);
    data /= 4;

    barrierBit = (data >> 4) & 0x0F;
    uint16_t dip_raw = data >> 16;
    uint16_t dip_reversed = reverse16(dip_raw);
    barrierCodeMain = dip_reversed >> 6;
}

// --- Главная функция-обработчик (новая версия) ---
void processCameTweeSignal(bool level, uint32_t duration) {
    // Длинная пауза всегда сбрасывает автомат и начинает новый прием
    if (!level && duration > TWEE_INTER_PACKET_MIN_US) {
        tweeState = TWEE_DECODING;
        tweeCounter = 0;
        tweeRawData = 0;
        // Важно: нужно "скормить" нашему декодеру первый импульс,
        // который является концом длинной паузы. Это синхронизирует его.
        manchester_decoder_add_pulse(level); 
        return; 
    }

    if (tweeState != TWEE_DECODING) {
        return; // Если мы не в режиме декодирования, игнорируем все
    }
    
    // --- Логика декодирования Манчестера ---
    if (is_duration_correct(duration, TWEE_HALF_BIT_PERIOD)) {
        manchester_decoder_add_pulse(level);
    } else if (is_duration_correct(duration, TWEE_FULL_BIT_PERIOD)) {
        manchester_decoder_add_pulse(level);
        manchester_decoder_add_pulse(level);
    } else {
        tweeState = TWEE_RESET;
    }
}

// Эта функция - "движок" нашего ручного декодера манчестера.
void manchester_decoder_add_pulse(bool level) {
    static uint8_t pulse_count = 0;
    static bool pulse_buffer[2];

    pulse_buffer[pulse_count] = level;
    pulse_count++;

    if (pulse_count == 2) {
        if (pulse_buffer[0] != pulse_buffer[1]) {
            bool decoded_bit = pulse_buffer[0];
            
            tweeRawData = (tweeRawData << 1) | decoded_bit;
            tweeCounter++;
        } else {
            tweeState = TWEE_RESET;
        }
        pulse_count = 0;

        if (tweeCounter == 54) {
            decryptCameTwee(tweeRawData);
            barrierProtocol = 4;
            cameTweeCaptured = true;
            tweeState = TWEE_RESET;
        }
    }
}