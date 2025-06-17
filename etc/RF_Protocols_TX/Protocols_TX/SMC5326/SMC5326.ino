/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу SMC5326.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для SMC5326 (в микросекундах)
const int SMC_TE_SHORT = 300;
const int SMC_TE_LONG = 900;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("SMC5326 Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШИ ДАННЫЕ ЗДЕСЬ ---
  // Задайте 8-позиционный DIP-код в виде строки из символов '+', '0', '-'
  const char* dip_string = "+0-+0-0+"; 
  // Номер кнопки (от 1 до 4)
  uint8_t button = 1; 
  // ------------------------------------
  
  // 1. Собираем правильный 25-битный ключ
  uint32_t key_25bit = buildSmc5326Key(dip_string, button);

  Serial.print("Sending SMC5326. DIP: "); Serial.print(dip_string);
  Serial.print(", BTN: "); Serial.print(button);
  Serial.print(" -> Key: 0x"); Serial.println(key_25bit, HEX);
  
  // 2. Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendSmc5326Packet(key_25bit, 25);
  }

  delay(3000); 
}

/**
 * @brief Собирает 25-битный ключ из 8-символьной строки DIP и кнопки.
 */
uint32_t buildSmc5326Key(const char* dips, uint8_t btn) {
    uint32_t dip_key = 0;
    // Кодируем 8 DIP-переключателей в 16-битное число
    for (int i = 0; i < 8; i++) {
        dip_key <<= 2;
        if (dips[i] == '+') {
            dip_key |= 0b11;
        } else if (dips[i] == '0') {
            dip_key |= 0b10;
        } else if (dips[i] == '-') {
            dip_key |= 0b00;
        }
    }

    uint32_t button_key = 0b000000010; // 9 бит кнопок, по умолчанию все выключены
    if(btn >= 1 && btn <= 4) {
      // Кнопки кодируются установкой двух бит в 11
      // B1: биты 7,6; B2: 6,5; B3: 4,3; B4: 2,1
      uint8_t shift = (4 - btn) * 2;
      button_key |= (0b11 << shift);
    }
    
    // Собираем финальный 25-битный ключ
    return (dip_key << 9) | button_key;
}


/**
 * @brief Отправляет один бит данных по протоколу SMC5326.
 */
void sendSmc5326Bit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинный HIGH + короткий LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(SMC_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(SMC_TE_SHORT);
  } else { // Бит '0': короткий HIGH + длинный LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(SMC_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(SMC_TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных SMC5326.
 * @param data 25-битный ключ.
 * @param num_bits Количество бит в ключе (всегда 25).
 */
void sendSmc5326Packet(uint32_t data, int num_bits) {
  // Протокол не имеет преамбулы.
  
  // 1. Данные
  for (int i = num_bits - 1; i >= 0; i--) {
    sendSmc5326Bit((data >> i) & 1);
  }
  
  // 2. Стоп-бит
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(SMC_TE_SHORT);
  
  // 3. Финальная пауза (Guard Time)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(SMC_TE_SHORT * 25);
}