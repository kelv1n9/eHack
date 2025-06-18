/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Dooya.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 5;     // Dooya обычно требует меньше повторов

// Тайминги для Dooya (в микросекундах)
const int DOOYA_TE_SHORT = 366;
const int DOOYA_TE_LONG = 733;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Dooya Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 40-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  uint64_t key_40bit = 0xE1DC030533ULL; // Пример ключа из документации Flipper
  
  Serial.print("Sending 40-bit Dooya key: 0x");
  Serial.print((uint32_t)(key_40bit >> 32), HEX);
  Serial.println((uint32_t)key_40bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendDooyaPacket(key_40bit, 40);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу Dooya.
 */
void sendDooyaBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинный HIGH + короткий LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(DOOYA_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(DOOYA_TE_SHORT);
  } else { // Бит '0': короткий HIGH + длинный LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(DOOYA_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(DOOYA_TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Dooya.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (всегда 40).
 */
void sendDooyaPacket(uint64_t data, int num_bits) {
  // 1. Преамбула, Часть 1: длинная пауза, зависящая от последнего бита
  bool last_bit = data & 1ULL;
  uint32_t preamble_part1_us = DOOYA_TE_LONG * 12;
  if (last_bit) {
    preamble_part1_us += DOOYA_TE_LONG;
  } else {
    preamble_part1_us += DOOYA_TE_SHORT;
  }
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(preamble_part1_us);

  // 2. Преамбула, Часть 2: очень длинный импульс + длинная пауза
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(DOOYA_TE_SHORT * 13);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(DOOYA_TE_LONG * 2);

  // 3. Данные
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1ULL;
    sendDooyaBit(current_bit);
  }
  
  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}