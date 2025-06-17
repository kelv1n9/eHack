/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Linear MegaCode.
  Реализация основана на анализе протокола, а не на запутанном коде Flipper.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 8;     // Количество повторов отправки пакета

// Тайминги для Linear MegaCode (в микросекундах)
const int MEGACODE_TE = 1000;
const int MEGACODE_GUARD_US = 13000; // Пауза между пакетами

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Linear MegaCode Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ КЛЮЧ ЗДЕСЬ ---
  // Протокол передает 24 бита. Первый бит ВСЕГДА '1' (стартовый).
  // Остальные 23 бита - это ваш код (Facility Code + Key + Button).
  // Пример: 1 (старт) + 0011 (FC) + 0100001110100100 (Key) + 001 (Btn)
  uint32_t key_24bit = 0b100110100001110100100001; 
  
  Serial.print("Sending 24-bit MegaCode key: 0b");
  for(int i = 23; i >= 0; i--) {
      Serial.print( (key_24bit >> i) & 1 );
  }
  Serial.println();
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendMegaCodePacket(key_24bit, 24);
  }

  delay(2000); // Пауза 2 секунды
}

/**
 * @brief Отправляет один бит данных по протоколу MegaCode (PPM).
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendMegaCodeBit(bool bit_to_send) {
  // Каждый бит - это пара "Пауза + Импульс"
  if (bit_to_send) { // Бит '1'
    digitalWrite(TX_PIN, LOW); // Длинная пауза
    delayMicroseconds(MEGACODE_TE * 5);
  } else { // Бит '0'
    digitalWrite(TX_PIN, LOW); // Короткая пауза
    delayMicroseconds(MEGACODE_TE * 2);
  }
  // Все импульсы одинаковые
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(MEGACODE_TE);
}


/**
 * @brief Формирует и отправляет полный пакет данных MegaCode.
 * @param data 24-битный ключ (включая стартовый бит).
 * @param num_bits Количество бит (всегда 24).
 */
void sendMegaCodePacket(uint32_t data, int num_bits) {
  // Протокол не имеет преамбулы. Начинаем сразу с данных.
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendMegaCodeBit(current_bit);
  }
  
  // Финальная пауза для синхронизации приемника
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(MEGACODE_GUARD_US);
}