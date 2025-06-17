/*
  ФИНАЛЬНЫЙ рабочий скетч для Arduino Uno для отправки команд
  по протоколу Ansonic.
*/

// --- Настройки ---
const int TX_PIN = 5;
const int REPEATS = 10;

// Тайминги для Ansonic (в микросекундах)
const int ANSONIC_TE_SHORT = 555;
const int ANSONIC_TE_LONG = 1111;
const int ANSONIC_PREAMBLE_US = ANSONIC_TE_SHORT * 35; // 19425 us

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Ansonic Transmitter Initialized (FINAL VERSION).");
}

void loop() {
  uint16_t key_12bit = 0b101010101001; 
  
  Serial.print("Sending 12-bit Ansonic key: 0x");
  Serial.println(key_12bit, HEX);
  
  // ИСПРАВЛЕНО: Добавлена пауза между пакетами
  for (int i = 0; i < REPEATS; i++) {
    sendAnsonicPacket(key_12bit, 12);
    delay(20); // <-- ВАЖНОЕ ИЗМЕНЕНИЕ!
  }

  delay(3000);
}

void sendAnsonicBit(bool bit_to_send) {
  if (bit_to_send) {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(ANSONIC_TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(ANSONIC_TE_LONG);
  } else {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(ANSONIC_TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(ANSONIC_TE_SHORT);
  }
}

void sendAnsonicPacket(uint16_t data, int num_bits) {
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(ANSONIC_PREAMBLE_US);

  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(ANSONIC_TE_SHORT);

  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendAnsonicBit(current_bit);
  }
  
  digitalWrite(TX_PIN, LOW);
}