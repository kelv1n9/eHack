/*
  Рабочий скетч для Arduino Uno для отправки команд по протоколу KeeLoq.
  Для работы ТРЕБУЕТСЯ указать правильный серийный номер, счетчик,
  кнопку и 64-битный КЛЮЧ ПРОИЗВОДИТЕЛЯ.
*/

// --- Настройки ---
const int TX_PIN = 5;      // Пин, к которому подключен передатчик
const int REPEATS = 5;     // Количество повторов отправки пакета

// Тайминги для KeeLoq (в микросекундах)
const int KEELOQ_TE_SHORT = 400;
const int KEELOQ_TE_LONG = 800;

// Константа для нелинейной функции KeeLoq (NLF)
const uint32_t KEELOQ_NLF = 0x3A5C742E;

// --- ДАННЫЕ ВАШЕГО ПУЛЬТА (НУЖНО ЗАПОЛНИТЬ!) ---
// Серийный номер вашего пульта (28-бит)
uint32_t remote_serial = 0x01ABCDE; 
// Счетчик нажатий (16-бит), должен увеличиваться при каждом нажатии
uint16_t remote_counter = 100;
// Номер нажатой кнопки (4-бита)
uint8_t remote_button = 0xB;
// 64-битный ключ производителя (самое важное!)
// ПРИМЕР: Это НЕ настоящий ключ, а просто случайное число
uint64_t manufacturer_key = 0x0123456789ABCDEFULL; 
// -------------------------------------------------------------


// --- Функции, портированные из Flipper Zero ---

// Вспомогательная макро-функция
#define bit(x, n) (((x) >> (n)) & 1)
#define g5(x, a, b, c, d, e) (bit(x, a) + bit(x, b) * 2 + bit(x, c) * 4 + bit(x, d) * 8 + bit(x, e) * 16)

// Функция шифрования KeeLoq
uint32_t keeloq_encrypt(const uint32_t data, const uint64_t key) {
    uint32_t x = data, r;
    for(r = 0; r < 528; r++)
        x = (x >> 1) ^ ((bit(x, 0) ^ bit(x, 16) ^ (uint32_t)bit(key, r & 63) ^
                         bit(KEELOQ_NLF, g5(x, 1, 9, 20, 26, 31)))
                        << 31);
    return x;
}

// Функция реверса 64-битного числа
uint64_t reverse_key(uint64_t data, uint8_t len) {
    uint64_t temp = 0;
    for(uint8_t i = 0; i < len; i++) {
        if((data >> i) & 1) {
            temp |= (uint64_t)1 << (len - 1 - i);
        }
    }
    return temp;
}


void setup() {
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("KeeLoq Transmitter Initialized.");
}

void loop() {
  Serial.print("Sending KeeLoq packet, S/N: 0x");
  Serial.print(remote_serial, HEX);
  Serial.print(", CNT: ");
  Serial.println(remote_counter);

  // Генерируем и отправляем пакет
  sendKeeloqPacket();

  // Увеличиваем счетчик для следующей отправки
  remote_counter++;
  
  delay(3000); 
}

/**
 * @brief Отправляет один бит данных по протоколу KeeLoq.
 * @param bit_to_send Бит для отправки (true для '1', false для '0').
 */
void sendKeeloqBit(bool bit_to_send) {
  if (bit_to_send) { // Бит '1': короткий HIGH + длинный LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(KEELOQ_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(KEELOQ_TE_LONG);
  } else { // Бит '0': длинный HIGH + короткий LOW
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(KEELOQ_TE_LONG);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(KEELOQ_TE_SHORT);
  }
}

/**
 * @brief Генерирует и отправляет полный, зашифрованный пакет KeeLoq.
 */
void sendKeeloqPacket() {
  // --- Шаг 1: Формируем и шифруем данные ---
  // Формируем 32-битную незашифрованную часть
  uint32_t data_to_encrypt = (uint32_t)remote_button << 28 |
                             (remote_serial & 0x3FF) << 16 |
                             remote_counter;
                             
  // Шифруем ее с помощью ключа производителя
  // ПРИМЕЧАНИЕ: здесь реализован самый простой тип привязки `KEELOQ_LEARNING_SIMPLE`.
  // Для других систем (Normal Learning и т.д.) потребуется более сложная генерация ключа.
  uint32_t encrypted_part = keeloq_encrypt(data_to_encrypt, manufacturer_key);
  
  // Формируем 32-битную "фиксированную" (открытую) часть пакета
  uint32_t fixed_part = (uint32_t)remote_button << 28 | remote_serial;

  // Собираем полный 64-битный пакет
  uint64_t full_packet = (uint64_t)fixed_part << 32 | encrypted_part;
  
  // Переворачиваем биты в пакете, как это делает Flipper
  uint64_t reversed_packet = reverse_key(full_packet, 64);

  // --- Шаг 2: Отправляем пакет в эфир ---
  for (int r = 0; r < REPEATS; r++) {
    // 1. Преамбула (11 коротких импульсов)
    for (uint8_t i = 0; i < 11; i++) {
        digitalWrite(TX_PIN, HIGH);
        delayMicroseconds(KEELOQ_TE_SHORT);
        digitalWrite(TX_PIN, LOW);
        delayMicroseconds(KEELOQ_TE_SHORT);
    }
    // 2. Синхронизация (короткий HIGH + длинная пауза)
    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(KEELOQ_TE_SHORT);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(KEELOQ_TE_SHORT * 10);
    
    // 3. Отправка 64 бит данных
    for (int i = 63; i >= 0; i--) {
        sendKeeloqBit((reversed_packet >> i) & 1ULL);
    }
    
    // 4. Отправка 2-х статусных бит (в данном случае, два бита '1')
    sendKeeloqBit(true);
    sendKeeloqBit(true);

    // 5. Финальная пауза
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(KEELOQ_TE_SHORT * 40);
  }
}