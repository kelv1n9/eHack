/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Holtek.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Holtek (в микросекундах)
const int HOLTEK_TE_SHORT = 430;
const int HOLTEK_TE_LONG = 870;
const int HOLTEK_PREAMBLE_US = HOLTEK_TE_SHORT * 36; // 15480 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Holtek Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 36-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // Ключ Holtek состоит из 2-х частей:
  // 1. Старшие 4 бита всегда 0x5 (0b0101)
  // 2. Младшие 36 бит - это ваш уникальный код.
  uint64_t user_key_36bit = 0x123456789ULL; // Пример вашего 36-битного кода
  
  // Собираем полный 40-битный ключ
  uint64_t key_40bit = (0x5ULL << 36) | user_key_36bit;
  
  Serial.print("Sending 40-bit Holtek key: 0x");
  Serial.print((uint32_t)(key_40bit >> 32), HEX);
  Serial.println((uint32_t)key_40bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendHoltekPacket(key_40bit, 40);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу Holtek.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendHoltekBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинная пауза + короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HOLTEK_TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HOLTEK_TE_SHORT);
  } else { // Бит '0': короткая пауза + длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HOLTEK_TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HOLTEK_TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Holtek.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (всегда 40).
 */
void sendHoltekPacket(uint64_t data, int num_bits) {
  // 1. Преамбула (длинная пауза)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(HOLTEK_PREAMBLE_US);

  // 2. Стартовый бит (короткий импульс)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(HOLTEK_TE_SHORT);

  // 3. Данные
  // Отправляем биты, начиная со старшего (MSB)
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1ULL;
    sendHoltekBit(current_bit);
  }
  
  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}