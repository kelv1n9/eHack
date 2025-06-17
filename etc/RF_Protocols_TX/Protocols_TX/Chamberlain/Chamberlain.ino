/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Chamberlain Code (старые системы с DIP-переключателями).
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 8;     // Количество повторов отправки пакета

// Тайминги для Chamberlain Code (в микросекундах)
const int CHAMB_TE = 1000;

// Константы для символов
#define SYMBOL_0 0
#define SYMBOL_1 1
#define SYMBOL_STOP 2

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Chamberlain Code Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШ КОД И ЕГО ДЛИНУ ЗДЕСЬ ---
  // Этот протокол может иметь 7, 8 или 9 бит.
  // Выберите нужный и раскомментируйте соответствующий блок.
  
  // Пример для 9-битного пульта
  uint16_t dip_code = 0b101010101; 
  uint8_t bit_length = 9;

  /*
  // Пример для 8-битного пульта
  uint16_t dip_code = 0b10101010; 
  uint8_t bit_length = 8;
  */

  /*
  // Пример для 7-битного пульта
  uint16_t dip_code = 0b1010101; 
  uint8_t bit_length = 7;
  */

  Serial.print("Sending ");
  Serial.print(bit_length);
  Serial.print("-bit Chamberlain key: 0x");
  Serial.println(dip_code, HEX);
  
  // Отправляем пакет несколько раз
  for (int i = 0; i < REPEATS; i++) {
    sendChamberlainPacket(dip_code, bit_length);
  }

  delay(3000); // Пауза 3 секунды
}

/**
 * @brief Отправляет один символ протокола Chamberlain Code.
 * @param symbol Тип символа для отправки (SYMBOL_0, SYMBOL_1, or SYMBOL_STOP).
 */
void sendChamberlainSymbol(uint8_t symbol) {
  uint32_t pause_us, pulse_us;

  switch(symbol) {
    case SYMBOL_0:
      pause_us = CHAMB_TE * 1;
      pulse_us = CHAMB_TE * 3;
      break;
    case SYMBOL_1:
      pause_us = CHAMB_TE * 2;
      pulse_us = CHAMB_TE * 2;
      break;
    case SYMBOL_STOP:
      pause_us = CHAMB_TE * 3;
      pulse_us = CHAMB_TE * 1;
      break;
    default:
      return; // Не отправляем ничего, если символ неизвестен
  }

  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(pause_us);
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(pulse_us);
}

/**
 * @brief Формирует и отправляет полный пакет данных Chamberlain Code.
 * @param data Код с DIP-переключателей.
 * @param num_bits Количество бит в ключе (7, 8 или 9).
 */
void sendChamberlainPacket(uint16_t data, int num_bits) {
  // 1. Преамбула (длинная пауза)
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(CHAMB_TE * 39);

  // 2. Стартовый бит (короткий импульс)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(CHAMB_TE);

  // 3. Данные (отправляем биты как символы)
  for (int i = num_bits - 1; i >= 0; i--) {
    bool current_bit = (data >> i) & 1;
    sendChamberlainSymbol(current_bit ? SYMBOL_1 : SYMBOL_0);
  }
  
  // 4. Отправляем Стоп-символ
  sendChamberlainSymbol(SYMBOL_STOP);

  // Завершаем передачу, устанавливая низкий уровень
  digitalWrite(TX_PIN, LOW);
}