#include "DataTransmission.h"

DataTransmission::DataTransmission(RF24 *radioPtrNRF,
                                   ELECHOUSE_CC1101 *radioPtrCC)
{
    radioNRF24 = radioPtrNRF;
    radioCC1101 = radioPtrCC;
    DBG("DataTransmission initialized");
}

void DataTransmission::setRadioCC1101()
{
    currentRadio = RADIO_CC1101;
    DBG("Radio set to CC1101");
}

void DataTransmission::setRadioNRF24()
{
    currentRadio = RADIO_NRF24;
    DBG("Radio set to NRF24");
}

void DataTransmission::setSlaveMode()
{
    currentMode = Slave;
    DBG("Radio set to Slave mode");
}

void DataTransmission::setMasterMode()
{
    currentMode = Master;
    DBG("Radio set to Master mode");
}

void DataTransmission::_initRadio()
{
    if (currentRadio == RADIO_CC1101)
    {
        radioCC1101->SetTx(450);
        radioCC1101->setModulation(0);
        radioCC1101->setDeviation(150);
        radioCC1101->setDRate(500);
        radioCC1101->setLengthConfig(1);
        radioCC1101->setCrc(1);
        radioCC1101->setDcFilterOff(0);
        radioCC1101->setManchester(0);
        radioCC1101->setSyncWord(211, 145);
        radioCC1101->setWhiteData(0);
        DBG("CC1101 radio initialized");
    }
    else if (currentRadio == RADIO_NRF24)
    {
        if (currentMode == Master)
        {
            radioNRF24->powerUp();
            radioNRF24->stopListening();
            radioNRF24->setDataRate(RF24_2MBPS);
            radioNRF24->setPayloadSize(32);
            radioNRF24->setCRCLength(RF24_CRC_16);
            radioNRF24->openWritingPipe(0x000000000001);
            radioNRF24->setChannel(125);
            DBG("NRF24 radio initialized in Master mode");
        }
        else if (currentMode == Slave)
        {
            radioNRF24->powerUp();
            radioNRF24->stopListening();
            radioNRF24->setDataRate(RF24_2MBPS);
            radioNRF24->setPayloadSize(32);
            radioNRF24->setCRCLength(RF24_CRC_16);
            radioNRF24->openReadingPipe(0, 0x000000000001);
            radioNRF24->setChannel(125);
            radioNRF24->startListening();
            DBG("NRF24 radio initialized in Slave mode");
        }
    }
}

uint8_t DataTransmission::buildPacket(uint8_t type, uint8_t mode,
                                      uint8_t action, const uint8_t *payload,
                                      uint8_t payloadLen, uint8_t *packetOut)
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

#ifdef DEBUG
    DBG("Packet built with type: %d, mode: %d, action: %d, payload length: %d",
        type, mode, action, payloadLen);
    DBG("Packet length: %d", index);

    for (uint8_t i = 0; i < index; i++)
    {
        DBG("Packet byte %d: %02X", i, packetOut[i]);
    }
#endif

    return index;
}

bool DataTransmission::sendPacket(uint8_t *data, uint8_t len)
{
    _initRadio();
#ifdef DEBUG
    for (uint8_t i = 0; i < len; i++)
    {
        DBG("Packet to send %d: %02X", i, data[i]);
    }
#endif
    if (currentRadio == RADIO_CC1101)
    {
        radioCC1101->SendData(data, len);
        DBG("Packet sent via CC1101");

    }
    else if (currentRadio == RADIO_NRF24)
    {
        radioNRF24->write(data, len);
        DBG("Packet sent via NRF24");
    }

    return true;
}

bool DataTransmission::receivePacket(uint8_t *data, uint8_t *len)
{
    _initRadio();
    if (currentRadio == RADIO_CC1101 && radioCC1101->CheckReceiveFlag())
    {
        *len = radioCC1101->ReceiveData(data);
#ifdef DEBUG
        DBG("Packet received via CC1101, length: %d", *len);
        for (uint8_t i = 0; i < *len; i++)
        {
            DBG("Received byte %d: %02X", i, data[i]);
        }
#endif
        return radioCC1101->CheckCRC();
    }
    else if (currentRadio == RADIO_NRF24 && radioNRF24->available())
    {
        radioNRF24->read(data, 32);
#ifdef DEBUG
        DBG("Packet received via NRF24, length: %d", *len);
        for (uint8_t i = 0; i < 32; i++)
        {
            DBG("Received byte %d: %02X", i, data[i]);
        } 
#endif
        *len = 32;
        return true;
    }
    return false;
}

bool DataTransmission::checkConnection(uint16_t timeoutMs)
{
    uint8_t buf[32];
    uint8_t len = 0;
    uint32_t t0 = millis();

    if (currentMode == Master)
    {
        static const uint8_t ping[4] = {'P', 'I', 'N', 'G'};

        sendPacket((uint8_t *)ping, 4);
        while (millis() - t0 < timeoutMs)
        {
            DBG("Waiting for PING response...");
            if (receivePacket(buf, &len) && len == 4 && buf[0] == 'P' &&
                buf[1] == 'O' && buf[2] == 'N' && buf[3] == 'G')
            {
                DBG("Received PONG response");
                return true;
            }
        }
        return false;
    }

    if (currentMode == Slave)
    {
        if (receivePacket(buf, &len) && len == 4 && buf[0] == 'P' &&
            buf[1] == 'I' && buf[2] == 'N' && buf[3] == 'G')
        {   
            DBG("Received PING request, sending PONG response");
            static const uint8_t pong[4] = {'P', 'O', 'N', 'G'};
            sendPacket((uint8_t *)pong, 4);
            return true;
        }
    }
    return false;
}
