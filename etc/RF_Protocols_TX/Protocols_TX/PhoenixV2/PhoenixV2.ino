/*
  Простой передатчик для протокола Phoenix V2 на Arduino.
*/

// --- НАСТРОЙКИ ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки

// --- Константы протокола Phoenix V2 (в микросекундах) ---
const int P_TE_SHORT = 427;
const int P_TE_LONG = 853;
const int P_START_PULSE = 2562; // 427 * 6
const int P_PREAMBLE_PAUSE = 25620; // 427 * 60

/**
 * @brief Отправляет один полный 52-битный пакет Phoenix V2.
 * @param key Полный 52-битный ключ для отправки.
 */
void sendPhoenixV2Packet(uint64_t key) {
  // 1. Отправляем преамбулу (длинная пауза)
  digitalWrite(TX_PIN, LOW);
  // Надежно генерируем длинную паузу
  for (int j = 0; j < 25; j++) {
      delayMicroseconds(1000); 
  }
  delayMicroseconds(620);

  // 2. Отправляем стартовый бит (длинный импульс)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(P_START_PULSE);

  // 3. Отправляем 52 бита данных (со старшего к младшему)
  for (int i = 51; i >= 0; i--) {
    bool bit = (key >> i) & 1;
    
    // ВАЖНО: Инвертируем бит перед отправкой!
    bit = !bit; 
    
    if (bit) { // Отправляем логический '1' (длинная пауза + короткий импульс)
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(P_TE_LONG);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(P_TE_SHORT);
    } else { // Отправляем логический '0' (короткая пауза + длинный импульс)
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(P_TE_SHORT);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(P_TE_LONG);
    }
  }

  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Phoenix V2 Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ ПОЛНЫЙ 52-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // Ключ можно взять из Flipper Zero после анализа или сохранения сигнала.
  uint64_t phoenix_key = 0xABCDEF1234567ULL; // Пример 52-битного ключа

  Serial.print("Sending Phoenix V2 key: 0x");
  Serial.print((uint32_t)(phoenix_key >> 32), HEX);
  Serial.println((uint32_t)phoenix_key, HEX);
  
  // Отправляем пакет несколько раз для надежности
  for (int i = 0; i < REPEATS; i++) {
    sendPhoenixV2Packet(phoenix_key);
    // Пауза между повторами для разделения пакетов
    delay(5); 
  }

  delay(2000); // Пауза 2 секунды между сериями отправок
}