#pragma once

#include <Arduino.h>
#include "RF24.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define PROTOCOL_HEADER 0xA5

#define COMMAND_HF_SPECTRUM 0x01
#define COMMAND_HF_ACTIVITY 0x02
#define COMMAND_HF_BARRIER_SCAN 0x03
#define COMMAND_HF_BARRIER_REPLAY 0x04
#define COMMAND_HF_BARRIER_BRUTE_CAME 0x05
#define COMMAND_HF_BARRIER_BRUTE_NICE 0x06
#define COMMAND_HF_SCAN 0x07
#define COMMAND_HF_REPLAY 0x08
#define COMMAND_HF_NOISE 0x09
#define COMMAND_HF_TESLA 0x10

#define COMMAND_UHF_SPECTRUM 0x01
#define COMMAND_UHF_ALL_JAMMER 0x02
#define COMMAND_UHF_WIFI_JAMMER 0x03
#define COMMAND_UHF_BT_JAMMER 0x04
#define COMMAND_UHF_BLE_JAMMER 0x05

#define COMMAND_DISABLE 0x00
#define COMMAND_ENABLE 0x01

/*
Protocol:
Header - 0xA5
Type - HF/UHF - 0x01/0x02
Mode -> HF - 0x1 -> 0x09
     -> UHF - 0x1 -> 0x05
Action - Enable/Disable - 0x01/0x02
Payload - []
CRC - CRC
*/

enum RadioType
{
    RADIO_CC1101,
    RADIO_NRF24
};

enum RadioMode
{
    Master,
    Slave
};

class DataTransmission
{
private:
    RadioType currentRadio;
    RadioMode currentMode;
    RF24 *radioNRF24;
    ELECHOUSE_CC1101 *radioCC1101;
    /**
     * @brief Initializes the radio module based on the current settings.
     * @param None
     * @return None
     */
    void _initRadio();
    
public:
    /**
    @brief Constructor for DataTransmission class.
    @param radioPtrNRF Pointer to RF24 radio object.
    @param radioPtrCC Pointer to ELECHOUSE_CC1101 radio object.
    @note This constructor initializes the DataTransmission object with pointers to the RF24 and ELECHOUSE_CC1101 radio objects.
    */
    DataTransmission(RF24 *radioPtrNRF, ELECHOUSE_CC1101 *radioPtrCC);

    /**
     * @brief Initializes the CC1101 radio module for communication.
     * @param None
     * @return None
     * @note This function is called internally to ensure the radio is properly configured before sending or receiving packets.
     * 
     */
    void setRadioCC1101();

    /**
     * @brief Initializes the NRF24 radio module for communication.
     * @param None
     * @return None
     */
    void setRadioNRF24();
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
     * @param type The type of the packet (HF/UHF).
     * @param mode The mode of the packet (e.g., HF/UHF modes).
     * @param action The action to be performed (enable/disable).
     * @param payload Pointer to the payload data.
     * @param payloadLen Length of the payload data.
     * @param packetOut Pointer to the output buffer for the packet.
     * @return The length of the built packet.
     */
    uint8_t buildPacket(uint8_t type, uint8_t mode, uint8_t action, const uint8_t *payload, uint8_t payloadLen, uint8_t *packetOut);
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
     * @brief Checks the connection status of the radio module.
     * @param timeoutMs Timeout in milliseconds for the connection check.
     * @return True if the connection is established, false otherwise.
     * @note This function attempts to verify if the radio module is connected and operational within the specified timeout.
     */
    bool checkConnection(uint16_t timeoutMs);
};