/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Magellan.
  Включает автоматический расчет CRC8.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 5;     // Количество повторов отправки пакета

// Тайминги для Magellan (в микросекундах)
const int MAGELLAN_TE_SHORT = 200;
const int MAGELLAN_TE_LONG = 400;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Magellan Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 24-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // Состоит из 8 бит кода события и 16 бит серийного номера
  uint32_t key_24bit = 0x1275EC; // Пример из кода Flipper
  
  // 1. Собираем правильный 32-битный пакет с контрольной суммой
  uint32_t packet_to_send = buildMagellanPacket(key_24bit);

  Serial.print("Sending 24-bit key: 0x");
  Serial.print(key_24bit, HEX);
  Serial.print(" as 32-bit packet: 0x");
  Serial.println(packet_to_send, HEX);
  
  // 2. Отправляем всю серию
  for (int i = 0; i < REPEATS; i++) {
    sendMagellanPacket(packet_to_send, 32);
  }

  delay(2000); 
}

/**
 * @brief Рассчитывает контрольную сумму CRC8 по алгоритму из Flipper.
 */
uint8_t crc8(uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief Собирает валидный 32-битный пакет Magellan с CRC8.
 * @param data_24bit 24-битные данные (событие + серийник).
 * @return Готовый к отправке 32-битный пакет.
 */
uint32_t buildMagellanPacket(uint32_t data_24bit) {
    uint8_t data_for_crc[3];
    // Берем 3 байта из 24-битного ключа
    data_for_crc[0] = (data_24bit >> 16) & 0xFF;
    data_for_crc[1] = (data_24bit >> 8) & 0xFF;
    data_for_crc[2] = data_24bit & 0xFF;
    
    // Считаем CRC
    uint8_t calculated_crc = crc8(data_for_crc, 3);
    
    // Собираем финальный 32-битный пакет
    return (data_24bit << 8) | calculated_crc;
}


/**
 * @brief Отправляет один бит данных по протоколу Magellan.
 */
void sendMagellanBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': короткий HIGH + длинный LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(MAGELLAN_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(MAGELLAN_TE_LONG);
  } else { // Бит '0': длинный HIGH + короткий LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(MAGELLAN_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(MAGELLAN_TE_SHORT);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Magellan.
 */
void sendMagellanPacket(uint32_t data, int num_bits) {
  // 1. Сложная преамбула
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(MAGELLAN_TE_SHORT * 4);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(MAGELLAN_TE_SHORT);
  for(uint8_t i = 0; i < 12; i++) {
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(MAGELLAN_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(MAGELLAN_TE_SHORT);
  }
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(MAGELLAN_TE_SHORT);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(MAGELLAN_TE_LONG);

  // 2. Стартовый бит
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(MAGELLAN_TE_LONG * 3);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(MAGELLAN_TE_LONG);

  // 3. Данные (32 бита)
  for (int i = num_bits - 1; i >= 0; i--) {
    sendMagellanBit((data >> i) & 1);
  }
  
  // 4. Стоп-бит
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(MAGELLAN_TE_SHORT);
  // Финальная длинная пауза
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(MAGELLAN_TE_LONG * 100);
}