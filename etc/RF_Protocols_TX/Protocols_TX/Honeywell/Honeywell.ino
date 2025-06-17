/*
  Рабочий скетч для Arduino Uno для отправки команд
  по протоколу Honeywell WDB.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 10;    // Количество повторов отправки пакета

// Тайминги для Honeywell WDB (в микросекундах)
const int HW_TE_SHORT = 160;
const int HW_TE_LONG = 320;

// --- Константы для полей данных ---
// Тип устройства
#define DEVICE_DOORBELL 0b10
#define DEVICE_PIR      0b01
// Тип оповещения
#define ALERT_NORMAL 0b00
#define ALERT_HIGH   0b01 // или 0b10
#define ALERT_FULL   0b11

void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Honeywell WDB Transmitter Initialized.");
}

void loop() {
  // --- УКАЖИТЕ ВАШИ ДАННЫЕ ЗДЕСЬ ---
  uint32_t serial = 0x12345;   // 20-битный серийный номер вашего устройства
  uint8_t device_type = DEVICE_DOORBELL; // Тип устройства
  uint8_t alert_type = ALERT_NORMAL;     // Тип оповещения
  bool secret_knock = false; // Флаг "секретного стука"
  bool low_battery = false;  // Флаг низкого заряда батареи
  
  // 1. Собираем правильный 48-битный пакет с расчетом бита четности
  uint64_t packet_to_send = buildHoneywellPacket(serial, device_type, alert_type, secret_knock, low_battery);

  Serial.print("Sending 48-bit Honeywell WDB packet: 0x");
  Serial.print((uint32_t)(packet_to_send >> 32), HEX);
  Serial.println((uint32_t)packet_to_send, HEX);
  
  // 2. Отправляем всю серию
  for (int i = 0; i < REPEATS; i++) {
    sendHoneywellPacket(packet_to_send);
  }

  delay(2000); 
}

/**
 * @brief Собирает валидный 48-битный пакет Honeywell WDB.
 * @param serial 20-битный серийный номер.
 * @param dev_type Код типа устройства.
 * @param alert Код типа оповещения.
 * @param knock Флаг "секретного стука".
 * @param low_bat Флаг низкого заряда батареи.
 * @return Готовый к отправке 48-битный пакет.
 */
uint64_t buildHoneywellPacket(uint32_t serial, uint8_t dev_type, uint8_t alert, bool knock, bool low_bat) {
  uint64_t packet = 0;

  // Собираем первые 47 бит пакета
  packet |= (serial & 0xFFFFF) << 28;      // 20 бит S/N
  packet |= (uint64_t)(dev_type & 0x3) << 20; // 2 бита типа устройства
  packet |= (uint64_t)(alert & 0x3) << 16;   // 2 бита типа оповещения
  // 9 бит неизвестных флагов (пока оставляем нулями)
  packet |= (uint64_t)(knock & 0x1) << 4;    // 1 бит Secret Knock
  // 1 бит флага ретрансляции (оставляем 0)
  packet |= (uint64_t)(low_bat & 0x1) << 1;  // 1 бит Low Battery

  // Рассчитываем бит четности (Parity)
  // Это LSB (младший бит) от количества установленных бит в первых 47 битах.
  uint8_t set_bits_count = 0;
  for (int i = 1; i < 48; i++) {
    if ((packet >> i) & 1ULL) {
      set_bits_count++;
    }
  }
  uint8_t parity_bit = set_bits_count & 1;
  
  // Добавляем бит четности в конец
  packet |= parity_bit;
  
  return packet;
}

/**
 * @brief Отправляет один бит данных по протоколу Honeywell WDB.
 */
void sendHoneywellBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': длинный HIGH + короткий LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HW_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HW_TE_SHORT);
  } else { // Бит '0': короткий HIGH + длинный LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(HW_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(HW_TE_LONG);
  }
}

/**
 * @brief Отправляет полный пакет данных Honeywell WDB.
 */
void sendHoneywellPacket(uint64_t data) {
  // 1. Преамбула
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(HW_TE_SHORT * 3);

  // 2. Данные (48 бит)
  for (int i = 47; i >= 0; i--) {
    sendHoneywellBit((data >> i) & 1ULL);
  }
  
  // 3. Стоп-бит
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(HW_TE_SHORT * 3);
  digitalWrite(TX_PIN, LOW);
}