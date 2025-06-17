/*
  ФИНАЛЬНАЯ ВЕРСИЯ скетча для Arduino Uno для отправки команд
  по протоколу Gate-TX. Исправлена проблема с таймингами.
*/

// --- Настройки ---
const int TX_PIN = 5;
const int REPEATS = 5;

// Тайминги для Gate-TX (в микросекундах)
const int GATETX_TE_SHORT = 350;
const int GATETX_TE_LONG = 700;
const int GATETX_PREAMBLE_US = GATETX_TE_SHORT * 49; // 17150 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Gate-TX Transmitter Initialized (FINAL TIMING FIX).");
}

void loop() {
  uint32_t key_24bit = 0x1A2B3C;
  
  Serial.print("Sending 24-bit Gate-TX key: 0x");
  Serial.println(key_24bit, HEX);
  
  for (int i = 0; i < REPEATS; i++) {
    sendGateTxPacket(key_24bit, 24);
    delay(30);
  }

  delay(3000);
}

/**
 * @brief Формирует и отправляет ОДИН полный и чистый пакет данных Gate-TX.
 */
void sendGateTxPacket(uint32_t data, int num_bits) {
  // 1. Преамбула (очень длинная пауза)
  // ИСПРАВЛЕНО: Разбиваем одну длинную задержку на несколько коротких для стабильности.
  digitalWrite(TX_PIN, LOW);
  for(int i = 0; i < 17; i++) {
    delayMicroseconds(1000);
  }
  delayMicroseconds(150); // 17*1000 + 150 = 17150

  // 2. Стартовый бит
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(GATETX_TE_LONG);
  
  // 3. Данные
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    if (current_bit) { // Бит '1': длинная пауза + короткий импульс
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(GATETX_TE_LONG);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(GATETX_TE_SHORT);
    } else { // Бит '0': короткая пауза + длинный импульс
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(GATETX_TE_SHORT);
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(GATETX_TE_LONG);
    }
  }
  
  digitalWrite(TX_PIN, LOW);
}