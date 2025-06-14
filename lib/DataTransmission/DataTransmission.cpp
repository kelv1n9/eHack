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

RadioMode DataTransmission::getCurrentMode()
{
    return currentMode;
}

void DataTransmission::init()
{
    if (currentRadio == RADIO_CC1101)
    {
        if (currentMode == Master)
        {
            radioCC1101->SetTx(450);
        }
        else if (currentMode == Slave)
        {
            radioCC1101->SetRx(450);
        }
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
        radioNRF24->powerUp();
        radioNRF24->stopListening();
        radioNRF24->enableDynamicPayloads();
        radioNRF24->setPayloadSize(32);
        radioNRF24->setDataRate(RF24_2MBPS);
        radioNRF24->setCRCLength(RF24_CRC_16);
        radioNRF24->setChannel(125);
        if (currentMode == Master)
        {
            radioNRF24->openWritingPipe(0x000000000001);
        }
        else if (currentMode == Slave)
        {
            radioNRF24->openReadingPipe(0, 0x000000000001);
            radioNRF24->startListening();
        }
        DBG("NRF24 radio initialized");
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
    DBG("Packet built with mode: %d, payload length: %d", mode, payloadLen);
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
        *len = radioNRF24->getDynamicPayloadSize();
        radioNRF24->read(data, *len);
#ifdef DEBUG
        DBG("Packet received via NRF24, length: %d", *len);
        for (uint8_t i = 0; i < *len; i++)
        {
            DBG("Received byte %d: %02X", i, data[i]);
        }
#endif
        return true;
    }
    return false;
}

bool DataTransmission::checkConnection(uint16_t timeoutMs)
{
    uint8_t buf[32];
    uint8_t len = 0;
    uint32_t t0 = millis();

    bool ledState = false;
    uint32_t blinkT0 = millis();

    if (currentMode == Master)
    {
        static const uint8_t ping[4] = {'P', 'I', 'N', 'G'};

        sendPacket((uint8_t *)ping, 4);
        while (millis() - t0 < timeoutMs)
        {
            if (millis() - blinkT0 >= 250)
            {
                blinkT0 = millis();
                ledState = !ledState;
                digitalWrite(LED_BUILTIN, ledState);
            }

            DBG("Waiting for PING response...");
            if (receivePacket(buf, &len) && len == 4 && buf[0] == 'P' &&
                buf[1] == 'O' && buf[2] == 'N' && buf[3] == 'G')
            {
                DBG("Received PONG response");
                digitalWrite(LED_BUILTIN, LOW);
                return true;
            }
        }
        digitalWrite(LED_BUILTIN, LOW);
        return false;
    }
    if (currentMode == Slave)
    {
        while (millis() - t0 < timeoutMs)
        {
            if (millis() - blinkT0 >= 250)
            {
                blinkT0 = millis();
                ledState = !ledState;
                digitalWrite(LED_BUILTIN, ledState);
            }

            if (receivePacket(buf, &len) && len == 4 &&
                buf[0] == 'P' && buf[1] == 'I' && buf[2] == 'N' && buf[3] == 'G')
            {
                DBG("Received PING request, sending PONG response");
                static const uint8_t pong[4] = {'P', 'O', 'N', 'G'};
                sendPacket((uint8_t *)pong, 4);
                digitalWrite(LED_BUILTIN, LOW); 
                return true;
            }
        }
    }
    digitalWrite(LED_BUILTIN, LOW);
    return false;
}
