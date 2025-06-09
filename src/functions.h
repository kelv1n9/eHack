#define EB_DEB_TIME 15
#define EB_HOLD_TIME 300
#define NFC_INTERFACE_I2C

#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <PN532_I2C.h>
#include <PN532_I2C.cpp>
#include <PN532.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <RCSwitch.h>
#include <IRremote.hpp>
#include "signal_data.h"
#include "hardware/adc.h"
#include "RF24.h"
#include <rdm6300.h>

#define APP_NAME "eHack"
#define APP_VERSION "v3.1.3"

#define BLE_PIN 18

#define VIBRO 16

#define BTN_DOWN 9
#define BTN_OK 10
#define BTN_UP 11

#define IR_TX 2
#define IR_RX 3

Button up(BTN_UP);
Button ok(BTN_OK);
Button down(BTN_DOWN);

bool initialized = false;
bool locked = false;

uint8_t mainMenuCount = 6;
uint8_t hfMenuCount = 6;
uint8_t irMenuCount = 4;
uint8_t uhfMenuCount = 6;
uint8_t RFIDMenuCount = 3;
uint8_t gamesMenuCount = 3;
uint8_t barrierMenuCount = 3;
uint8_t RAsignalMenuCount = 2;
uint8_t barrierBruteMenuCount = 2;

uint8_t MAINmenuIndex = 0;
uint8_t HFmenuIndex = 0;
uint8_t IRmenuIndex = 0;
uint8_t uhfMenuIndex = 0;
uint8_t RFIDMenuIndex = 0;
uint8_t gamesMenuIndex = 0;
uint8_t barrierMenuIndex = 0;
uint8_t RAsignalMenuIndex = 0;
uint8_t barrierBruteMenuIndex = 0;

/* ================= Battery ================== */
#define BATTERY_COEFFICIENT 0.9611905
#define R1 200000 // 200k
#define R2 100000 // 100k
#define BATTERY_RESISTANCE_COEFFICIENT (1 + R1 / R2)
#define V_REF 3.3

#define BATTERY_CHECK_INTERVAL 10000
#define BATTERY_MIN_VOLTAGE 3.5
#define BATTERY_MAX_VOLTAGE 4.2
#define BATTERY_READ_ITERATIONS 10

#define HISTORY_SIZE 5
#define CHARGE_THRESHOLD 0.002

float batVoltage;
float voltageHistory[HISTORY_SIZE] = {0};
uint32_t batteryTimer;
uint8_t historyIndex = 0;
bool isCharging = false;

/* ==================== OLED ================== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MIN_BRIGHTNESS 5
#define MAX_BRIGHTNESS 255
#define BRIGHTNESS_TIME 30000
#define SPECTRUM_TOP_LIMIT 10
#define SCREEN_BOTTOM 63

GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

bool peaksDynamic = true;
uint32_t brightnessTimer;

/* ==================== Tesla ================== */
#define pulseWidth 400     // µs
#define messageDistance 23 // ms
#define transmissions 5

/* =================== SubGHz MHz ================== */
#define GD0_PIN_CC 19
#define CSN_PIN_CC 17
#define RSSI_WINDOW_MS 100
#define RSSI_STEP_MS 50
#define RSSI_BUFFER_SIZE 90
#define MAX_SEGMENTS 64

int currentRssi = -100;
uint8_t currentFreqIndex = 1;
uint8_t currentScanFreq = 0;
const float raFrequencies[] = {315.0, 433.92, 868.0, 915.0};
const uint8_t raFreqCount = sizeof(raFrequencies) / sizeof(raFrequencies[0]);
float rssiMaxPeak[raFreqCount] = {-100, -100, -100, -100};
float rssiAbsoluteMax[raFreqCount] = {-100, -100, -100, -100};

uint8_t rssiIndex = 0;
int rssiBuffer[RSSI_BUFFER_SIZE];

RCSwitch mySwitch = RCSwitch();

struct SimpleRAData
{
  uint32_t code;
  uint16_t length;
  uint16_t protocol;
  uint16_t delay;
};

uint8_t lastUsedSlotRA = 0;
uint8_t selectedSlotRA = 0;

uint32_t capturedCode;
uint16_t capturedLength;
uint16_t capturedProtocol;
uint16_t capturedDelay;

bool mySwitchIsAvailable = false;
uint16_t mySwitchSetTime = 0;

bool attackIsActive = false;
bool signalCaptured_433MHZ = false;

/* ================ Barrier =================== */
#define MAX_DELTA_T_BARRIER 200
#define AN_MOTORS_PULSE 412

struct SimpleBarrierData
{
  uint32_t codeMain;
  uint32_t codeAdd;
  uint8_t protocol;
};

int16_t barrierBruteIndex = 4095;
volatile uint32_t barrierCodeMain, barrierCodeAdd;
volatile uint8_t barrierProtocol;
volatile uint8_t barrierBit;

uint8_t selectedSlotBarrier = 0;
uint8_t lastUsedSlotBarrier = 0;

volatile uint16_t lastEdgeMicros;
volatile uint16_t lowDurationMicros, highDurationMicros, barrierCurrentLevel;

// AN Motors
volatile byte anMotorsCounter = 0;      // количество принятых битов
volatile long code1 = 0;                // зашифрованная часть
volatile long code2 = 0;                // фиксированная часть
volatile bool anMotorsCaptured = false; // флаг, что код принят
// CAME
volatile byte cameCounter = 0;  // сохраненое количество бит
volatile uint32_t cameCode = 0; // код Came
volatile bool cameCaptured = false;
// NICE
volatile byte niceCounter = 0;  // сохраненое количество бит
volatile uint32_t niceCode = 0; // код Nice
volatile bool niceCaptured = false;

/* ================= IR ================== */
#define DELAY_AFTER_SEND 50
#define IR_N_REPEATS 0

struct SimpleIRData
{
  uint8_t protocol;
  uint16_t address;
  uint16_t command;
};

enum BruteState
{
  BRUTE_IDLE,
  BRUTE_RUNNING,
  BRUTE_PAUSED
};
BruteState bruteState = BRUTE_IDLE;

