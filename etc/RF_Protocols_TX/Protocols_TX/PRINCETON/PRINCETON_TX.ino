/*
  Скетч для Arduino Uno для отправки команд по протоколу Princeton (PT2262).
  Адаптировано на основе анализа прошивки Flipper Zero.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 8;     // Количество повторов отправки пакета

// Базовый временной интервал (в микросекундах)
const int PRINCETON_TE = 390;
// Множитель для паузы между пакетами (Guard Time)
const int PRINCETON_GUARD_MULTIPLIER = 30;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Princeton (PT2262) Transmitter Initialized.");
}

void loop() {
  // --- Пример отправки 24-битного кода ---
  long key_24bit = 0b101010101111000011001100; // Ваш 24-битный ключ
  
  Serial.print("Sending 24-bit Princeton key: 0x");
  Serial.println(key_24bit, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendPrincetonPacket(key_24bit, 24);
  }

  delay(3000); // Пауза 3 секунды перед следующей серией отправок
}

/**
 * @brief Формирует и отправляет полный пакет данных Princeton.
 * @param data Данные (ключ), которые нужно отправить.
 * @param num_bits Количество бит в ключе (обычно 24).
 */
void sendPrincetonPacket(long data, int num_bits) {
  // 1. Данные (Data)
  // Отправляем биты, начиная со старшего (MSB)
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    if (current_bit) {
      // Отправка бита '1': длинный HIGH + короткий LOW
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(PRINCETON_TE * 3);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(PRINCETON_TE);
    } else {
      // Отправка бита '0': короткий HIGH + длинный LOW
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(PRINCETON_TE);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(PRINCETON_TE * 3);
    }
  }

  // 2. Синхронизация (Sync bit + Guard Time)
  // Короткий высокий импульс...
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(PRINCETON_TE);
  // ...и длинная пауза для разделения пакетов
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(PRINCETON_TE * PRINCETON_GUARD_MULTIPLIER);
}