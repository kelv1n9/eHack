/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Gate-TX.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Gate-TX (в микросекундах)
const int GATETX_TE_SHORT = 350;
const int GATETX_TE_LONG = 700;
const int GATETX_PREAMBLE_US = GATETX_TE_SHORT * 49; // 17150 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Gate-TX Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 24-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  uint32_t key_24bit = 0x1A2B3C;
  
  Serial.print("Sending 24-bit Gate-TX key: 0x");
  Serial.println(key_24bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendGateTxPacket(key_24bit, 24);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу Gate-TX.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendGateTxBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинная пауза + короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(GATETX_TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(GATETX_TE_SHORT);
  } else { // Бит '0': короткая пауза + длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(GATETX_TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(GATETX_TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Gate-TX.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (обычно 24).
 */
void sendGateTxPacket(uint32_t data, int num_bits) {
  // 1. Преамбула (очень длинная пауза)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(GATETX_PREAMBLE_US);

  // 2. Стартовый бит (Синхронизация)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(GATETX_TE_LONG);

  // 3. Данные
  // Отправляем биты, начиная со старшего (MSB)
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendGateTxBit(current_bit);
  }
  
  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}