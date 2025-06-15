#include "DataTransmission.h"

DataTransmission::DataTransmission(RF24 *radioPtrNRF,
                                   ELECHOUSE_CC1101 *radioPtrCC)
{
    radioNRF24 = radioPtrNRF;
    radioCC1101 = radioPtrCC;
}

void DataTransmission::setRadioCC1101()
{
    currentRadio = RADIO_CC1101;
    DBG("Radio set to CC1101\n");
}

void DataTransmission::setRadioNRF24()
{
    currentRadio = RADIO_NRF24;
    DBG("Radio set to NRF24\n");
}

void DataTransmission::setSlaveMode()
{
    currentMode = Slave;
    DBG("Radio set to Slave mode\n");
}

void DataTransmission::setMasterMode()
{
    currentMode = Master;
    DBG("Radio set to Master mode\n");
}

RadioMode DataTransmission::getCurrentMode()
{
    return currentMode;
}

void DataTransmission::init()
{
    if (currentRadio == RADIO_CC1101)
    {
        radioCC1101->Init();
        radioCC1101->setCCMode(1);
        if (currentMode == Master)
        {
            radioCC1101->SetTx(433.92);
        }
        else if (currentMode == Slave)
        {
            radioCC1101->SetRx(433.92);
        }
        radioCC1101->setCrc(1);
        radioCC1101->setDRate(500);

        DBG("CC1101 radio initialized\n");
    }
    else if (currentRadio == RADIO_NRF24)
    {
        radioNRF24->powerUp();
        radioNRF24->stopListening();
        radioNRF24->enableDynamicPayloads();
        radioNRF24->setAutoAck(true);
        radioNRF24->setPayloadSize(32);
        radioNRF24->setDataRate(RF24_2MBPS);
        radioNRF24->setCRCLength(RF24_CRC_16);
        radioNRF24->setChannel(125);
        if (currentMode == Master)
        {
            radioNRF24->openWritingPipe(0xAABBCCDD11LL);
        }
        else if (currentMode == Slave)
        {
            radioNRF24->openReadingPipe(1, 0xAABBCCDD11LL);
            radioNRF24->startListening();
        }
        DBG("NRF24 radio initialized\n");
    }
}

uint8_t DataTransmission::buildPacket(uint8_t mode, const uint8_t *payload, uint8_t payloadLen, uint8_t *packetOut)
{
    uint8_t index = 0;
    packetOut[index++] = PROTOCOL_HEADER; // Header
    packetOut[index++] = mode;            // Mode

    for (uint8_t i = 0; i < payloadLen; i++)
    {
        packetOut[index++] = payload[i];
    }

#ifdef DEBUG
    DBG("Packet built with mode: %d, payload length: %d\n", mode, payloadLen);
    DBG("Packet length: %d\n", index);

    for (uint8_t i = 0; i < index; i++)
    {
        DBG("Packet byte %d: %02X\n", i, packetOut[i]);
    }
#endif

    return index;
}

bool DataTransmission::sendPacket(uint8_t *data, uint8_t len)
{
#ifdef DEBUG
    for (uint8_t i = 0; i < len; i++)
    {
        DBG("Packet to send %d: %02X\n", i, data[i]);
    }
#endif
    if (currentRadio == RADIO_CC1101)
    {
        radioCC1101->SendData(data, len);
        DBG("Packet sent via CC1101\n");
    }
    else if (currentRadio == RADIO_NRF24)
    {
        radioNRF24->write(data, len);
        DBG("Packet sent via NRF24\n");
    }

    return true;
}

bool DataTransmission::receivePacket(uint8_t *data, uint8_t *len)
{
    if (currentRadio == RADIO_CC1101 && radioCC1101->CheckReceiveFlag())
    {
        *len = radioCC1101->ReceiveData(data);
#ifdef DEBUG
        DBG("Packet received via CC1101, length: %d\n", *len);
        for (uint8_t i = 0; i < *len; i++)
        {
            DBG("Received byte %d: %02X\n", i, data[i]);
        }
#endif
        return radioCC1101->CheckCRC();
    }
    else if (currentRadio == RADIO_NRF24 && radioNRF24->available())
    {
        uint8_t size = radioNRF24->getDynamicPayloadSize();

        if (size == 0)
        {
            radioNRF24->flush_rx();
            return false;
        }

        *len = size;
        radioNRF24->read(data, *len);
#ifdef DEBUG
        DBG("Packet received via NRF24, length: %d\n", *len);
        for (uint8_t i = 0; i < *len; i++)
        {
            DBG("Received byte %d: %02X\n", i, data[i]);
        }
#endif
        return true;
    }
    return false;
}

bool DataTransmission::checkConnection(uint16_t timeoutMs)
{
    static uint32_t startT = 0;
    static bool waiting = false;

    uint8_t buf[32];
    uint8_t len = 0;
    uint32_t now = millis();

    if (!waiting)
    {
        startT = now;

        if (currentMode == Master)
        {
            byte ping[4] = {80, 73, 78, 71};
            sendPacket(ping, 4);
        }
        waiting = true;
    }

    if (receivePacket(buf, &len) && len == 4)
    {
        if (currentMode == Master &&
            buf[0] == 80 && buf[1] == 79 && buf[2] == 78 && buf[3] == 71) // «PONG»
        {
            waiting = false;
            return true;
        }

        if (currentMode == Slave &&
            buf[0] == 80 && buf[1] == 73 && buf[2] == 78 && buf[3] == 71) // «PING»
        {
            byte pong[4] = {80, 79, 78, 71};
            sendPacket(pong, 4);
            waiting = false;
            return true;
        }
    }

    if (now - startT >= timeoutMs)
    {
        waiting = false;
    }
    return false;
}