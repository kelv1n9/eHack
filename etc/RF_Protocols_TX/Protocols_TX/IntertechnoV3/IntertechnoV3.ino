/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Intertechno V3.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 6;     // Протокол требует 6 повторов

// Тайминги для Intertechno V3 (в микросекундах)
const int ITV3_TE_SHORT = 275;
const int ITV3_TE_LONG = 1375; // 5 * 275

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Intertechno V3 Transmitter Initialized.");
}

void loop() {
  // --- Пример для обычной 32-битной команды ---
  uint32_t key_32bit = 0x3F86C59F; // Пример ключа
  
  Serial.print("Sending 32-bit Intertechno V3 key: 0x");
  Serial.println(key_32bit, HEX);
  
  for (int i = 0; i < REPEATS; i++) {
    sendIntertechnoV3Packet(key_32bit, 32);
  }
  delay(3000);

  // --- Пример для 36-битной команды с диммированием ---
  // Первые 27 бит - адрес, последние 4 - уровень диммирования (0-15)
  uint64_t key_36bit = 0x42D2E8856ULL; // Пример ключа
  
  Serial.print("Sending 36-bit Intertechno V3 Dimming key: 0x");
  Serial.print((uint32_t)(key_36bit >> 32), HEX);
  Serial.println((uint32_t)key_36bit, HEX);

  for (int i = 0; i < REPEATS; i++) {
    sendIntertechnoV3Packet(key_36bit, 36);
  }
  delay(3000);
}


/**
 * @brief Формирует и отправляет полный пакет данных Intertechno V3.
 * @param data Данные (ключ) для отправки.
 * @param num_bits Количество бит в ключе (32 или 36).
 */
void sendIntertechnoV3Packet(uint64_t data, int num_bits) {
  // 1. Преамбула
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(ITV3_TE_SHORT);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(ITV3_TE_SHORT * 38);

  // 2. Синхронизация
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(ITV3_TE_SHORT);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(ITV3_TE_SHORT * 10);

  // 3. Данные
  for (int i = num_bits - 1; i >= 0; i--) {
    // Особый случай для бита диммирования в 36-битной версии
    if (num_bits == 36 && i == 8) {
        // Отправка символа "Dimm"
        digitalWrite(TX_PIN, HIGH);
        delayMicroseconds(ITV3_TE_SHORT);
        digitalWrite(TX_PIN, LOW);
        delayMicroseconds(ITV3_TE_SHORT);
        digitalWrite(TX_PIN, HIGH);
        delayMicroseconds(ITV3_TE_SHORT);
        digitalWrite(TX_PIN, LOW);
        delayMicroseconds(ITV3_TE_SHORT);
        continue; // Переходим к следующему биту
    }

    bool current_bit = (data >> i) & 1ULL;
    if (current_bit) { // Бит '1'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(ITV3_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(ITV3_TE_LONG);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(ITV3_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(ITV3_TE_SHORT);
    } else { // Бит '0'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(ITV3_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(ITV3_TE_SHORT);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(ITV3_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(ITV3_TE_LONG);
    }
  }
  
  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}