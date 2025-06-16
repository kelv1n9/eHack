/*
  Скетч для Arduino Uno для отправки команд по протоколу Nice Flo.
  Адаптировано на основе анализа прошивки Flipper Zero.
*/

// --- Настройки ---
const int TX_PIN = 5; // Пин, к которому подключен передатчик
const int REPEATS = 10; // Количество повторов отправки пакета (как в Flipper)

// Длительности импульсов из кода Flipper (в микросекундах)
const int TE_SHORT = 700;
const int TE_LONG = 1400;

// Ваш ключ (данные) для отправки.
// Nice Flo обычно использует 12 или 24 бит.
// Пример 24-битного ключа:
const long KEY_DATA = 0x123456;
const int KEY_BIT_COUNT = 24;


void setup() {
  // Настраиваем пин передатчика на выход
  pinMode(TX_PIN, OUTPUT);
  // В начальном состоянии на пине низкий уровень
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Nice Flo Transmitter Initialized.");
  Serial.println("Press any key in Serial Monitor to send the signal.");
}

void loop() {
  // Выводим в порт информацию о том, что сейчас будет отправка
  Serial.print("Sending Key: 0x");
  Serial.print(KEY_DATA, HEX);
  Serial.print(" (");
  Serial.print(REPEATS);
  Serial.println(" times)...");
  
  // Отправляем пакет несколько раз для надежности
  for (int i = 0; i < REPEATS; i++) {
    sendNiceFloPacket(KEY_DATA, KEY_BIT_COUNT);
  }
  
  Serial.println("Done. Waiting for 2 seconds.");

  // Ждем 2 секунды (2000 миллисекунд) перед следующей отправкой
  delay(2000);
}


/**
 * @brief Отправляет один бит данных по протоколу Nice Flo.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendBit(bool bit_to_send) {
  if (bit_to_send) {
    // Отправка бита '1': длинная пауза + короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_SHORT);
  } else {
    // Отправка бита '0': короткая пауза + длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_LONG);
  }
}

/**
 * @brief Формирует и отправляет полный пакет данных Nice Flo.
 * @param data Данные (ключ), которые нужно отправить.
 * @param num_bits Количество бит в ключе (обычно 12 или 24).
 */
void sendNiceFloPacket(long data, int num_bits) {
  // 1. Заголовок (Header)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(TE_SHORT * 36);

  // 2. Стартовый бит (Start Bit)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(TE_SHORT);

  // 3. Данные (Data)
  // Отправляем биты, начиная со старшего (MSB)
  for (int i = num_bits - 1; i >= 0; i--) {
    // Используем побитовую операцию "И" для извлечения i-го бита
    bool current_bit = (data >> i) & 1;
    sendBit(current_bit);
  }

  // Завершаем передачу, устанавливая низкий уровень на пине
  digitalWrite(TX_PIN, LOW);
}