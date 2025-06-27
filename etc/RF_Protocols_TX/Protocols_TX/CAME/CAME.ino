const int TX_PIN = 5;
const int REPEATS = 10;

const int CAME_FAMILY_TE = 320;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("CAME Family Transmitter Initialized.");
}

void loop() {
  // --- ВЫБЕРИ И РАСКОММЕНТИРУЙ НУЖНЫЙ ПРИМЕР ---

  // --- Пример для CAME 12-bit ---
  // uint32_t key = 0xABC;
  // uint8_t bits = 12;
  // Serial.println("Sending CAME 12-bit...");
  
  
  // --- Пример для CAME 24-bit ---
  // uint32_t key = 0xABCDEF;
  // uint8_t bits = 24;
  // Serial.println("Sending CAME 24-bit...");
  

  // --- Пример для Prastel 25-bit ---
  uint32_t key = 0x1ABCDEF;
  uint8_t bits = 25;
  Serial.println("Sending Prastel 25-bit...");
  
  
  for (int i = 0; i < REPEATS; i++) {
    sendCameFamilyPacket(key, bits);
  }

  delay(3000);
}


void sendCameFamilyBit(bool bit) {
  if (bit) {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(CAME_FAMILY_TE * 2);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(CAME_FAMILY_TE);
  } else {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(CAME_FAMILY_TE);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(CAME_FAMILY_TE * 2);
  }
}

void sendCameFamilyPacket(uint64_t data, int num_bits) {
  uint32_t header_duration_us;
  
  switch (num_bits) {
    case 24:
    case 42:
      header_duration_us = 76 * CAME_FAMILY_TE;
      break;
    case 12:
    case 18:
      header_duration_us = 47 * CAME_FAMILY_TE;
      break;
    case 25:
      header_duration_us = 36 * CAME_FAMILY_TE;
      break;
    default:
      header_duration_us = 16 * CAME_FAMILY_TE;
      break;
  }
  
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(header_duration_us);

  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(CAME_FAMILY_TE);

  for (int i = num_bits - 1; i >= 0; i--) {
    sendCameFamilyBit((data >> i) & 1ULL);
  }
  
  digitalWrite(TX_PIN, LOW);
}