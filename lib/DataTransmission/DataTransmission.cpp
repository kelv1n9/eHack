#include "DataTransmission.h"

DataTransmission::DataTransmission(RF24 *radioPtrNRF, ELECHOUSE_CC1101 *radioPtrCC)
{
    radioNRF24 = radioPtrNRF;
    radioCC1101 = radioPtrCC;
}

void DataTransmission::setRadioCC1101()
{
    currentRadio = RADIO_CC1101;
}

void DataTransmission::setRadioNRF24()
{
    currentRadio = RADIO_NRF24;
}

uint8_t DataTransmission::buildCommandPacket(uint8_t type, uint8_t mode, uint8_t action, const uint8_t *payload, uint8_t payloadLen, uint8_t *packetOut)
{
    uint8_t index = 0;
    packetOut[index++] = PROTOCOL_HEADER; // Header
    packetOut[index++] = type;            // Type (HF/UHF)
    packetOut[index++] = mode;            // Mode
    packetOut[index++] = action;          // Action

    for (uint8_t i = 0; i < payloadLen; i++)
    {
        packetOut[index++] = payload[i];
    }

    return index;
}

void DataTransmission::sendPacket(uint8_t *data, uint8_t len)
{
    if (currentRadio == RADIO_CC1101)
    {
        radioCC1101->SetTx(433.92);
        radioCC1101->setModulation(0);
        radioCC1101->setDeviation(150);
        radioCC1101->setDRate(600);
        radioCC1101->setLengthConfig(1);
        radioCC1101->setCrc(1);
        radioCC1101->setDcFilterOff(0);
        radioCC1101->setManchester(0);
        radioCC1101->setSyncWord(211, 145);
        radioCC1101->SendData(data, len);
        radioCC1101->goSleep();
    }
    else if (currentRadio == RADIO_NRF24)
    {
        radioNRF24->powerUp();
        radioNRF24->stopListening();
        radioNRF24->setDataRate(RF24_2MBPS);
        radioNRF24->setPayloadSize(32);
        radioNRF24->setCRCLength(RF24_CRC_16);
        radioNRF24->openWritingPipe(0x000000000001);
        radioNRF24->setChannel(125);
        radioNRF24->write(data, len);
        radioNRF24->powerDown();
    }
}