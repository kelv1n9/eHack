/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Hormann HSM (синие кнопки).
*/

// --- Настройки ---
const int TX_PIN = 5;       // Пин, к которому подключен передатчик
const int MAIN_REPEATS = 4; // Сколько раз отправить всю длинную посылку
const int SUB_REPEATS = 20; // Сколько суб-пакетов в одной посылке

// Тайминги для Hormann (в микросекундах)
const int HORMANN_TE_SHORT = 500;
const int HORMANN_TE_LONG = 1000;
const int HORMANN_HEADER_US = HORMANN_TE_SHORT * 24; // 12000 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Hormann HSM Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШИ ДАННЫЕ ЗДЕСЬ ---
  // Ключ Hormann состоит из 3-х частей:
  // 1. Старшие 8 бит всегда 0xFF
  // 2. Средние 34 бита - это ваш уникальный код (например, с DIP-переключателей)
  // 3. Младшие 2 бита всегда 0b11 (или 3 в десятичной)
  uint64_t user_code_34bit = 0x123456789ULL; // Пример вашего 34-битного кода
  
  // Собираем полный 44-битный ключ
  uint64_t key_44bit = (0xFFULL << 36) | (user_code_34bit << 2) | 0x3ULL;
  
  Serial.print("Sending Hormann HSM key: 0x");
  Serial.print((uint32_t)(key_44bit >> 32), HEX);
  Serial.println((uint32_t)key_44bit, HEX);
  
  // Отправляем всю серию посылок несколько раз
  for (int i = 0; i < MAIN_REPEATS; i++) {
    sendHormannSequence(key_44bit);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу Hormann.
 */
void sendHormannBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HORMANN_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HORMANN_TE_SHORT);
  } else { // Бит '0'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HORMANN_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HORMANN_TE_LONG);
  }
}

/**
 * @brief Отправляет один суб-пакет Hormann (заголовок + 44 бита данных).
 */
void sendHormannSubPacket(uint64_t data) {
  // 1. Заголовок суб-пакета
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(HORMANN_HEADER_US);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(HORMANN_TE_SHORT);

  // 2. Данные (44 бита)
  for (int i = 43; i >= 0; i--) {
    bool current_bit = (data >> i) & 1ULL;
    sendHormannBit(current_bit);
  }
}

/**
 * @brief Отправляет полную "супер-посылку" Hormann из 20 суб-пакетов.
 */
void sendHormannSequence(uint64_t data) {
  // Отправляем 20 суб-пакетов подряд
  for (int i = 0; i < SUB_REPEATS; i++) {
    sendHormannSubPacket(data);
  }
  // Завершающий длинный импульс
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(HORMANN_HEADER_US);
  digitalWrite(TX_PIN, LOW);
}