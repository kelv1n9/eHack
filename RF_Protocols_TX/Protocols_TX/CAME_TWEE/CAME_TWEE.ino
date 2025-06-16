/*
  Рабочий скетч для Arduino Uno для отправки команд по протоколу CAME Twee.
  Воспроизводит полную последовательность из 15 зашифрованных пакетов.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик

// Тайминги для CAME Twee (в микросекундах)
const int TWEE_HALF_BIT_PERIOD = 500;
const int TWEE_INTER_PACKET_PAUSE_US = 51000;

// "Радужная таблица" с магическими числами для XOR-шифрования.
// Скопирована из кода Flipper Zero.
const uint32_t came_twee_magic_numbers_xor[15] = {
    0x0E0E0E00, 0x1D1D1D11, 0x2C2C2C22, 0x3B3B3B33, 0x4A4A4A44,
    0x59595955, 0x68686866, 0x77777777, 0x86868688, 0x95959599,
    0xA4A4A4AA, 0xB3B3B3BB, 0xC2C2C2CC, 0xD1D1D1DD, 0xE0E0E0EE,
};


void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("CAME Twee Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШИ ДАННЫЕ ЗДЕСЬ ---
  // Код с 10-ти DIP-переключателей (0 = OFF, 1 = ON)
  // Пример: 0101010000 -> 0b0101010000
  uint16_t dip_code = 0b0101010000; 
  // Номер кнопки (обычно от 1 до 4)
  uint8_t button_code = 1; // 1-я кнопка
  // ------------------------------------
  
  // Формируем ключ для шифрования
  uint32_t key = (dip_code << 4) | button_code;

  Serial.print("Sending CAME Twee sequence for key: 0x");
  Serial.println(key, HEX);
  
  sendCameTweeSequence(key);

  delay(5000); // Пауза 5 секунд перед следующей отправкой
}


/**
 * @brief Отправляет один бит, используя Манчестерское кодирование.
 * '0' -> переход LOW-HIGH
 * '1' -> переход HIGH-LOW
 * @param bit_to_send Бит для отправки.
 */
void sendManchesterBit(bool bit_to_send) {
  if (bit_to_send) { // Отправка '1' (HIGH-LOW)
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TWEE_HALF_BIT_PERIOD);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TWEE_HALF_BIT_PERIOD);
  } else { // Отправка '0' (LOW-HIGH)
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TWEE_HALF_BIT_PERIOD);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TWEE_HALF_BIT_PERIOD);
  }
}

/**
 * @brief Генерирует и отправляет полную последовательность из 15 пакетов CAME Twee.
 * @param key 32-битный ключ, содержащий код DIP-переключателей и номер кнопки.
 */
void sendCameTweeSequence(uint32_t key) {
  // Цикл по всем 15 пакетам, от 14 до 0 (как в коде Flipper)
  for (int i = 14; i >= 0; i--) {
    // Шаг 1: "Шифруем" ключ, используя таблицу
    uint32_t encrypted_part = key ^ came_twee_magic_numbers_xor[i];
    
    // Шаг 2: Создаем полный 54-битный пакет, добавляя фиксированную часть
    uint64_t parcel_to_send = 0x003FFF7200000000ULL | encrypted_part;

    // Шаг 3: Отправляем 54 бита пакета манчестерским кодом
    // Flipper инвертирует биты перед отправкой, мы делаем то же самое
    for (int bit_idx = 53; bit_idx >= 0; bit_idx--) {
      bool current_bit = (parcel_to_send >> bit_idx) & 1ULL;
      sendManchesterBit(!current_bit); // Инвертируем бит
    }

    // Шаг 4: Отправляем длинную паузу между пакетами
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TWEE_INTER_PACKET_PAUSE_US);
  }
}