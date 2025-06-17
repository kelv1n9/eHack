/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Linear Delta-3.
  Написано с учетом всех особенностей протокола.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Linear Delta-3 (в микросекундах)
const int LD3_TE_SHORT = 500;
const int LD3_TE_LONG = 2000;

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Linear Delta-3 Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ 8-БИТНЫЙ КЛЮЧ ЗДЕСЬ ---
  // Код с 8-ми DIP-переключателей.
  // ВАЖНО: в отличие от других, у Delta-3 '1' - это выключенный DIP, '0' - включенный.
  // Но для эмуляции сигнала просто передаем как есть.
  uint8_t dip_code = 0b10101010; 
  
  Serial.print("Sending 8-bit Linear Delta-3 key: 0b");
  for(int i = 7; i >= 0; i--) {
      Serial.print( (dip_code >> i) & 1 );
  }
  Serial.println();
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendLinearDelta3Packet(dip_code, 8);
  }

  delay(2000); // Пауза 2 секунды
}

/**
 * @brief Формирует и отправляет полный пакет данных Linear Delta-3.
 * @param data 8-битный ключ.
 * @param num_bits Количество бит в ключе (всегда 8).
 */
void sendLinearDelta3Packet(uint8_t data, int num_bits) {
  // Протокол не имеет преамбулы или стартового бита.

  // 1. Отправляем первые 7 бит (от 7-го до 1-го)
  for (int i = num_bits - 1; i > 0; i--) {
    bool current_bit = (data >> i) & 1;
    if (current_bit) { // Бит '1': короткий HIGH + очень длинный LOW
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(LD3_TE_SHORT);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(LD3_TE_SHORT * 7);
    } else { // Бит '0': длинный HIGH + длинный LOW
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(LD3_TE_LONG);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(LD3_TE_LONG);
    }
  }

  // 2. Отправка ПОСЛЕДНЕГО (нулевого) бита с особой финальной паузой
  bool last_bit = data & 1;
  if (last_bit) { // Последний бит '1'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(LD3_TE_SHORT); // Короткий импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(LD3_TE_SHORT * 73); // Финальная пауза
  } else { // Последний бит '0'
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(LD3_TE_LONG); // Длинный импульс
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(LD3_TE_SHORT * 70); // Финальная пауза
  }
}