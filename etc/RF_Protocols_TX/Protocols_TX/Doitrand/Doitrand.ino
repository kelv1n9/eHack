/*
  ФИНАЛЬНАЯ ВЕРСИЯ передатчика для протокола Doitrand на Arduino.
  С надежной генерацией длинной паузы.
*/

// --- НАСТРОЙКИ ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки

// --- Константы протокола Doitrand (в микросекундах) ---
const int D_TE_SHORT = 400;
const int D_TE_LONG = 1100;
// const int D_PREAMBLE_PAUSE = 24800; // Больше не нужна
const int D_START_PULSE = 700;

/**
 * @brief Отправляет один полный 37-битный пакет Doitrand.
 * @param key Полный 37-битный ключ для отправки.
 */
void sendDoitrandPacket(uint64_t key) {
  // 1. Отправляем преамбулу и стартовый бит
  digitalWrite(TX_PIN, LOW);
  // ИСПРАВЛЕНИЕ: Надежно генерируем длинную паузу из коротких
  for (int j = 0; j < 25; j++) {
      delayMicroseconds(1000); // 25 * 1000 ~= 25000 мкс
  }
  
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(D_START_PULSE);

  // 2. Отправляем 37 бит данных (со старшего к младшему)
  for (int i = 36; i >= 0; i--) {
    if ((key >> i) & 1) { // Отправляем '1'
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(D_TE_LONG);  // Длинная пауза
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(D_TE_SHORT); // Короткий импульс
    } else { // Отправляем '0'
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(D_TE_SHORT); // Короткая пауза
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(D_TE_LONG);  // Длинный импульс
    }
  }

  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Doitrand Transmitter Initialized (Final Version).");
}

void loop() {
  // --- УКАЖИТЕ ВАШ ПОЛНЫЙ 37-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  uint64_t doitrand_key = 0x123456789AULL; // Пример 37-битного ключа

  Serial.print("Sending Doitrand key: 0x");
  Serial.print((uint32_t)(doitrand_key >> 32), HEX);
  Serial.println((uint32_t)doitrand_key, HEX);
  
  // Отправляем пакет несколько раз для надежности
  for (int i = 0; i < REPEATS; i++) {
    sendDoitrandPacket(doitrand_key);
    // Пауза между повторами для разделения пакетов
    delayMicroseconds(4200); 
  }

  delay(2000); // Пауза 2 секунды между сериями отправок
}