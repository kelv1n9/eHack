#include "DataTransmission.h"

const uint64_t pipe_master_to_slave = 0xAABBCCDD11LL;
const uint64_t pipe_slave_to_master = 0x11223344EELL;

DataTransmission::DataTransmission(RF24 *radioPtrNRF)
{
    radioNRF24 = radioPtrNRF;
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
    radioNRF24->powerUp();
    // radioNRF24->enableDynamicPayloads();
    radioNRF24->setPayloadSize(32);
    radioNRF24->setAutoAck(true);
    radioNRF24->setDataRate(RF24_2MBPS);
    radioNRF24->setCRCLength(RF24_CRC_16);
    radioNRF24->setChannel(40);
    radioNRF24->setPALevel(RF24_PA_MAX);
    radioNRF24->setRetries(0, 15);
    if (currentMode == Master)
    {
        radioNRF24->openWritingPipe(pipe_master_to_slave);
        radioNRF24->openReadingPipe(1, pipe_slave_to_master);
    }
    else if (currentMode == Slave)
    {
        radioNRF24->openReadingPipe(1, pipe_master_to_slave);
        radioNRF24->openWritingPipe(pipe_slave_to_master);
    }
    radioNRF24->startListening();
    DBG("NRF24 radio initialized\n");
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

    radioNRF24->stopListening();
    bool sent = radioNRF24->write(data, len);
    radioNRF24->startListening();
    DBG("Packet sent via NRF24, success: %d\n", sent);
    return sent;
}

bool DataTransmission::receivePacket(uint8_t *data, uint8_t *len)
{

    if (radioNRF24->available())
    {
        uint8_t size = radioNRF24->getPayloadSize();

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
    else
    {
        DBG("NRF24 is not available\n");
        return false;
    }
}