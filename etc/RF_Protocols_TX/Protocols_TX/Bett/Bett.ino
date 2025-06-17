/*
  Простой передатчик для протоколов BETT (Berner/Elka/TedSen/Teletaster) на Arduino.
  Использует PWM-кодирование.
*/

// --- НАСТРОЙКИ ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки

// --- Константы протокола ---
const int TE_SHORT_US = 340;
const int TE_LONG_US = 2000;
// Длинная пауза в конце пакета, служит также синхронизацией для следующего
const int TRAILER_PAUSE_US = TE_LONG_US * 7; 

/**
 * @brief Собирает 18-битный ключ из массива символов DIP-переключателей.
 * @param dips Массив из 9 символов ('+', '0' или '-').
 * @return 32-битное число, где младшие 18 бит представляют ключ.
 */
uint32_t buildBettKey(const char dips[9]) {
  uint32_t key = 0;
  for (int i = 0; i < 9; i++) {
    key <<= 2; // Сдвигаем на 2 бита для нового переключателя
    switch (dips[i]) {
      case '+':
        key |= 0b11; // Код для '+'
        break;
      case '0':
        key |= 0b10; // Код для '0'
        break;
      case '-':
        key |= 0b00; // Код для '-'
        break;
    }
  }
  return key;
}

/**
 * @brief Отправляет один полный 18-битный пакет BETT.
 * @param key 18-битный ключ для отправки.
 */
void sendBettPacket(uint32_t key) {
  // Отправляем биты с 17-го по 1-й (всего 17 бит)
  for (int i = 17; i >= 1; i--) {
    if ((key >> i) & 1) { // Отправляем '1'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(TE_LONG_US);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(TE_SHORT_US);
    } else { // Отправляем '0'
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(TE_SHORT_US);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(TE_LONG_US);
    }
  }
  
  // Отправляем последний, 0-й бит с длинной паузой в конце
  if (key & 1) { // Отправляем '1'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_LONG_US);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_SHORT_US + TRAILER_PAUSE_US);
  } else { // Отправляем '0'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_SHORT_US);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_LONG_US + TRAILER_PAUSE_US);
  }
}

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("BETT Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ПОЛОЖЕНИЕ 9-ти DIP-ПЕРЕКЛЮЧАТЕЛЕЙ ЗДЕСЬ ---
  // Используйте символы '+', '0', '-'
  const char dip_switches[9] = {'+', '0', '-', '+', '0', '-', '+', '0', '-'};

  // 1. Собираем ключ из DIP-переключателей
  uint32_t key = buildBettKey(dip_switches);

  Serial.print("Sending BETT key. DIP: ");
  for(int i=0; i<9; i++) Serial.print(dip_switches[i]);
  Serial.print(", Key: 0x");
  Serial.println(key, HEX);
  
  // 2. Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendBettPacket(key);
  }

  delay(2000); // Пауза между сериями отправок
}