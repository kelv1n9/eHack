#include "visualization.h"

void handleBatteryTasks();

void setup()
{
  // I2C
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.setClock(400000);
  Wire.begin();

  // SPI
  SPI.setSCK(6);
  SPI.setMOSI(7);
  SPI.setMISO(4);
  SPI.begin();

  cc1101Init();
  radio.begin();
  radio.powerDown();

  oled.init();
  oled.clear();
  resetBrightness();
  oled.update();
  ShowSplashScreen();

  nfc.begin();
  nfc.setPassiveActivationRetries(0xFF);
  nfc.powerDownMode();

  analogReadResolution(12);
  batVoltage = readBatteryVoltage();
  EEPROM.begin(512);

  IrReceiver.begin(IR_RX, DISABLE_LED_FEEDBACK);
  IrSender.begin(IR_TX, DISABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
  IrReceiver.disableIRIn();

  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);

  pinMode(RFID_COIL_PIN, OUTPUT);
  digitalWrite(RFID_COIL_PIN, LOW);

  pinMode(BLE_PIN, OUTPUT);
  digitalWrite(BLE_PIN, LOW);

  pinMode(VIBRO, OUTPUT);

  findLastUsedSlotRA();
  findLastUsedSlotIR();
  findLastUsedSlotRFID();

  loadSettings();
  loadAllScores();

  for (uint8_t i = 0; i < RSSI_BUFFER_SIZE; i++)
  {
    rssiBuffer[i] = -100;
  }

  vibro(255, 30);

#ifdef DEBUG_eHack
  Serial.begin(9600);
#endif
}

void setup1()
{
}

