/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу на базе чипов Holtek HT12E.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Holtek HT12x (в микросекундах)
const int HT12X_TE = 320;
const int HT12X_PREAMBLE_US = HT12X_TE * 36; // 11520 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Holtek HT12x Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 12-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // Обычно это 8 бит адреса (DIP-переключатели) и 4 бита данных (кнопки)
  uint16_t key_12bit = 0b101010100101; 
  
  Serial.print("Sending 12-bit Holtek HT12x key: 0x");
  Serial.println(key_12bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendHoltekHT12xPacket(key_12bit, 12);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу Holtek HT12x.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendHoltekHT12xBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинная пауза + короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HT12X_TE * 2);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HT12X_TE);
  } else { // Бит '0': короткая пауза + длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HT12X_TE);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HT12X_TE * 2);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Holtek HT12x.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (всегда 12).
 */
void sendHoltekHT12xPacket(uint16_t data, int num_bits) {
  // 1. Преамбула (длинная пауза)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(HT12X_PREAMBLE_US);

  // 2. Стартовый бит (короткий импульс)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(HT12X_TE);

  // 3. Данные
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendHoltekHT12xBit(current_bit);
  }
  
  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}