/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Nero Radio.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Nero Radio (в микросекундах)
const int NERO_RADIO_TE_SHORT = 200;
const int NERO_RADIO_TE_LONG = 400;
const int NERO_RADIO_PREAMBLE_COUNT = 49; // Количество пар в преамбуле

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Nero Radio Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 56-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  uint64_t key_56bit = 0x123456789ABCDEULL; // "ULL" в конце обязательно
  
  Serial.print("Sending 56-bit Nero Radio key: 0x");
  Serial.print((uint32_t)(key_56bit >> 32), HEX);
  Serial.println((uint32_t)key_56bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendNeroRadioPacket(key_56bit, 56);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Формирует и отправляет полный пакет данных Nero Radio.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (обычно 56).
 */
void sendNeroRadioPacket(uint64_t data, int num_bits) {
  // 1. Преамбула
  for (int i = 0; i < NERO_RADIO_PREAMBLE_COUNT; i++) {
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(NERO_RADIO_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(NERO_RADIO_TE_SHORT);
  }

  // 2. Стартовый бит (Синхронизация)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(NERO_RADIO_TE_SHORT * 4);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(NERO_RADIO_TE_SHORT);

  // 3. Данные
  // Отправляем все биты, кроме последнего
  for (int i = num_bits - 1; i > 0; i--) {
    bool current_bit = (data >> i) & 1ULL;
    if (current_bit) { // Бит '1'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(NERO_RADIO_TE_LONG);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(NERO_RADIO_TE_SHORT);
    } else { // Бит '0'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(NERO_RADIO_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(NERO_RADIO_TE_LONG);
    }
  }

  // 4. Отправка ПОСЛЕДНЕГО бита с длинной финальной паузой
  bool last_bit = data & 1ULL;
  if (last_bit) { // Последний бит '1'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(NERO_RADIO_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(NERO_RADIO_TE_SHORT * 37);
  } else { // Последний бит '0'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(NERO_RADIO_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(NERO_RADIO_TE_SHORT * 37);
  }
}