void loop1()
{
  switch (currentMenu)
  {
  /*============================= 433 MHz Protocol =================================*/
  /********************************** SCANNING **************************************/
  case HF_SCAN:
  {
    if (!initialized)
    {
      pinMode(GD0_PIN_CC, INPUT);
      mySwitch.enableReceive(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_SCAN, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (checkFreqButtons())
    {
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_SCAN, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }
    }

    if (mySwitch.available())
    {
      // Check if the signal is valid
      if (mySwitch.getReceivedBitlength() < 10)
      {
        mySwitch.resetAvailable();
        break;
      }

      signalCaptured_433MHZ = true;

      currentRssi = ELECHOUSE_cc1101.getRssi();

      capturedCode = mySwitch.getReceivedValue();
      capturedLength = mySwitch.getReceivedBitlength();
      capturedProtocol = mySwitch.getReceivedProtocol();
      capturedDelay = mySwitch.getReceivedDelay();

      SimpleRAData data;
      data.code = capturedCode;
      data.length = capturedLength;
      data.protocol = capturedProtocol;
      data.delay = capturedDelay;

      vibro(255, 200, 3, 80);

      // Check for duplicates
      if (isDuplicateRA(data))
      {
        mySwitch.resetAvailable();
        break;
      }

      if (settings.saveRA)
      {
        writeRAData(lastUsedSlotRA, data);
      }

      lastUsedSlotRA = (lastUsedSlotRA + 1) % MAX_RA_SIGNALS;
      mySwitch.resetAvailable();
    }
    break;
  }
  /********************************** ATTACK **************************************/
  case HF_REPLAY:
  {
    static uint32_t attackTimer = 0;

    if (!initialized)
    {
      pinMode(GD0_PIN_CC, OUTPUT);
      mySwitch.enableTransmit(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_REPLAY, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (!locked && !attackIsActive && (up.click() || up.step()))
    {
      if (selectedSlotRA == 0)
        selectedSlotRA = MAX_RA_SIGNALS - 1;
      else
        selectedSlotRA--;
      vibro(255, 20);
    }

    if (!locked && !attackIsActive && (down.click() || down.step()))
    {
      selectedSlotRA = (selectedSlotRA + 1) % MAX_RA_SIGNALS;
      vibro(255, 20);
    }

    SimpleRAData data = readRAData(selectedSlotRA);
    capturedCode = data.code;
    capturedLength = data.length;
    capturedProtocol = data.protocol;
    capturedDelay = data.delay;

    mySwitch.setProtocol(capturedProtocol);

    if (attackIsActive)
    {
      if (checkFreqButtons())
      {
        ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);
        if (successfullyConnected)
        {
          outgoingDataLen = communication.buildPacket(COMMAND_HF_REPLAY, &currentFreqIndex, 1, outgoingData);
          communication.sendPacket(outgoingData, outgoingDataLen);
        }
      }

      if (millis() - attackTimer >= 1000 && !successfullyConnected)
      {
        mySwitch.send(capturedCode, capturedLength);
        attackTimer = millis();
      }
      else if (successfullyConnected && !commandSent)
      {
        mySwitch.send(capturedCode, capturedLength);
        commandSent = true;
      }
    }

    break;
  }
  /********************************** NOISE **************************************/
  case HF_JAMMER:
  {
    static uint32_t lastNoise = 0;
    static bool noiseState = false;

    if (!initialized)
    {
      pinMode(GD0_PIN_CC, OUTPUT);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_JAMMER, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (checkFreqButtons())
    {
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_JAMMER, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }
    }

    if (micros() - lastNoise > 500)
    {
      noiseState = !noiseState;
      digitalWrite(GD0_PIN_CC, noiseState);
      lastNoise = micros();
    }
    break;
  }
  /********************************** TESLA **************************************/
  case HF_TESLA:
  {
    if (!initialized)
    {
      pinMode(GD0_PIN_CC, OUTPUT);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_TESLA, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (!successfullyConnected)
    {
      static bool toggleFreq = false;
      float freq = toggleFreq ? 315.0 : 433.92;
      ELECHOUSE_cc1101.SetTx(freq);
      toggleFreq = !toggleFreq;

      sendTeslaSignal_v1();
      delay(50);
      sendTeslaSignal_v2();
      delay(50);
    }

    break;
  }
  /********************************** RF SPECTRUM **************************************/
  case HF_SPECTRUM:
  {
    static uint32_t spectrumTimer = 0;
    static bool waitingForSettle = false;

    if (!initialized)
    {
      if (!successfullyConnected)
      {
        pinMode(GD0_PIN_CC, INPUT);
        ELECHOUSE_cc1101.SetRx(raFrequencies[currentScanFreq]);
        currentScanFreq = 0;
        spectrumTimer = millis();
        waitingForSettle = true;
      }
      else
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_SPECTRUM, &currentScanFreq, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (!successfullyConnected && waitingForSettle)
    {
      if (millis() - spectrumTimer >= RSSI_STEP_MS)
      {
        currentRssi = ELECHOUSE_cc1101.getRssi();

        if (currentRssi > rssiMaxPeak[currentScanFreq])
        {
          rssiMaxPeak[currentScanFreq] = currentRssi;
        }
        else
        {
          if (rssiMaxPeak[currentScanFreq] > -100)
          {
            rssiMaxPeak[currentScanFreq] -= 3;
            if (rssiMaxPeak[currentScanFreq] < -100)
              rssiMaxPeak[currentScanFreq] = -100;
          }
        }

        if (currentRssi > rssiAbsoluteMax[currentScanFreq])
        {
          rssiAbsoluteMax[currentScanFreq] = currentRssi;
        }

        currentScanFreq = (currentScanFreq + 1) % raFreqCount;
        ELECHOUSE_cc1101.SetRx(raFrequencies[currentScanFreq]);
        spectrumTimer = millis();
        waitingForSettle = true;
      }
    }
    else if (successfullyConnected && radio.available())
    {
      radio.read(&recievedHFData, sizeof(recievedHFData));
      currentRssi = recievedHFData[0];
      currentScanFreq = recievedHFData[1];
      DBG("RSSI: %d, FREQ: %d\n", currentRssi, currentScanFreq);

      if (currentRssi > 0)
      {
        currentRssi = -100;
      }
      if (currentScanFreq > 3)
      {
        currentScanFreq = 3;
      }

      if (currentRssi > rssiMaxPeak[currentScanFreq])
      {
        rssiMaxPeak[currentScanFreq] = currentRssi;
      }
      else
      {
        if (rssiMaxPeak[currentScanFreq] > -100)
        {
          rssiMaxPeak[currentScanFreq] -= 3;
          if (rssiMaxPeak[currentScanFreq] < -100)
            rssiMaxPeak[currentScanFreq] = -100;
        }
      }

      if (currentRssi > rssiAbsoluteMax[currentScanFreq])
      {
        rssiAbsoluteMax[currentScanFreq] = currentRssi;
      }
    }

    break;
  }
  /********************************** RF ACTIVITY **************************************/
  case HF_ACTIVITY:
  {
    static uint32_t lastStepMs = millis();

    if (!initialized)
    {
      if (!successfullyConnected)
      {
        pinMode(GD0_PIN_CC, INPUT);
        ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
      }
      else
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_ACTIVITY, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (checkFreqButtons())
    {
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_ACTIVITY, &currentFreqIndex, 1, outgoingData);

        unsigned long spamDuration = SEND_DURATION_MS + LISTEN_DURATION_MS + 50;
        unsigned long spamStartTime = millis();
        while (millis() - spamStartTime < spamDuration)
        {
          communication.sendPacket(outgoingData, outgoingDataLen);
          delay(5);
        }
      }
    }

    if (!successfullyConnected && millis() - lastStepMs >= RSSI_STEP_MS)
    {
      currentRssi = ELECHOUSE_cc1101.getRssi();
      rssiBuffer[rssiIndex++] = currentRssi;
      if (rssiIndex >= RSSI_BUFFER_SIZE)
        rssiIndex = 0;

      lastStepMs = millis();
    }
    else if (successfullyConnected && radio.available())
    {
      radio.read(&currentRssi, sizeof(currentRssi));

      if (currentRssi > 0)
      {
        currentRssi = -100;
      }

      rssiBuffer[rssiIndex++] = currentRssi;
      if (rssiIndex >= RSSI_BUFFER_SIZE)
        rssiIndex = 0;

      lastStepMs = millis();
    }
    break;
  }
  /*============================= InfraRed Protocol =================================*/
  /********************************** SCANNING **************************************/
  case IR_SCAN:
  {
    if (!initialized)
    {
      protocol, address, command = 0;
      IrReceiver.enableIRIn();
      initialized = true;
    }

    if (IrReceiver.decode())
    {
      auto &data = IrReceiver.decodedIRData;

      if (data.rawDataPtr->rawlen < 4 || data.flags & IRDATA_FLAGS_IS_REPEAT)
      {
        IrReceiver.resume();
        break;
      }

      signalCaptured_IR = true;

      protocol = data.protocol;
      address = data.address;
      command = data.command;

      SimpleIRData ir;
      ir.protocol = protocol;
      ir.address = address;
      ir.command = command;

      // Check for duplicates
      if (isDuplicateIR(ir))
      {
        IrReceiver.resume();
        break;
      }

      if (settings.saveIR)
      {
        writeIRData(lastUsedSlotIR, ir);
      }

      lastUsedSlotIR = (lastUsedSlotIR + 1) % MAX_IR_SIGNALS;

      IrReceiver.resume();
      vibro(255, 200, 3, 80);
    }
    break;
  }
  /********************************** SENDER **************************************/
  case IR_REPLAY:
  {
    if (!initialized)
    {
      IrReceiver.disableIRIn();
      initialized = true;
    }

    SimpleIRData data = readIRData(selectedSlotIR);
    protocol = data.protocol;
    address = data.address;
    command = data.command;

    if (!locked && (up.click() || up.step()))
    {
      if (selectedSlotIR == 0)
        selectedSlotIR = MAX_IR_SIGNALS - 1;
      else
        selectedSlotIR--;
      vibro(255, 20);
    }

    if (!locked && (down.click() || down.step()))
    {
      selectedSlotIR = (selectedSlotIR + 1) % MAX_IR_SIGNALS;
      vibro(255, 20);
    }

    if (!locked && ok.click())
    {
      if (protocol == 0 && address == 0 && command == 0)
        break;
      IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
      delay(DELAY_AFTER_SEND);
      vibro(255, 50);
    }

    break;
  }
  /********************************** BRUTE FORCE TV **************************************/
  case IR_BRUTE_TV:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      IrReceiver.disableIRIn();
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_RUNNING;
        currentIndex = 0;
        lastSendTime = millis();
        vibro(255, 50);
      }
      break;
    }

    case BRUTE_RUNNING:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_PAUSED;
        vibro(255, 50);
        break;
      }

      if (millis() - lastSendTime > 50 && currentIndex < IR_COMMAND_COUNT_TV)
      {
        getIRCommand(irCommandsTV, currentIndex, protocol, address, command);
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        lastSendTime = millis();
        currentIndex++;
      }

      if (currentIndex >= IR_COMMAND_COUNT_TV)
      {
        bruteState = BRUTE_IDLE;
        vibro(255, 50);
      }
      break;
    }
    case BRUTE_PAUSED:
    {
      if (!locked && (up.click() || up.step()))
      {
        if (currentIndex == 0)
          currentIndex = IR_COMMAND_COUNT_TV - 1;
        else
          currentIndex--;

        getIRCommand(irCommandsTV, currentIndex, protocol, address, command);
        vibro(255, 20);
      }

      if (!locked && (down.click() || down.step()))
      {
        currentIndex = (currentIndex + 1) % IR_COMMAND_COUNT_TV;

        getIRCommand(irCommandsTV, currentIndex, protocol, address, command);
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        vibro(255, 50);
      }
      break;
    }
    }
    break;
  }
  /********************************** BRUTE FORCE PROJECTOR **************************************/
  case IR_BRUTE_PROJECTOR:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      IrReceiver.disableIRIn();
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_RUNNING;
        currentIndex = 0;
        lastSendTime = millis();
        vibro(255, 50);
      }
      break;
    }

    case BRUTE_RUNNING:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_PAUSED;
        vibro(255, 50);
        break;
      }

      if (millis() - lastSendTime > 20 && currentIndex < IR_COMMAND_COUNT_PR)
      {
        getIRCommand(irCommandsPR, currentIndex, protocol, address, command);
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        lastSendTime = millis();
        currentIndex++;
      }

      if (currentIndex >= IR_COMMAND_COUNT_PR)
      {
        bruteState = BRUTE_IDLE;
        vibro(255, 50);
      }
      break;
    }
    case BRUTE_PAUSED:
    {
      if (!locked && (up.click() || up.step()))
      {
        if (currentIndex == 0)
          currentIndex = IR_COMMAND_COUNT_PR - 1;
        else
          currentIndex--;

        getIRCommand(irCommandsPR, currentIndex, protocol, address, command);
        vibro(255, 20);
      }

      if (!locked && (down.click() || down.step()))
      {
        currentIndex = (currentIndex + 1) % IR_COMMAND_COUNT_PR;

        getIRCommand(irCommandsPR, currentIndex, protocol, address, command);
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        vibro(255, 50);
      }
      break;
    }
    }
    break;
  }
  /******************************** ENABLE 2.4 GHz **************************************/
  case UHF_ALL_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_ALL_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(full_channels) / sizeof(full_channels[0]));
    radioChannel = full_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_WIFI_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_WIFI_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(wifi_channels) / sizeof(wifi_channels[0]));
    radioChannel = wifi_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_BT_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_BT_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
    radioChannel = bluetooth_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_BLE_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_BLE_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
    radioChannel = ble_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_USB_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_USB_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(usb_channels) / sizeof(usb_channels[0]));
    radioChannel = usb_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_VIDEO_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_VIDEO_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(video_channels) / sizeof(video_channels[0]));
    radioChannel = video_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_RC_JAMMER:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_UHF_RC_JAMMER, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(rc_channels) / sizeof(rc_channels[0]));
    radioChannel = rc_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_SPECTRUM:
  {
    static uint8_t channel = 0;
    if (!initialized)
    {
      initRadioScanner();
      initialized = true;
    }

    bool foundSignal = scanChannels(channel);
    uint8_t strength = stored[channel].push(foundSignal);
    channelStrength[channel] = strength;

    channel = (channel + 1) % NUM_CHANNELS;
    break;
  }
  /*************************** ENABLE BLE SPAM *********************************/
  case UHF_BLE_SPAM:
  {
    if (!initialized)
    {
      initialized = true;
      digitalWrite(BLE_PIN, HIGH);
    }
  }
  /******************************** RFID *************************************/
  case RFID_SCAN:
  {
    if (!initialized)
    {
      rdm6300.begin(RFID_RX_PIN);
      nfc.SAMConfig();
      initialized = true;
    }

    static uint32_t lastCheck125kHz = 0;

    /********************* 125 kHz *******************/
    if (millis() - lastCheck125kHz >= 10)
    {
      lastCheck125kHz = millis();

      if (rdm6300.get_new_tag_id())
      {
        tagID_125kHz = rdm6300.get_tag_id();
        tagDetected = 1;
        vibro(255, 30);

        RFID data;
        data.tagID = tagID_125kHz;

        // Check for duplicates
        if (isDuplicateRFID(data))
        {
          break;
        }

        writeRFIDData(lastUsedSlotRFID, data);
        lastUsedSlotRFID = (lastUsedSlotRFID + 1) % MAX_RFID;
      }
    }

    break;
  }
  case RFID_EMULATE:
  {
    static uint32_t *card_generated;

    if (!locked && !attackIsActive && (up.click() || up.step()))
    {
      if (selectedSlotRFID == 0)
        selectedSlotRFID = MAX_RFID - 1;
      else
        selectedSlotRFID--;
      vibro(255, 20);
    }

    if (!locked && !attackIsActive && (down.click() || down.step()))
    {
      selectedSlotRFID = (selectedSlotRFID + 1) % MAX_RFID;
      vibro(255, 20);
    }

    RFID data = readRFIDData(selectedSlotRFID);
    tagID_125kHz = data.tagID;

    card_generated = generate_card(tagID_125kHz);
    emulateCard(card_generated);

    break;
  }
  case RFID_WRITE:
  {
    break;
  }
  /********************************** CONNECTION **************************************/
  case CONNECTION:
  {
    if (!initialized)
    {
      ok.reset();
      communication.setMasterMode();
      communication.init();

      initialized = true;
    }
    if (!locked && ok.click())
    {
      startConnection = !startConnection;

      if (!startConnection)
      {
        successfullyConnected = false;
        showLocalVoltage = true;
        isPortableInited = false;
        connState = CONN_IDLE;
        DBG("Connection attempts STOPPED by user.\n");
      }

      vibro(255, 30);
    }
    break;
  }
  }
}