uint8_t lastUsedSlotIR = 0;
uint8_t selectedSlotIR = 0;
uint8_t currentIndex;
uint32_t lastSendTime_IR_BRUTE = 0;
uint16_t protocol, address, command;

bool signalCaptured_IR = false;

/*=============== GAMES ====================== */
// SNAKE
#define SNAKE_WIDTH 32
#define SNAKE_HEIGHT 16
#define SNAKE_PIXEL 4
#define SNAKE_MOVE_DELAY_MS 100
#define SNAKE_FAST_DELAY_MS 50

// Flappy
#define FLAPPY_WIDTH 128
#define FLAPPY_HEIGHT 64
#define BIRD_SIZE 4
#define GRAVITY 0.5
#define JUMP_STRENGTH -2
#define TUBE_WIDTH 10
#define GAP_HEIGHT 30
#define NUM_TUBES 2

struct GameScores
{
  uint16_t snakeMax;
  uint16_t flappyMax;
  uint16_t dotsMax;
};
GameScores gameScores;

struct SnakeSegment
{
  int8_t x, y;
};

struct Position
{
  int8_t x, y;
};

const Position snakeDirs[4] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

struct Tube
{
  int16_t x;
  int8_t gapY;
};

/*=================== EEPROM ==========================*/
#define MAX_IR_SIGNALS 10
#define MAX_RA_SIGNALS 10
#define MAX_BARRIER_SIGNALS 10
#define MAX_RFID 5

#define SLOT_IR_SIZE sizeof(SimpleIRData)
#define SLOT_RA_SIZE sizeof(SimpleRAData)
#define SLOT_BARRIER_SIZE sizeof(SimpleBarrierData)
#define SLOT_SCORE_SIZE sizeof(GameScores)
#define SLOT_SETTINGS_SIZE sizeof(Settings)
#define SLOT_RFID_SIZE sizeof(RFID)

#define EEPROM_IR_START 0
#define EEPROM_RA_START (EEPROM_IR_START + MAX_IR_SIGNALS * SLOT_IR_SIZE)
#define EEPROM_BARRIER_START (EEPROM_RA_START + MAX_RA_SIGNALS * SLOT_RA_SIZE)
#define EEPROM_SCORE_START (EEPROM_BARRIER_START + MAX_BARRIER_SIGNALS * SLOT_BARRIER_SIZE)
#define EEPROM_SETTINGS_START (EEPROM_SCORE_START + SLOT_SCORE_SIZE)
#define EEPROM_RFID_START (EEPROM_SETTINGS_START + SLOT_RFID_SIZE * MAX_RFID)

/*=================== SETTINGS ==========================*/
struct Settings
{
  bool saveIR;
  bool saveRA;
};
Settings settings;

const uint8_t settingsMenuCount = 5;
// ================= 2.4 GHZ ===========================/
#define CE_PIN_NRF 21
#define CSN_PIN_NRF 20
#define START_CHANNEL 45
#define NUM_CHANNELS 126

RF24 radio(CE_PIN_NRF, CSN_PIN_NRF, 16000000);

const uint8_t cacheMax = 15;
const uint16_t margin = 1;
const uint16_t barWidth = (SCREEN_WIDTH - (margin * 2)) / NUM_CHANNELS;
const uint16_t chartHeight = SCREEN_HEIGHT - 10;
const uint16_t chartWidth = margin * 2 + (NUM_CHANNELS * barWidth);
uint8_t channelStrength[NUM_CHANNELS];

struct ChannelHistory
{
  uint8_t maxPeak = 0;

  uint8_t push(bool value)
  {
    uint8_t sum = value;
    for (uint8_t i = 0; i < cacheMax - 1; ++i)
    {
      history[i] = history[i + 1];
      sum += history[i];
    }
    history[cacheMax - 1] = value;
    maxPeak = max((uint8_t)(sum * 2), maxPeak);
    return sum;
  }

private:
  bool history[cacheMax] = {0};
};
ChannelHistory stored[126];

uint8_t radioChannel = 0;
/*========================== RFID ==============================*/
#define RFID_COIL_PIN 14
#define RFID_RX_PIN 15

Rdm6300 rdm6300;

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

struct RFID
{
  uint32_t tagID; // 125kHz
};
RFID rfid_mem;

uint8_t lastUsedSlotRFID = 0;
uint8_t selectedSlotRFID = 0;

uint32_t tagID_125kHz;
uint8_t tagDetected = 0; // 0 = none, 1 = 125 kHz, 2 = 13.56 MHz
uint8_t nfcCardType = 0; // 0 = none, 1 = Mifare Classic, 2 = Mifare Ultralight

