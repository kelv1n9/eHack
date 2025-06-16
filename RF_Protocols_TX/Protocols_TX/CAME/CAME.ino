/*
  Скетч для Arduino Uno для отправки команд по протоколу CAME.
  Адаптировано на основе анализа прошивки Flipper Zero.
*/

// --- Настройки ---
const int TX_PIN = 5;    // Пин, к которому подключен передатчик (433.92 MHz)
const int REPEATS = 10;  // Количество повторов отправки пакета

// Длительности импульсов CAME (в микросекундах)
const int CAME_TE_SHORT = 320;
const int CAME_TE_LONG = 640;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("CAME Transmitter Initialized.");
}

void loop() {
  // --- Пример отправки 12-битного кода ---
  long key_12bit = 0b101010101010; // Ваш 12-битный ключ
  Serial.print("Sending 12-bit CAME key: 0x");
  Serial.println(key_12bit, HEX);
  for (int i = 0; i < REPEATS; i++) {
    sendCamePacket(key_12bit, 12);
  }
  delay(2000); // Пауза 2 секунды

  // --- Пример отправки 24-битного кода ---
  long key_24bit = 0xABCDEF; // Ваш 24-битный ключ
  Serial.print("Sending 24-bit CAME key: 0x");
  Serial.println(key_24bit, HEX);
  for (int i = 0; i < REPEATS; i++) {
    sendCamePacket(key_24bit, 24);
  }
  delay(2000); // Пауза 2 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу CAME.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendBit(bool bit_to_send) {
  if (bit_to_send) {
    // Отправка бита '1': длинная пауза + короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(CAME_TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(CAME_TE_SHORT);
  } else {
    // Отправка бита '0': короткая пауза + длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(CAME_TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(CAME_TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных CAME.
 * @param data Данные (ключ), которые нужно отправить.
 * @param num_bits Количество бит в ключе (обычно 12 или 24).
 */
void sendCamePacket(long data, int num_bits) {
  // 1. Заголовок (Header), длительность зависит от числа бит
  int header_duration_us;
  switch (num_bits) {
    case 12:
      header_duration_us = 47 * CAME_TE_SHORT; // 15040 us
      break;
    case 24:
      header_duration_us = 76 * CAME_TE_SHORT; // 24320 us
      break;
    default:
      header_duration_us = 47 * CAME_TE_SHORT; // По умолчанию для 12 бит
      break;
  }
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(header_duration_us);

  // 2. Стартовый бит (Start Bit)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(CAME_TE_SHORT);

  // 3. Данные (Data)
  // Отправляем биты, начиная со старшего (MSB)
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendBit(current_bit);
  }

  // Завершаем передачу, устанавливая низкий уровень на пине
  digitalWrite(TX_PIN, LOW);
}