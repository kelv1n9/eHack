#pragma once

// #define DEBUG_DT

#ifdef DEBUG_DT
#define DBG_DT(...)                \
  do                            \
  {                             \
    Serial.printf(__VA_ARGS__); \
  } while (0)
#else
#define DBG_DT(...) \
  do             \
  {              \
  } while (0)
#endif

#include <Arduino.h>
#include "RF24.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define PROTOCOL_HEADER 0xA5
#define COMMAND_IDLE 0xCC

#define COMMAND_HF_SPECTRUM 0x01
#define COMMAND_HF_ACTIVITY 0x02
#define COMMAND_HF_SCAN 0x03
#define COMMAND_HF_REPLAY 0x04
#define COMMAND_HF_JAMMER 0x05
#define COMMAND_HF_TESLA 0x06

#define COMMAND_UHF_SPECTRUM 0x07
#define COMMAND_UHF_ALL_JAMMER 0x08
#define COMMAND_UHF_WIFI_JAMMER 0x09
#define COMMAND_UHF_BT_JAMMER 0x10
#define COMMAND_UHF_BLE_JAMMER 0x11
#define COMMAND_UHF_USB_JAMMER 0x12
#define COMMAND_UHF_VIDEO_JAMMER 0x13
#define COMMAND_UHF_RC_JAMMER 0x14

#define COMMAND_HF_BARRIER_SCAN 0x15
#define COMMAND_HF_BARRIER_REPLAY 0x16
#define COMMAND_HF_BARRIER_BRUTE_CAME 0x17
#define COMMAND_HF_BARRIER_BRUTE_NICE 0x18

#define COMMAND_FM_RADIO 0x19

#define COMMAND_BATTERY_VOLTAGE 0xFF

enum RadioMode
{
  Master,
  Slave
};

class DataTransmission
{
private:
  RadioMode currentMode;
  RF24 *radioNRF24;

public:
  /**
   * @brief Initializes the radio module based on the current settings.
   * @param None
   * @return None
   */
  void init();

  /**
  @brief Constructor for DataTransmission class.
  @param radioPtrNRF Pointer to RF24 radio object.
  @param radioPtrCC Pointer to ELECHOUSE_CC1101 radio object.
  @note This constructor initializes the DataTransmission object with pointers to the RF24 and ELECHOUSE_CC1101 radio objects.
  */
  DataTransmission(RF24 *radioPtrNRF);

  /**
   * @brief Sets the radio module to slave mode.
   * @param None
   * @return None
   */
  void setSlaveMode();

  /**
   * @brief Sets the radio module to master mode.
   * @param None
   * @return None
   */
  void setMasterMode();

  /**
   * @brief Builds a packet for transmission.
   * @param mode The mode of the packet (e.g., command type).
   * @param payload Pointer to the payload data.
   * @param payloadLen Length of the payload data.
   * @param packetOut Pointer to the output buffer for the packet.
   * @return The length of the built packet.
   */
  uint8_t buildPacket(uint8_t mode, const uint8_t *payload, uint8_t payloadLen, uint8_t *packetOut);

  /**
   * @brief Sends a packet of data.
   * @param data Pointer to the data to be sent.
   * @param len Length of the data to be sent.
   * @return True if the packet was successfully sent, false otherwise.
   * @note This function initializes the radio and sends the provided data packet.
   */
  bool sendPacket(uint8_t *data, uint8_t len);

  /**
   * @brief Receives a packet of data.
   * @param data Pointer to the buffer where received data will be stored.
   * @param len Pointer to a variable that will hold the length of the received data.
   * @return True if a packet was successfully received, false otherwise.
   * @note This function checks for incoming packets and reads them into the provided buffer.
   */
  bool receivePacket(uint8_t *data, uint8_t *len);

  /**
   * @brief Gets the current mode of the radio.
   * @return The current RadioMode (Master or Slave).
   */
  RadioMode getCurrentMode();
};