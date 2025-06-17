#include "KeeLoqDecoder.h"

// --- Определение флага захвата ---
extern volatile boolean newSignalReady; 

// --- Внутренние переменные модуля ---
enum KeeloqState {
    KEELOQ_RESET,
    KEELOQ_SAVE_DURATION,
    KEELOQ_CHECK_DURATION
};
volatile KeeloqState keeloqState = KEELOQ_RESET;
volatile uint64_t keeloqRawData = 0;
volatile uint8_t keeloqBitCount = 0;
volatile uint32_t keeloqLastDuration = 0;
volatile uint16_t keeloqCounter;

// --- ПОРТИРОВАННАЯ КРИПТОГРАФИЯ ---
const uint64_t manufacturer_key = 0x0123456789ABCDEFULL; // <--- ЗАМЕНИТЬ НА РЕАЛЬНЫЙ КЛЮЧ
const uint32_t KEELOQ_NLF = 0x3A5C742E;
#define KL_bit(x, n) (((x) >> (n)) & 1)
#define g5(x, a, b, c, d, e) (KL_bit(x, a) + KL_bit(x, b) * 2 + KL_bit(x, c) * 4 + KL_bit(x, d) * 8 + KL_bit(x, e) * 16)

uint32_t keeloq_decrypt(const uint32_t data, const uint64_t key) {
    uint32_t x = data, r;
    for(r = 0; r < 528; r++)
        x = (x << 1) ^ KL_bit(x, 31) ^ KL_bit(x, 15) ^ (uint32_t)KL_bit(key, (15 - r) & 63) ^
            KL_bit(KEELOQ_NLF, g5(x, 0, 8, 19, 25, 30));
    return x;
}

uint64_t reverse_data(uint64_t data, uint8_t len) {
    uint64_t temp = 0;
    for(uint8_t i = 0; i < len; i++) {
        if((data >> i) & 1) {
            temp |= (uint64_t)1 << (len - 1 - i);
        }
    }
    return temp;
}

bool check_decrypted_data(uint32_t decrypted_data, uint32_t fixed_part_from_packet) {
    uint8_t btn_dec = decrypted_data >> 28;
    uint8_t btn_fix = fixed_part_from_packet >> 28;
    if (btn_dec != btn_fix) return false;

    uint16_t serial_part_dec = (decrypted_data >> 16) & 0xFF;
    uint16_t serial_part_fix = fixed_part_from_packet & 0xFF;
    if (serial_part_dec == serial_part_fix || serial_part_dec == 0) {
        barrierCodeMain = fixed_part_from_packet & 0x0FFFFFFF;
        barrierBit = btn_fix;
        keeloqCounter = decrypted_data & 0xFFFF;
        return true;
    }
    return false;
}

// --- КОНЕЧНЫЙ АВТОМАТ ПРИЕМНИКА ---
#define KEELOQ_TE_SHORT 400
#define KEELOQ_TE_LONG 800
#define KEELOQ_TE_DELTA 160
#define KEELOQ_SYNC_US 4000
#define KEELOQ_SYNC_DELTA (KEELOQ_TE_DELTA * 10)

void processKeeloqSignal(bool level, uint32_t duration) {
    switch (keeloqState) {
    case KEELOQ_RESET:
        // Ищем длинную синхро-паузу, которая идет после преамбулы
        if (!level && (duration > KEELOQ_SYNC_US - KEELOQ_SYNC_DELTA) && (duration < KEELOQ_SYNC_US + KEELOQ_SYNC_DELTA)) {
            keeloqRawData = 0;
            keeloqBitCount = 0;
            keeloqState = KEELOQ_SAVE_DURATION;
        }
        break;

    case KEELOQ_SAVE_DURATION:
        if (level) {
            keeloqLastDuration = duration;
            keeloqState = KEELOQ_CHECK_DURATION;
        } else {
            keeloqState = KEELOQ_RESET;
        }
        break;

    case KEELOQ_CHECK_DURATION:
        if (!level) {
            // Как только мы собрали 64 бита данных, мы перестаем пытаться декодировать биты
            // и просто игнорируем все до следующего сброса.
            if (keeloqBitCount >= 64) {
                // Пакет данных собран, теперь ждем финальную паузу для сброса.
                // Если пришла очень длинная пауза - это она и есть.
                if (duration > KEELOQ_SYNC_US) {
                     keeloqState = KEELOQ_RESET;
                }
                break; 
            }

            bool bit_val;
            bool success = false;
            uint32_t delta_long = KEELOQ_TE_DELTA * 2;
            if ((keeloqLastDuration > KEELOQ_TE_SHORT - KEELOQ_TE_DELTA) && (keeloqLastDuration < KEELOQ_TE_SHORT + KEELOQ_TE_DELTA) &&
                (duration > KEELOQ_TE_LONG - delta_long) && (duration < KEELOQ_TE_LONG + delta_long)) {
                bit_val = 1; success = true;
            } else if ((duration > KEELOQ_TE_SHORT - KEELOQ_TE_DELTA) && (duration < KEELOQ_TE_SHORT + KEELOQ_TE_DELTA) &&
                       (keeloqLastDuration > KEELOQ_TE_LONG - delta_long) && (keeloqLastDuration < KEELOQ_TE_LONG + delta_long)) {
                bit_val = 0; success = true;
            }

            if (success) {
                keeloqRawData = (keeloqRawData << 1) | bit_val;
                keeloqBitCount++;
                keeloqState = KEELOQ_SAVE_DURATION;
                
                if (keeloqBitCount == 64) { // Когда собрали ровно 64 бита данных
                    // Мы не сбрасываем состояние, а ждем статусные биты и финальную паузу
                    uint64_t reversed_data = reverse_data(keeloqRawData, 64);
                    uint32_t hop = reversed_data & 0xFFFFFFFF;
                    uint32_t fix = reversed_data >> 32;
                    uint32_t decrypted = keeloq_decrypt(hop, manufacturer_key);
                    
                    if (check_decrypted_data(decrypted, fix)) {
                        barrierProtocol = 4;
                        newSignalReady = true;
                    }
                }
            } else {
                keeloqState = KEELOQ_RESET;
            }
        } else {
            keeloqState = KEELOQ_RESET;
        }
        break;
    }
}