const int TX_PIN = 5;
const int REPEATS = 10;

const int TE_SHORT = 700;
const int TE_LONG = 1400;

const long KEY_DATA = 0x123456;
const int KEY_BIT_COUNT = 24;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Nice Flo Transmitter Initialized.");
  Serial.println("Press any key in Serial Monitor to send the signal.");
}

void loop() {
  Serial.print("Sending Key: 0x");
  Serial.print(KEY_DATA, HEX);
  Serial.print(" (");
  Serial.print(REPEATS);
  Serial.println(" times)...");
  
  for (int i = 0; i < REPEATS; i++) {
    sendNiceFloPacket(KEY_DATA, KEY_BIT_COUNT);
  }
  
  Serial.println("Done. Waiting for 2 seconds.");

  delay(2000);
}

void sendBit(bool bit_to_send) {
  if (bit_to_send) {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_LONG);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_SHORT);
  } else {
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(TE_SHORT);
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(TE_LONG);
  }
}

void sendNiceFloPacket(long data, int num_bits) {
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(TE_SHORT * 36);

  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(TE_SHORT);

  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendBit(current_bit);
  }

  digitalWrite(TX_PIN, LOW);
}