uint8_t nfcData[32];
uint8_t nfcDataLength = 0;
uint8_t tagID_NFC[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
uint8_t tagIDLength_NFC;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
bool nfcDataValid = false;

/*======================= FUNCTIONS ============================*/

/*======================== COMMON ==============================*/
void resetBrightness()
{
  brightnessTimer = millis();
  oled.setContrast(MAX_BRIGHTNESS);
}

template <typename T>
T findMaxValue(const T *array, size_t length)
{
  T maxVal = array[0];
  for (size_t i = 1; i < length; i++)
  {
    if (array[i] > maxVal)
    {
      maxVal = array[i];
    }
  }
  return maxVal;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int getTextWidth(const char *text)
{
  return strlen(text) * 6;
}

void vibro(uint8_t intensity = 120, uint16_t duration = 100, uint8_t repeat = 1, uint32_t pause = 150)
{
  IrReceiver.stopTimer();
  for (uint8_t i = 0; i < repeat; i++)
  {
    analogWrite(VIBRO, intensity);
    delay(duration);
    analogWrite(VIBRO, 0);
    delay(pause);
  }
  delay(10);
  IrReceiver.restartTimer();
}

void cheсkCharging(float newVoltage)
{
  uint8_t growthCount = 0;

  voltageHistory[historyIndex] = newVoltage;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  for (uint8_t i = 0; i < HISTORY_SIZE - 1; i++)
  {
    uint8_t idx1 = (historyIndex + i) % HISTORY_SIZE;
    uint8_t idx2 = (historyIndex + i + 1) % HISTORY_SIZE;

    float delta = voltageHistory[idx2] - voltageHistory[idx1];
    if (delta >= CHARGE_THRESHOLD)
    {
      growthCount++;
    }
  }

  if (growthCount >= 3)
  {
    isCharging = true;
  }
  else
  {
    isCharging = false;
  }
}

float readBatteryVoltage()
{
  uint16_t total = 0;
  for (int i = 0; i < BATTERY_READ_ITERATIONS; i++)
  {
    total += analogRead(A3);
    delayMicroseconds(500);
  }

  return (float)BATTERY_COEFFICIENT * (total / (float)BATTERY_READ_ITERATIONS) * (float)BATTERY_RESISTANCE_COEFFICIENT * (float)V_REF / 4095.0;
}

void menuButtons(uint8_t &page, uint8_t MAX_PAGES)
{
  if (!locked && (up.click() || up.step()))
  {
    page = (page + MAX_PAGES - 1) % MAX_PAGES;
    vibro(255, 20);
  }
  if (!locked && (down.click() || down.step()))
  {
    page = (page + 1) % MAX_PAGES;
    vibro(255, 20);
  }
}

void getIRCommand(const uint16_t *data, uint8_t index, uint16_t &protocol, uint16_t &address, uint16_t &command)
{
  protocol = pgm_read_word(&data[index * 3]);
  address = pgm_read_word(&data[index * 3 + 1]);
  command = pgm_read_word(&data[index * 3 + 2]);
}

//================================== HF ======================================*/

void changeFreqButtons(const char *mode)
{
  if (!locked && (down.click()))
  {
    currentFreqIndex = (currentFreqIndex + 1) % raFreqCount;

    if (strcmp(mode, "RX") == 0)
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
    else if (strcmp(mode, "TX") == 0)
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

    vibro(255, 30);
  }
  else if (!locked && (up.click()))
  {
    currentFreqIndex = (currentFreqIndex == 0 ? raFreqCount - 1 : currentFreqIndex - 1);

    if (strcmp(mode, "RX") == 0)
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
    else if (strcmp(mode, "TX") == 0)
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

    vibro(255, 30);
  }
}

void resetSpectrum_HF()
{
  for (uint8_t i = 0; i < raFreqCount; i++)
  {
    rssiMaxPeak[i] = -100;
    rssiAbsoluteMax[i] = -100;
  }
}

//================================== TESLA ======================================*/

void sendByte(uint8_t dataByte)
{
  for (int8_t bit = 7; bit >= 0; bit--)
  { // MSB
    digitalWrite(GD0_PIN_CC, (dataByte & (1 << bit)) != 0 ? HIGH : LOW);
    delayMicroseconds(pulseWidth);
  }
}

void sendTeslaSignal_v1()
{
  for (uint8_t t = 0; t < transmissions; t++)
  {
    for (uint8_t i = 0; i < 43; i++)
      sendByte(teslaSequence[i]);
    digitalWrite(GD0_PIN_CC, LOW);
    delay(messageDistance);
  }
}

//================================ TESLA V2 =====================================*/
// creates the preamble before the 3 repetitions of the open command
void SendPreamble()
{
  // transmit preamble: 13 times high to low + add the "chirp" on top
  for (int ii = 0; ii < 13; ii++)
  {
    digitalWrite(GD0_PIN_CC, 1);
    delayMicroseconds(pulseWidth + (12 - ii) * 10);
    digitalWrite(GD0_PIN_CC, 0);
    delayMicroseconds(pulseWidth - (12 - ii) * 10);
  }

  // space between preamble and Manchester Code
  delayMicroseconds(pulseWidth);
}

void SendManchester()
{
  int iByte, iBit;

  for (iByte = 0; iByte < 6; iByte++)
  {
    int cValue = bManCode[iByte];
    for (iBit = 0; iBit < 8; iBit++)
    {
      int bBitVal = 0;
      if ((cValue & (1 << iBit)) != 0)
      {
        // transmit one; i.e. we need a negative edge
        digitalWrite(GD0_PIN_CC, 1);
        delayMicroseconds(pulseWidth);
        digitalWrite(GD0_PIN_CC, 0);
        delayMicroseconds(pulseWidth);
      }
      else
      {
        // transmit zero; i.e. we need a positive edge
        digitalWrite(GD0_PIN_CC, 0);
        delayMicroseconds(pulseWidth);
        digitalWrite(GD0_PIN_CC, 1);
        delayMicroseconds(pulseWidth);
      }
    }
  }
}

void SendPostamble(bool bLast)
{
  // send Postamble
  digitalWrite(GD0_PIN_CC, 0); // 400us low
  delayMicroseconds(pulseWidth);
  digitalWrite(GD0_PIN_CC, 1); // 400us high
  delayMicroseconds(pulseWidth);
  digitalWrite(GD0_PIN_CC, 0); // 400us low
  delayMicroseconds(pulseWidth);

  int iMulti = 2;
  if (bLast)
  {
    iMulti = 1;
  }
  digitalWrite(GD0_PIN_CC, 1); // 800us high, 400us on last repetition
  delayMicroseconds(iMulti * pulseWidth);
  digitalWrite(GD0_PIN_CC, 0); // 800us low
  delayMicroseconds(pulseWidth);
}

void sendTeslaSignal_v2()
{
  // message repetition loop
  for (int iTransmit = 0; iTransmit < 10; iTransmit++)
  {
    SendPreamble();

    // repeat Manchester & Postamble 3 times
    for (int iRepeat = 0; iRepeat < 3; iRepeat++)
    {
      // Manchester Code & Postamble
      SendManchester();
      SendPostamble(iRepeat == 2);
    }

    delay(25); // 25ms delay between transmissions
  }
}

//======================= EEPROM ============================*/
/************************** HF *******************************/

void writeBarrierData(uint8_t slot, const SimpleBarrierData &data)
{
  int addr = EEPROM_BARRIER_START + slot * SLOT_BARRIER_SIZE;
  EEPROM.put(addr, data);
  EEPROM.commit();
}

SimpleRAData readRAData(uint8_t slot)
{
  int addr = EEPROM_RA_START + slot * SLOT_RA_SIZE;
  SimpleRAData data;
  EEPROM.get(addr, data);
  return data;
}

SimpleBarrierData readBarrierData(uint8_t slot)
{
  int addr = EEPROM_BARRIER_START + slot * SLOT_BARRIER_SIZE;
  SimpleBarrierData data;
  EEPROM.get(addr, data);
  return data;
}

void clearAllRAData()
{
  for (uint8_t slot = 0; slot < MAX_RA_SIGNALS; slot++)
  {
    SimpleRAData empty = {0, 0, 0, 0};
    int addr = EEPROM_RA_START + slot * SLOT_RA_SIZE;
    EEPROM.put(addr, empty);
  }

  for (uint8_t slot = 0; slot < MAX_BARRIER_SIGNALS; slot++)
  {
    SimpleRAData empty = {0, 0, 0};
    int addr = EEPROM_BARRIER_START + slot * SLOT_BARRIER_SIZE;
    EEPROM.put(addr, empty);
  }
  EEPROM.commit();

  lastUsedSlotRA = 0;
  lastUsedSlotBarrier = 0;
}

void clearRAData(uint8_t slot)
{
  SimpleRAData empty = {0, 0, 0, 0};
  int addr = EEPROM_RA_START + slot * SLOT_RA_SIZE;
  EEPROM.put(addr, empty);
  EEPROM.commit();

  lastUsedSlotRA = slot;
}

void clearBarrierData(uint8_t slot)
{
  SimpleRAData empty = {0, 0, 0};
  int addr = EEPROM_BARRIER_START + slot * SLOT_BARRIER_SIZE;
  EEPROM.put(addr, empty);
  EEPROM.commit();

  lastUsedSlotBarrier = slot;
}

void findLastUsedSlotRA()
{
  for (uint8_t slot = 0; slot < MAX_RA_SIGNALS; slot++)
  {
    SimpleRAData data = readRAData(slot);
    if (data.code == 0)
    {
      lastUsedSlotRA = slot;
      return;
    }
  }
  lastUsedSlotRA = 0;
}

void findLastUsedSlotBarrier()
{
  for (uint8_t slot = 0; slot < MAX_BARRIER_SIGNALS; slot++)
  {
    SimpleBarrierData data = readBarrierData(slot);
    if (data.protocol == 0 && data.codeMain == 0 && data.codeAdd == 0)
    {
      lastUsedSlotBarrier = slot;
      return;
    }
  }
  lastUsedSlotBarrier = 0;
}

bool isDuplicateRA(const SimpleRAData &newData)
{
  for (uint8_t i = 0; i < lastUsedSlotRA; i++)
  {
    SimpleRAData existingData = readRAData(i);
    if (newData.code == existingData.code &&
        newData.length == existingData.length &&
        newData.protocol == existingData.protocol)
    {
      return true;
    }
  }
  return false;
}

bool isDuplicateBarrier(const SimpleBarrierData &newData)
{
  for (uint8_t i = 0; i < lastUsedSlotBarrier; i++)
  {
    SimpleBarrierData existingData = readBarrierData(i);
    if (newData.codeMain == existingData.codeMain &&
        newData.codeAdd == existingData.codeAdd &&
        newData.protocol == existingData.protocol)
    {
      return true;
    }
  }
  return false;
}

/************************** IR *******************************/

void writeIRData(uint8_t slot, const SimpleIRData &data)
{
  int addr = EEPROM_IR_START + slot * SLOT_IR_SIZE;
  EEPROM.put(addr, data);
  EEPROM.commit();
}

SimpleIRData readIRData(uint8_t slot)
{
  int addr = EEPROM_IR_START + slot * SLOT_IR_SIZE;
  SimpleIRData data;
  EEPROM.get(addr, data);
  return data;
}

void clearAllIRData()
{
  for (uint8_t slot = 0; slot < MAX_IR_SIGNALS; slot++)
  {
    SimpleIRData empty = {0, 0, 0};
    int addr = EEPROM_IR_START + slot * SLOT_IR_SIZE;
    EEPROM.put(addr, empty);
  }
  EEPROM.commit();

  lastUsedSlotIR = 0;
}

void clearIRData(uint8_t slot)
{
  SimpleRAData empty = {0, 0, 0};
  int addr = EEPROM_IR_START + slot * SLOT_IR_SIZE;
  EEPROM.put(addr, empty);
  EEPROM.commit();

  lastUsedSlotIR = slot;
}

void writeRAData(uint8_t slot, const SimpleRAData &data)
{
  int addr = EEPROM_RA_START + slot * SLOT_RA_SIZE;
  EEPROM.put(addr, data);
  EEPROM.commit();
}

void findLastUsedSlotIR()
{
  for (uint8_t slot = 0; slot < MAX_IR_SIGNALS; slot++)
  {
    SimpleIRData data = readIRData(slot);
    if (data.protocol == 0 && data.address == 0 && data.command == 0)
    {
      lastUsedSlotIR = slot;
      return;
    }
  }
  lastUsedSlotIR = 0;
}

bool isDuplicateIR(const SimpleIRData &newData)
{
  for (uint8_t i = 0; i < lastUsedSlotRA; i++)
  {
    SimpleIRData existingData = readIRData(i);
    if (newData.protocol == existingData.protocol &&
        newData.address == existingData.address &&
        newData.command == existingData.command)
    {
      return true;
    }
  }
  return false;
}

/************************** RFID *******************************/

void writeRFIDData(uint8_t slot, const RFID &data)
{
  int addr = EEPROM_RFID_START + slot * SLOT_RFID_SIZE;
  EEPROM.put(addr, data);
  EEPROM.commit();
}

RFID readRFIDData(uint8_t slot)
{
  int addr = EEPROM_RFID_START + slot * SLOT_RFID_SIZE;
  RFID data;
  EEPROM.get(addr, data);
  return data;
}

void clearRFIDData(uint8_t slot)
{
  RFID empty = {0};
  int addr = EEPROM_RFID_START + slot * SLOT_RFID_SIZE;
  EEPROM.put(addr, empty);
  EEPROM.commit();

  lastUsedSlotRFID = slot;
}

void findLastUsedSlotRFID()
{
  for (uint8_t slot = 0; slot < MAX_RFID; slot++)
  {
    RFID data = readRFIDData(slot);
    if (data.tagID == 0)
    {
      lastUsedSlotRFID = slot;
      return;
    }
  }
  lastUsedSlotRFID = 0;
}

bool isDuplicateRFID(const RFID &newData)
{
  for (uint8_t i = 0; i < lastUsedSlotRFID; i++)
  {
    RFID existingData = readRFIDData(i);
    if (newData.tagID == existingData.tagID)
    {
      return true;
    }
  }
  return false;
}

/************************** COMMON *******************************/

void saveSettings()
{
  EEPROM.put(EEPROM_SETTINGS_START, settings);
  EEPROM.commit();
}

void loadSettings()
{
  EEPROM.get(EEPROM_SETTINGS_START, settings);
}

void loadAllScores()
{
  EEPROM.get(EEPROM_SCORE_START, gameScores);
}

void saveAllScores()
{
  EEPROM.put(EEPROM_SCORE_START, gameScores);
  EEPROM.commit();
}

/*================================== GAMES ======================================*/
/*********************************** DOTS ***************************************/
struct FallingDotsGame
{
  int16_t playerX = 44;
  const uint8_t playerWidth = 36;
  const uint8_t playerY = 56;
  const uint8_t playerSpeed = 6;
  bool initialized = false;
  uint16_t maxScore;

  uint8_t dotX = 0;
  uint8_t dotY = 0;

  uint8_t lives = 3;
  uint16_t score = 0;
  uint32_t lastFallTime = 0;
  uint16_t fallDelay = 20;
  uint8_t deltaY = 3;
  bool gameOver = false;

  void reset()
  {
    dotY = 0;
    dotX = random(0, 122);
    playerX = (128 - playerWidth) / 2;
    score = 0;
    lives = 3;
    fallDelay = 20;
    gameOver = false;
    lastFallTime = millis();
    maxScore = gameScores.dotsMax;
  }

  void update()
  {
    if (gameOver)
    {
      if (score > gameScores.dotsMax)
      {
        gameScores.dotsMax = score;
        saveAllScores();
      }
      oled.setScale(2);

      const char *msg = "Game Over!";
      char scoreStr[20];
      sprintf(scoreStr, "Score: %d", score);

      oled.setCursorXY((128 - 2 * getTextWidth(msg)) / 2, 10);
      oled.print(msg);

      oled.setCursorXY((128 - 2 * getTextWidth(scoreStr)) / 2, 30);
      oled.print(scoreStr);

      oled.setScale(1);
      char maxStr[20];
      sprintf(maxStr, "Best score: %d", gameScores.dotsMax);
      oled.setCursorXY((128 - getTextWidth(maxStr)) / 2, 50);
      oled.print(maxStr);

      return;
    }

    if (millis() - lastFallTime > fallDelay)
    {
      dotY += deltaY;
      lastFallTime = millis();

      if (dotY >= playerY)
      {
        if (dotX + 6 >= playerX && dotX <= playerX + playerWidth)
        {
          score++;
          if (fallDelay > 10)
            fallDelay -= 2;
        }
        else
        {
          lives--;
          if (lives == 0)
          {
            gameOver = true;
            vibro(255, 50, 3);
          }
          else
          {
            vibro(255, 50);
          }
        }
        dotY = 0;
        dotX = random(0, 122);
      }
    }

    draw();
  }

  void draw()
  {
    // dot
    oled.rect(dotX, dotY, dotX + 6, dotY + 6, 1);
    // player
    oled.rect(playerX, playerY, playerX + playerWidth, playerY + 4, 1);
  }

  void handleInput()
  {
    if (!locked && up.pressing() && playerX > 0)
    {
      playerX -= playerSpeed;
      if (playerX < 0)
        playerX = 0;
    }
    if (!locked && down.pressing() && playerX + playerWidth < 128)
    {
      playerX += playerSpeed;
      if (playerX + playerWidth > 128)
        playerX = 128 - playerWidth;
    }
  }
};
FallingDotsGame fallingDots;

/*********************************** SNAKE ***************************************/

struct SnakeGame
{
  SnakeSegment body[64];
  uint8_t length = 3;
  uint8_t dir = 0;
  uint32_t lastMove = 0;
  int8_t foodX = 5, foodY = 5;
  uint16_t moveDelay = SNAKE_MOVE_DELAY_MS;
  uint16_t maxScore;
  bool initialized = false;
  bool gameOver = false;

  void reset()
  {
    length = 3;
    dir = 0;
    gameOver = false;
    for (uint8_t i = 0; i < length; i++)
    {
      body[i] = {int8_t(5 - i), 5};
    }
    spawnFood();
    lastMove = millis();
    maxScore = gameScores.snakeMax;
  }

  bool isOccupied(int8_t x, int8_t y)
  {
    for (uint8_t i = 0; i < length; i++)
    {
      if (body[i].x == x && body[i].y == y)
        return true;
    }
    return false;
  }

  void spawnFood()
  {
    do
    {
      foodX = random(0, SNAKE_WIDTH);
      foodY = random(0, SNAKE_HEIGHT);
    } while (isOccupied(foodX, foodY));
  }

  bool checkSelfCollision()
  {
    for (uint8_t i = 1; i < length; i++)
    {
      if (body[0].x == body[i].x && body[0].y == body[i].y)
      {
        return true;
      }
    }
    return false;
  }

  void move()
  {
    for (int i = length - 1; i > 0; i--)
    {
      body[i] = body[i - 1];
    }

    body[0].x += snakeDirs[dir].x;
    body[0].y += snakeDirs[dir].y;

    if (body[0].x >= SNAKE_WIDTH)
      body[0].x = 0;
    if (body[0].x < 0)
      body[0].x = SNAKE_WIDTH - 1;
    if (body[0].y >= SNAKE_HEIGHT)
      body[0].y = 0;
    if (body[0].y < 0)
      body[0].y = SNAKE_HEIGHT - 1;

    if (checkSelfCollision())
    {
      gameOver = true;
      vibro(255, 50, 3);
    }

    if (body[0].x == foodX && body[0].y == foodY && length < 64)
    {
      body[length] = body[length - 1];
      length++;
      spawnFood();
      vibro(255, 20);
    }
  }

  void draw()
  {
    for (uint8_t i = 0; i < length; i++)
    {
      oled.rect(body[i].x * SNAKE_PIXEL, body[i].y * SNAKE_PIXEL,
                body[i].x * SNAKE_PIXEL + SNAKE_PIXEL - 1,
                body[i].y * SNAKE_PIXEL + SNAKE_PIXEL - 1, 1);
    }
    oled.rect(foodX * SNAKE_PIXEL, foodY * SNAKE_PIXEL,
              foodX * SNAKE_PIXEL + SNAKE_PIXEL - 1,
              foodY * SNAKE_PIXEL + SNAKE_PIXEL - 1, 1);
  }

  void update()
  {
    if (gameOver)
    {
      if (length - 3 > gameScores.snakeMax)
      {
        gameScores.snakeMax = length - 3;
        saveAllScores();
      }

      oled.setScale(2);

      const char *msg = "Game Over!";
      const char *label = "Score: ";
      int score = length - 3;

      oled.setCursorXY((128 - 2 * getTextWidth(msg)) / 2, 10);
      oled.print(msg);

      char scoreStr[20];
      sprintf(scoreStr, "Score: %d", score);
      oled.setCursorXY((128 - 2 * getTextWidth(scoreStr)) / 2, 30);
      oled.print(scoreStr);

      oled.setScale(1);
      char maxStr[20];
      sprintf(maxStr, "Best score: %d", gameScores.snakeMax);
      oled.setCursorXY((128 - getTextWidth(maxStr)) / 2, 50);
      oled.print(maxStr);

      return;
    }

    if (!locked && up.pressing() && down.pressing())
      moveDelay = SNAKE_FAST_DELAY_MS;
    else
      moveDelay = SNAKE_MOVE_DELAY_MS;

    if (millis() - lastMove >= moveDelay)
    {
      move();
      lastMove = millis();
    }

    draw();
  }

  void handleInput()
  {
    if (!locked && up.click())
      dir = (dir + 3) % 4;
    if (!locked && down.click())
      dir = (dir + 1) % 4;
  }
};
SnakeGame snake;

/*********************************** FLAPPY BIRD ***************************************/

struct FlappyGame
{
  float birdY = FLAPPY_HEIGHT / 2.0;
  float velocity = 0;
  bool gameOver = false;
  uint32_t lastUpdate = 0;
  uint32_t score = 0;
  bool initialized = false;
  u16_t maxScore;

  Tube tubes[NUM_TUBES];
  uint8_t tubeSpacing = 64;

  void reset()
  {
    birdY = FLAPPY_HEIGHT / 2;
    velocity = 0;
    gameOver = false;
    score = 0;

    for (uint8_t i = 0; i < NUM_TUBES; i++)
    {
      tubes[i].x = FLAPPY_WIDTH + i * tubeSpacing;
      tubes[i].gapY = random(5, FLAPPY_HEIGHT - GAP_HEIGHT - 10);
    }

    lastUpdate = millis();
    maxScore = gameScores.flappyMax;
  }

  void draw()
  {
    oled.rect(20, (int)birdY, 20 + BIRD_SIZE, (int)birdY + BIRD_SIZE, 1);

    for (uint8_t i = 0; i < NUM_TUBES; i++)
    {
      if (tubes[i].x + TUBE_WIDTH <= 0 || tubes[i].x >= FLAPPY_WIDTH)
        continue;

      oled.rect(tubes[i].x, 0, tubes[i].x + TUBE_WIDTH, tubes[i].gapY, 1);
      oled.rect(tubes[i].x, tubes[i].gapY + GAP_HEIGHT, tubes[i].x + TUBE_WIDTH, FLAPPY_HEIGHT, 1);
    }
  }

  void update()
  {
    if (gameOver)
    {
      if (score > gameScores.flappyMax)
      {
        gameScores.flappyMax = score;
        saveAllScores();
      }
      oled.setScale(2);

      const char *msg = "Game Over!";
      char scoreStr[20];
      sprintf(scoreStr, "Score: %lu", score);

      oled.setCursorXY((128 - 2 * getTextWidth(msg)) / 2, 10);
      oled.print(msg);

      oled.setCursorXY((128 - 2 * getTextWidth(scoreStr)) / 2, 30);
      oled.print(scoreStr);

      oled.setScale(1);
      char maxStr[20];
      sprintf(maxStr, "Best score: %d", gameScores.flappyMax);
      oled.setCursorXY((128 - getTextWidth(maxStr)) / 2, 50);
      oled.print(maxStr);

      return;
    }

    velocity += GRAVITY;
    birdY += velocity;
    if (birdY < 0)
      birdY = 0;
    if (birdY + BIRD_SIZE >= FLAPPY_HEIGHT)
      gameOver = true;

    if (!locked && (down.click() || down.pressing()))
    {
      velocity = JUMP_STRENGTH;
    }

    if (millis() - lastUpdate > 40)
    {
      lastUpdate = millis();
      for (uint8_t i = 0; i < NUM_TUBES; i++)
      {
        tubes[i].x--;

        if (tubes[i].x + TUBE_WIDTH < 0)
        {
          tubes[i].x = FLAPPY_WIDTH;
          tubes[i].gapY = random(10, FLAPPY_HEIGHT - GAP_HEIGHT - 10);
          score++;
        }

        if (20 + BIRD_SIZE > tubes[i].x && 20 < tubes[i].x + TUBE_WIDTH)
        {
          if (birdY < tubes[i].gapY || birdY + BIRD_SIZE > tubes[i].gapY + GAP_HEIGHT)
          {
            gameOver = true;
            vibro(255, 50, 3);
          }
        }
      }
    }

    draw();
  }
};
FlappyGame flappy;

//*================================== UHF ======================================*/
void initRadioAttack()
{
  if (radio.begin())
  {
    radio.powerUp();
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.startConstCarrier(RF24_PA_HIGH, START_CHANNEL);
  }
}

void initRadioScanner()
{
  if (radio.begin())
  {
    radio.setAutoAck(false);
    radio.disableCRC();
    radio.setAddressWidth(2);
    for (uint8_t i = 0; i < 6; ++i)
    {
      radio.openReadingPipe(i, noiseAddress[i]);
    }
    radio.setDataRate(RF24_1MBPS);
    radio.startListening();
    radio.stopListening();
    radio.flush_rx();
  }
}

void stopRadioAttack()
{
  radio.stopConstCarrier();
  radio.powerDown();
}

bool scanChannels(uint8_t channel)
{
  radio.setChannel(channel);
  radio.startListening();
  delayMicroseconds(130);
  bool foundSignal = radio.testRPD();
  radio.stopListening();

  if (foundSignal || radio.testRPD() || radio.available())
  {
    radio.flush_rx();
    return true;
  }
  return false;
}

/********************************** BARRIER TRANSMISSION (HF) **************************************/

void SendBit(byte b, int pulse)
{
  if (b == 0)
  {
    digitalWrite(GD0_PIN_CC, HIGH);
    delayMicroseconds(pulse * 2);
    digitalWrite(GD0_PIN_CC, LOW);
    delayMicroseconds(pulse);
  }
  else
  {
    digitalWrite(GD0_PIN_CC, HIGH);
    delayMicroseconds(pulse);
    digitalWrite(GD0_PIN_CC, LOW);
    delayMicroseconds(pulse * 2);
  }
}

// AN-MOTORS
void sendANMotors(uint32_t c1, uint32_t c2)
{
  for (int j = 0; j < 4; j++)
  {
    // отправка 12 начальных импульсов 0-1
    for (int i = 0; i < 12; i++)
    {
      delayMicroseconds(AN_MOTORS_PULSE);
      digitalWrite(GD0_PIN_CC, HIGH);
      delayMicroseconds(AN_MOTORS_PULSE);
      digitalWrite(GD0_PIN_CC, LOW);
    }
    delayMicroseconds(AN_MOTORS_PULSE * 10);
    // отправка первой части кода
    for (int i = 32; i > 0; i--)
    {
      SendBit(bitRead(c1, i - 1), AN_MOTORS_PULSE);
    }
    // отправка второй части кода
    for (int i = 32; i > 0; i--)
    {
      SendBit(bitRead(c2, i - 1), AN_MOTORS_PULSE);
    }
    // отправка бит, которые означают батарею и флаг повтора
    SendBit(1, AN_MOTORS_PULSE);
    SendBit(1, AN_MOTORS_PULSE);
    delayMicroseconds(AN_MOTORS_PULSE * 38);
  }
}

// CAME
void sendCame(uint32_t Code)
{
  int bits = (Code >> 12) ? 24 : 12;
  for (int j = 0; j < 4; j++)
  {
    digitalWrite(GD0_PIN_CC, HIGH);
    delayMicroseconds(320);
    digitalWrite(GD0_PIN_CC, LOW);
    for (int i = bits; i > 0; i--)
    {
      byte b = bitRead(Code, i - 1);
      if (b)
      {
        digitalWrite(GD0_PIN_CC, LOW); // 1
        delayMicroseconds(640);
        digitalWrite(GD0_PIN_CC, HIGH);
        delayMicroseconds(320);
      }
      else
      {
        digitalWrite(GD0_PIN_CC, LOW); // 0
        delayMicroseconds(320);
        digitalWrite(GD0_PIN_CC, HIGH);
        delayMicroseconds(640);
      }
    }
    digitalWrite(GD0_PIN_CC, LOW);
    if (bits == 24)
      delayMicroseconds(23040);
    else
      delayMicroseconds(11520);
  }
}

// NICE
void sendNice(uint32_t Code)
{
  int bits = (Code >> 12) ? 24 : 12;
  for (int j = 0; j < 4; j++)
  {
    digitalWrite(GD0_PIN_CC, HIGH);
    delayMicroseconds(700);
    digitalWrite(GD0_PIN_CC, LOW);
    for (int i = bits; i > 0; i--)
    {
      byte b = bitRead(Code, i - 1);
      if (b)
      {
        digitalWrite(GD0_PIN_CC, LOW); // 1
        delayMicroseconds(1400);
        digitalWrite(GD0_PIN_CC, HIGH);
        delayMicroseconds(700);
      }
      else
      {
        digitalWrite(GD0_PIN_CC, LOW); // 0
        delayMicroseconds(700);
        digitalWrite(GD0_PIN_CC, HIGH);
        delayMicroseconds(1400);
      }
    }
    digitalWrite(GD0_PIN_CC, LOW);
    if (bits == 24)
      delayMicroseconds(50400);
    else
      delayMicroseconds(25200);
  }
}

/********************************** BARRIER SIGNAL RECEIVE (HF) ***************************************/

boolean CheckValue(uint16_t base, uint16_t value)
{
  return ((value == base) || ((value > base) && ((value - base) < MAX_DELTA_T_BARRIER)) || ((value < base) && ((base - value) < MAX_DELTA_T_BARRIER)));
}

void captureBarrierCode()
{
  // barrierCurrentLevel = digitalRead(RA_RX);
  barrierCurrentLevel = digitalRead(GD0_PIN_CC);
  if (barrierCurrentLevel == HIGH)
    lowDurationMicros = micros() - lastEdgeMicros;
  else
    highDurationMicros = micros() - lastEdgeMicros;

  lastEdgeMicros = micros();

  // AN-MOTORS
  if (barrierCurrentLevel == HIGH)
  {
    if (CheckValue(AN_MOTORS_PULSE, highDurationMicros) && CheckValue(AN_MOTORS_PULSE * 2, lowDurationMicros))
    { // valid 1
      if (anMotorsCounter < 32)
        code1 = (code1 << 1) | 1;
      else if (anMotorsCounter < 64)
        code2 = (code2 << 1) | 1;
      anMotorsCounter++;
    }
    else if (CheckValue(AN_MOTORS_PULSE * 2, highDurationMicros) && CheckValue(AN_MOTORS_PULSE, lowDurationMicros))
    { // valid 0
      if (anMotorsCounter < 32)
        code1 = (code1 << 1) | 0;
      else if (anMotorsCounter < 64)
        code2 = (code2 << 1) | 0;
      anMotorsCounter++;
    }
    else
    {
      anMotorsCounter = 0;
      code1 = 0;
      code2 = 0;
    }
    if (anMotorsCounter >= 65 && code2 != -1)
    {
      anMotorsCaptured = true;
      barrierProtocol = 0;
      barrierCodeMain = code1;
      barrierCodeAdd = code2;
      barrierBit = anMotorsCounter;

      code1 = 0;
      code2 = 0;
      anMotorsCounter = 0;
    }
  }

  // CAME
  if (barrierCurrentLevel == LOW)
  {
    if (CheckValue(320, highDurationMicros) && CheckValue(640, lowDurationMicros))
    { // valid 1
      cameCode = (cameCode << 1) | 1;
      cameCounter++;
    }
    else if (CheckValue(640, highDurationMicros) && CheckValue(320, lowDurationMicros))
    { // valid 0
      cameCode = (cameCode << 1) | 0;
      cameCounter++;
    }
    else
    {
      cameCounter = 0;
      cameCode = 0;
    }
  }
  else if ((cameCounter == 12 || cameCounter == 24) && lowDurationMicros > 1000)
  {
    cameCaptured = true;
    barrierProtocol = 2;
    barrierCodeMain = cameCode;
    barrierBit = cameCounter;

    cameCode = 0;
    cameCounter = 0;
  }

  // NICE
  if (barrierCurrentLevel == LOW)
  {
    if (CheckValue(700, highDurationMicros) && CheckValue(1400, lowDurationMicros))
    { // valid 1
      niceCode = (niceCode << 1) | 1;
      niceCounter++;
    }
    else if (CheckValue(1400, highDurationMicros) && CheckValue(700, lowDurationMicros))
    { // valid 0
      niceCode = (niceCode << 1) | 0;
      niceCounter++;
    }
    else
    {
      niceCounter = 0;
      niceCode = 0;
    }
  }
  else if ((niceCounter == 12 || niceCounter == 24) && lowDurationMicros > 2000)
  {
    niceCaptured = true;
    barrierProtocol = 1;
    barrierCodeMain = niceCode;
    barrierBit = niceCounter;

    niceCode = 0;
    niceCounter = 0;
  }
}

/* ============================= 125 kHz RFID EMULATION =============================== */

void set_pin_manchester(int clock_half, int signal)
{
  int man_encoded = clock_half ^ signal;
  if (man_encoded == 1)
  {
    digitalWrite(RFID_COIL_PIN, LOW);
  }
  else
  {
    digitalWrite(RFID_COIL_PIN, HIGH);
  }
}

uint32_t *generate_card(uint64_t Hex_IDCard)
{
  static uint32_t data_card[64];
  static uint32_t card_id[10];

  for (int i = 0; i < 10; i++)
    card_id[i] = (Hex_IDCard >> 36 - i * 4) & 0xF;

  for (int i = 0; i < 9; i++)
    data_card[i] = 1;

  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 4; j++)
      data_card[9 + i * 5 + j] = card_id[i] >> (3 - j) & 1;
    data_card[9 + i * 5 + 4] = (data_card[9 + i * 5 + 0] + data_card[9 + i * 5 + 1] + data_card[9 + i * 5 + 2] + data_card[9 + i * 5 + 3]) % 2;
  }

  for (int i = 0; i < 4; i++)
  {
    int checksum = 0;
    for (int j = 0; j < 10; j++)
      checksum += data_card[9 + i + j * 5];
    data_card[i + 59] = checksum % 2;
  }
  data_card[63] = 0;

  return data_card;
}