void loop()
{
  up.tick();
  down.tick();
  ok.tick();

  oled.clear();

  if (up.hold() && down.hold() && currentMenu != DOTS_GAME && currentMenu != SNAKE_GAME && currentMenu != FLAPPY_GAME && currentMenu != HF_REPLAY && currentMenu != IR_REPLAY && currentMenu != RFID_EMULATE)
  {
    locked = !locked;

    if (locked)
    {
      vibro(255, 30, 2);
    }
    else
    {
      vibro(255, 30);
    }
  }

  if (locked)
  {
    showLock();
  }

  if (isPortableInited)
  {
    showPortableInited();
  }

  if (isCharging)
  {
    showCharging();
  }

  if (!locked && (up.press() || ok.press() || down.press()))
  {
    resetBrightness();
  }

  if (!locked && ok.hold())
  {
    bool isSimpleMenuExit = false;
    MenuState lastMenu = currentMenu;

    if (currentMenu == MAIN_MENU)
    {
      return;
    }

    currentMenu = parentMenu;
    parentMenu = grandParentMenu;
    grandParentMenu = MAIN_MENU;

    switch (lastMenu)
    {
    case SETTINGS:
    {
      saveSettings();
      isSimpleMenuExit = true;
      break;
    }
    case CONNECTION:
    {
      initialized = false;
      isSimpleMenuExit = true;
      break;
    }
    case UHF_MENU:
    case IR_MENU:
    case RFID_MENU:
    case GAMES:
    case HF_MENU:
    case HF_AIR_MENU:
    case HF_COMMON_MENU:
    {
      vibro(255, 50);
      break;
    }
    case IR_SCAN:
    {
      signalCaptured_IR = false;
    }
    case IR_REPLAY:
    case IR_BRUTE_TV:
    case IR_BRUTE_PROJECTOR:
    {
      bruteState = BRUTE_IDLE;
      IrReceiver.disableIRIn();
      isSimpleMenuExit = true;
      break;
    }
    case HF_JAMMER:
    case HF_TESLA:
    {
      digitalWrite(GD0_PIN_CC, LOW);
      isSimpleMenuExit = true;
      break;
    }
    case HF_SPECTRUM:
    case HF_ACTIVITY:
    {
      initialized = false;
      isPortableInited = false;
      if (successfullyConnected)
      {
        sendStopCommandToSlave();
      }
      vibro(255, 50);
      break;
    }
    case HF_REPLAY:
    {
      attackIsActive = false;
      commandSent = false;
    }
    case HF_SCAN:
    {
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      signalCaptured_433MHZ = false;
      isSimpleMenuExit = true;
      break;
    }
    case UHF_SPECTRUM:
    {
      stopRadioAttack();
      communication.setMasterMode();
      communication.init();
      initialized = false;
      isPortableInited = false;
      vibro(255, 50);
      break;
    }
    case UHF_ALL_JAMMER:
    case UHF_WIFI_JAMMER:
    case UHF_BT_JAMMER:
    case UHF_BLE_JAMMER:
    case UHF_USB_JAMMER:
    case UHF_VIDEO_JAMMER:
    case UHF_RC_JAMMER:
    {
      stopRadioAttack();
      communication.setMasterMode();
      communication.init();
      initialized = false;
      isPortableInited = false;
      if (successfullyConnected)
      {
        sendStopCommandToSlave(6);
      }
      vibro(255, 50);
      break;
    }
    case UHF_BLE_SPAM:
    {
      digitalWrite(BLE_PIN, LOW);
      isSimpleMenuExit = true;
      break;
    }
    case RFID_EMULATE:
    case RFID_WRITE:
    case RFID_SCAN:
    {
      attackIsActive = false;
      nfc.setRFField(0, 0);
      nfc.powerDownMode();
      rdm6300.end();
      digitalWrite(RFID_COIL_PIN, LOW);
      ShowReboot();
      vibro(255, 50);
      while (1)
      {
      }
    }
    case DOTS_GAME:
    case SNAKE_GAME:
    case FLAPPY_GAME:
    {
      vibro(255, 50);
      break;
    }
    }

    if (isSimpleMenuExit)
    {
      initialized = false;
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_IDLE, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }
      isPortableInited = false;
      vibro(255, 50);
      return;
    }
  }

  switch (currentMenu)
  {
  case MAIN_MENU:
  {
    menuButtons(MAINmenuIndex, mainMenuCount);
    drawMenu(mainMenuItems, mainMenuCount, MAINmenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(MAINmenuIndex + 1);
      vibro(255, 50);
    }
    break;
  }
  case HF_MENU:
  {
    menuButtons(HFmenuIndex, hfMenuCount);
    drawMenu(hfMenuItems, hfMenuCount, HFmenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_AIR_MENU + HFmenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case IR_MENU:
  {
    menuButtons(IRmenuIndex, irMenuCount);
    drawMenu(irMenuItems, irMenuCount, IRmenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(IR_SCAN + IRmenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case UHF_MENU:
  {
    menuButtons(uhfMenuIndex, uhfMenuCount);
    drawMenu(uhfMenuItems, uhfMenuCount, uhfMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(UHF_SPECTRUM + uhfMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case RFID_MENU:
  {
    menuButtons(RFIDMenuIndex, RFIDMenuCount);
    drawMenu(RFIDMenuItems, RFIDMenuCount, RFIDMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(RFID_SCAN + RFIDMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case GAMES:
  {
    menuButtons(gamesMenuIndex, gamesMenuCount);
    drawMenu(gamesMenuItems, gamesMenuCount, gamesMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(DOTS_GAME + gamesMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case SETTINGS:
  {
    static uint8_t settingsMenuIndex = 0;

    if (!locked && (down.click() || down.step()))
    {
      settingsMenuIndex = (settingsMenuIndex + 1) % settingsMenuCount;
      vibro(255, 20);
    }
    if (!locked && (up.click() || up.step()))
    {
      if (settingsMenuIndex == 0)
        settingsMenuIndex = settingsMenuCount - 1;
      else
        settingsMenuIndex--;
      vibro(255, 20);
    }

    if (!locked && ok.click())
    {
      switch (settingsMenuIndex)
      {
      case 0:
        settings.saveIR = !settings.saveIR;
        break;
      case 1:
        settings.saveRA = !settings.saveRA;
        break;
      case 2:
        clearAllIRData();
        showClearConfirmation("IR");
        vibro(255, 80);
        delay(500);
        break;
      case 3:
        clearAllRAData();
        showClearConfirmation("RA");
        vibro(255, 80);
        delay(500);
        break;
      case 4:
        clearAllRAData();
        clearAllIRData();
        showClearConfirmation("All");
        vibro(255, 80);
        delay(500);
        break;
      }
    }

    drawSettingsMenu(settingsMenuIndex);
    break;
  }
  case HF_COMMON_MENU:
  {
    menuButtons(HFCommonMenuIndex, hfCommonMenuCount);
    drawMenu(hfCommonMenuItems, hfCommonMenuCount, HFCommonMenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_SCAN + HFCommonMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case HF_AIR_MENU:
  {
    menuButtons(RAsignalMenuIndex, RAsignalMenuCount);
    drawMenu(RAsignalMenuItems, RAsignalMenuCount, RAsignalMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_SPECTRUM + RAsignalMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case HF_SCAN:
  {
    if (!signalCaptured_433MHZ)
      ShowScanning_HF();
    else
    {
      ShowCapturedSignal_HF();
    }
    break;
  }
  case HF_REPLAY:
  {
    if (ok.click() && capturedCode != 0)
    {
      attackIsActive = !attackIsActive;
      vibro(255, 50);
    }

    if (up.hold() && down.hold())
    {
      clearRAData(selectedSlotRA);
      vibro(255, 50);
    }

    if (attackIsActive)
    {
      ShowAttack_HF();
    }
    else
    {
      ShowSavedSignal_HF();
    }
    break;
  }
  case HF_JAMMER:
  {
    ShowJamming_HF();
    break;
  }
  case HF_TESLA:
  {
    ShowSendingTesla_HF();
    break;
  }
  case HF_ACTIVITY:
  {
    DrawRSSIPlot_HF();
    break;
  }
  case HF_SPECTRUM:
  {
    if (!locked && ok.click())
    {
      resetSpectrum_HF();
    }
    DrawRSSISpectrum_HF();
    break;
  }
  case IR_SCAN:
  {
    if (!signalCaptured_IR)
    {
      ShowScanning_IR();
    }
    else
    {
      ShowCapturedSignal_IR();
    }
    break;
  }
  case IR_REPLAY:
  {
    if (up.hold() && down.hold())
    {
      clearIRData(selectedSlotIR);
      vibro(255, 50);
    }

    ShowSavedSignal_IR();
    break;
  }
  case IR_BRUTE_TV:
  {
    ShowBrute_IR();
    break;
  }
  case IR_BRUTE_PROJECTOR:
  {
    ShowBrute_IR();
    break;
  }
  case UHF_ALL_JAMMER:
  case UHF_WIFI_JAMMER:
  case UHF_BT_JAMMER:
  case UHF_BLE_JAMMER:
  case UHF_USB_JAMMER:
  case UHF_VIDEO_JAMMER:
  case UHF_RC_JAMMER:
  {
    ShowJamming_UHF();
    break;
  }
  case UHF_SPECTRUM:
  {
    DrawSpectrum_UHF();
    break;
  }
  case UHF_BLE_SPAM:
  {
    ShowBLESpam_UHF();
    break;
  }
  case RFID_SCAN:
  {
    // I2C connection should be through the same core.
    nfcPool();

    if (!tagDetected)
      ShowScanning_RFID();
    else
    {
      ShowCapturedData_RFID();
    }
    break;
  }
  case RFID_EMULATE:
  {
    if (ok.click() && tagID_125kHz != 0)
    {
      attackIsActive = !attackIsActive;
      vibro(255, 50);
    }

    if (up.hold() && down.hold())
    {
      clearRFIDData(selectedSlotRFID);
      vibro(255, 50);
    }

    if (attackIsActive)
    {
      ShowEmulation_RFID();
    }
    else
    {
      ShowSavedSignal_RFID();
    }

    break;
  }
  case RFID_WRITE:
  {
    break;
  }
  case DOTS_GAME:
  {

    if (!fallingDots.initialized)
    {
      fallingDots.reset();
      fallingDots.initialized = true;
    }

    fallingDots.handleInput();
    fallingDots.update();

    if (!locked && fallingDots.gameOver && ok.click())
    {
      fallingDots.reset();
    }

    break;
  }
  case SNAKE_GAME:
  {
    if (!snake.initialized)
    {
      snake.reset();
      snake.initialized = true;
    }

    snake.handleInput();
    snake.update();

    if (!locked && snake.gameOver && ok.click())
    {
      snake.reset();
    }
    break;
  }
  case FLAPPY_GAME:
  {
    if (!flappy.initialized)
    {
      flappy.reset();
      flappy.initialized = true;
    }

    flappy.update();

    if (!locked && flappy.gameOver && ok.click())
    {
      flappy.reset();
    }
    break;
  }
  case CONNECTION:
  {
    showConnectionStatus();
    break;
  }
  }

  if (successfullyConnected)
  {
    if (currentMenu != HF_ACTIVITY && currentMenu != HF_SPECTRUM && !isUltraHighFrequencyMode())
    {
      if (communication.receivePacket(recievedData, &recievedDataLen))
      {
        if (recievedData[0] == PROTOCOL_HEADER && recievedData[1] == COMMAND_BATTERY_VOLTAGE)
        {
          memcpy(&remoteVoltage, &recievedData[2], sizeof(float));
          DBG("Remote voltage updated: %.2fV\n", remoteVoltage);
        }
        else if (recievedData[0] == 'I' && recievedData[1] == 'N' && recievedData[2] == 'I' && recievedData[3] == 'T')
        {
          isPortableInited = true;
        }
        else if (recievedData[0] == 'P' && recievedData[1] == 'O' && recievedData[2] == 'N' && recievedData[3] == 'G')
        {
          DBG("Master: PONG received! Connection OK.\n");
          successfullyConnected = true;
          checkConnectionTimer = millis();
        }
      }

      if (millis() - checkConnectionTimer > CONNECTION_DELAY)
      {
        if (communication.sendPacket(ping, sizeof(ping)))
        {
          DBG("Master: PING sent.\n");
          checkConnectionTimer = millis();
          connectionAttempts = 0;
        }
        else
        {
          connectionAttempts++;
          DBG("Master: Failed to send. Attempt %d\n", connectionAttempts);
          checkConnectionTimer = millis();

          if (connectionAttempts >= N_CONNECTIONS_ATTEMPTS)
          {
            DBG("Master: Connection lost.\n");
            successfullyConnected = false;
            // startConnection = false;
            isPortableInited = false;
            connState = CONN_IDLE;
            vibro(255, 100, 3, 20);
            connectionAttempts = 0;
          }
        }
      }
    }

    drawRadioConnected();
  }
  else if (!successfullyConnected && startConnection && currentMenu != HF_ACTIVITY && currentMenu != HF_SPECTRUM && !isUltraHighFrequencyMode())
  {
    switch (connState)
    {
    case CONN_IDLE:
    {
      DBG("Master: Sending PING...\n");
      communication.sendPacket(ping, sizeof(ping));
      DBG("Master: PING sent. Waiting for PONG...\n");
      connState = CONN_AWAITING_PONG;
      pongTimeoutTimer = millis();
      break;
    }

    case CONN_AWAITING_PONG:
    {
      if (communication.receivePacket(recievedData, &recievedDataLen) &&
          recievedData[0] == 'P' && recievedData[1] == 'O' && recievedData[2] == 'N' && recievedData[3] == 'G')
      {
        DBG("Master: PONG received! Connection OK.\n");
        DBG("Connection established\n");
        successfullyConnected = true;
        showLocalVoltage = false;
        batteryDisplayToggleTimer = millis();
        connState = CONN_IDLE;
        vibro(255, 30, 1);
        vibro(255, 80, 1);
        break;
      }

      if (millis() - pongTimeoutTimer > PONG_WAIT_MS)
      {
        DBG("Master: PONG response timeout.\n");
        successfullyConnected = false;
        connState = CONN_IDLE;
      }
      break;
    }
    }
  }

  if (!isGameOrFullScreenActivity())
  {
    handleBatteryTasks();
  }

  if (!isHighFrequencyMode())
  {
    setMinBrightness();
  }

  oled.update();
}

void handleBatteryTasks()
{
  if (millis() - batteryCheckTimer >= BATTERY_CHECK_INTERVAL)
  {
    batVoltage = readBatteryVoltage();
    checkCharging(batVoltage);
    batteryCheckTimer = millis();
  }

  if (successfullyConnected && millis() - batteryDisplayToggleTimer >= 5000)
  {
    batteryDisplayToggleTimer = millis();
    showLocalVoltage = !showLocalVoltage;
  }

  if (showLocalVoltage)
  {
    drawBattery(batVoltage);
  }
  else
  {
    drawBattery(remoteVoltage, ".");
  }
}
