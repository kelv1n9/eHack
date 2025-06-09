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

class DataTransmission
{
private:
    RadioType currentRadio;
    RF24 *radioNRF24;
    ELECHOUSE_CC1101 *radioCC1101;

public:
    DataTransmission(RF24 *radioPtrNRF, ELECHOUSE_CC1101 *radioPtrCC);

    void setRadioCC1101();
    void setRadioNRF24();

    uint8_t buildCommandPacket(uint8_t type, uint8_t mode, uint8_t action, const uint8_t *payload, uint8_t payloadLen, uint8_t *packetOut);
    void sendPacket(uint8_t *data, uint8_t len);
};