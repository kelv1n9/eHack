/*
  Простой передатчик для протокола Marantec на Arduino.
*/

// --- НАСТРОЙКИ ---
const int TX_PIN = 5;      // Пин, к которому подключен 433МГц передатчик
const int REPEATS = 10;    // Количество повторов отправки одного пакета

// --- Константы протокола ---
const int TE_HALF_BIT = 1000; // Длительность половины бита (1000 мкс = 1 мс)
const int PAUSE_US = 10000;   // Длительность паузы-заголовка (10 мс)

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Marantec Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 49-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // ВАЖНО: 49-й бит (самый старший) должен быть '1'.
  uint64_t marantec_key = 0x1223344556677ULL; // Пример ключа. 0x1... означает, что старший бит - единица.

  Serial.print("Sending Marantec key: 0x");
  Serial.print((uint32_t)(marantec_key >> 32), HEX);
  Serial.println((uint32_t)marantec_key, HEX);
  
  // Отправляем пакет несколько раз для надежности
  for (int i = 0; i < REPEATS; i++) {
    sendMarantecPacket(marantec_key);
  }

  delay(2000); // Пауза 2 секунды между отправками
}

/**
 * @brief Отправляет один бит с использованием Манчестерского кодирования.
 * @param bit Бит для отправки (true для 1, false для 0).
 */
void sendManchesterBit(bool bit) {
  // 0 кодируется как переход Low -> High
  // 1 кодируется как переход High -> Low
  if (bit) { // Отправляем '1'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_HALF_BIT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_HALF_BIT);
  } else { // Отправляем '0'
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_HALF_BIT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_HALF_BIT);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Marantec.
 * @param key Полный 49-битный ключ для отправки.
 */
void sendMarantecPacket(uint64_t key) {
  // 1. Отправляем самый первый бит (он всегда '1')
  sendManchesterBit(true);

  // 2. Отправляем длинную паузу-заголовок
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(PAUSE_US);

  // 3. Отправляем оставшиеся 48 бит (с 47-го по 0-й)
  for (int i = 47; i >= 0; i--) {
    bool current_bit = (key >> i) & 1;
    sendManchesterBit(current_bit);
  }

  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}