void emulateCard(uint32_t *data)
{
  for (int i = 0; i < 64; i++)
  {
    set_pin_manchester(0, data[i]);
    delayMicroseconds(255);

    set_pin_manchester(1, data[i]);
    delayMicroseconds(255);
  }
}

/* =========================================== NFC RFID ================================================= */
void nfcPool()
{
  static uint32_t lastCheckNFC = 0;

  if (millis() - lastCheckNFC >= 500)
  {
    lastCheckNFC = millis();

    uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, tagID_NFC, &tagIDLength_NFC, 25);
    // uint8_t success = nfc.readPassiveTargetID_B(tagID_NFC, &tagIDLength_NFC, 25);

    if (success)
    {
      tagDetected = 2;
      nfcDataValid = false;

      if (tagIDLength_NFC == 4)
      {
        nfcCardType = 1; // Mifare Classic

        uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        success = nfc.mifareclassic_AuthenticateBlock(tagID_NFC, tagIDLength_NFC, 4, 0, keya);

        if (success)
        {
          success = nfc.mifareclassic_ReadDataBlock(4, nfcData);
          if (success)
          {
            nfcDataLength = 16;
            nfcDataValid = true;
          }
        }
      }
      else if (tagIDLength_NFC == 7)
      {
        nfcCardType = 2; // Mifare Ultralight

        success = nfc.mifareultralight_ReadPage(4, nfcData);
        if (success)
        {
          nfcDataLength = 4;
          nfcDataValid = true;
        }
      }

      vibro(255, 30);
    }
  }
}