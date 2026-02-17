#include "visualization.h"

void handleBatteryTasks();

void setup()
{
  EEPROM.begin(8192);

  // I2C
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.setClock(400000);
  Wire.begin();

  // SPI
  SPI.setSCK(SCK_PIN_NRF);
  SPI.setTX(MOSI_PIN_NRF);
  SPI.setRX(MISO_PIN_NRF);
  SPI.begin();

  radio.begin();
  radio.powerDown();

  SPI1.setSCK(SCK_PIN_CC);
  SPI1.setTX(MOSI_PIN_CC);
  SPI1.setRX(MISO_PIN_CC);
  SPI1.begin();

  cc1101Init();

  loadStartMode();

  if (ok.read())
  {
    gamesOnlyMode = !gamesOnlyMode;
    saveStartMode();
  }
  if (gamesOnlyMode)
  {
    currentMenu = GAMES;
    APP_NAME = "eGames";
  }

  oled.init();
  oled.clear();
  resetBrightness();
  oled.update();
  ShowSplashScreen();

  delay(500);

  analogReadResolution(12);
  batVoltage = readBatteryVoltage();

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

  pinMode(RFID_POWER_PIN, OUTPUT);
  digitalWrite(RFID_POWER_PIN, HIGH);

  pinMode(VIBRO, OUTPUT);

  findLastUsedSlotRA();
  findLastUsedSlotIR();
  findLastUsedSlotRFID();
  findLastUsedSlotBarrier();

  loadSettings();
  loadAllScores();
  loadStartConnection();

  if (startConnection)
  {
    communication.setMasterMode();
    communication.init();
    connectionInited = true;
  }

  for (uint8_t i = 0; i < RSSI_BUFFER_SIZE; i++)
  {
    rssiBuffer[i] = -100;
  }

  vibro(255, 30);

#ifdef DEBUG_eHack
  Serial.begin(9600);
#endif

  // eraseAllEEPROM();
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

      addHFMonitorEntry(currentFreqIndex, capturedCode, currentRssi, capturedProtocol, false, true, true, capturedLength);

      SimpleRAData data;
      data.code = capturedCode;
      data.length = capturedLength;
      data.protocol = capturedProtocol;
      data.delay = capturedDelay;

      strncpy(data.name, "NEW", NAME_MAX_LEN);
      data.name[NAME_MAX_LEN] = '\0';

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

    if (RANameEdit || RAMenu)
    {
      return;
    }

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

      commandSent = false;
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
  /******************************* BARRIER SCAN **********************************/
  case HF_BARRIER_SCAN:
  {
    if (!initialized)
    {
      pinMode(GD0_PIN_CC, INPUT);
      attachInterrupt(digitalPinToInterrupt(GD0_PIN_CC), captureBarrierCode, CHANGE);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_SCAN, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
    }

    if (checkFreqButtons())
    {
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_SCAN, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }
    }

    if (barrierCaptured)
    {
      signalCaptured_433MHZ = true;
      currentRssi = ELECHOUSE_cc1101.getRssi();

      SimpleBarrierData data;
      data.codeMain = barrierCodeMain;
      data.codeAdd = barrierCodeAdd;
      data.protocol = barrierProtocol;

      vibro(255, 200, 3, 80);

      // Check for duplicates
      if (isDuplicateBarrier(data))
      {
        barrierCaptured = false;
        break;
      }

      if (settings.saveRA)
      {
        writeBarrierData(lastUsedSlotBarrier, data);
      }

      lastUsedSlotBarrier = (lastUsedSlotBarrier + 1) % MAX_BARRIER_SIGNALS;

      barrierCaptured = false;
    }

    break;
  }
  /********************************** BARRIER REPLAY **************************************/
  case HF_BARRIER_REPLAY:
  {
    static uint32_t attackTimer = 0;

    if (!initialized)
    {
      ok.reset();
      pinMode(GD0_PIN_CC, OUTPUT);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_REPLAY, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      commandSent = false;
      initialized = true;
    }

    if (!locked && !attackIsActive && (up.click() || up.step()))
    {
      if (selectedSlotBarrier == 0)
        selectedSlotBarrier = MAX_BARRIER_SIGNALS - 1;
      else
        selectedSlotBarrier--;
      vibro(255, 20);
    }

    if (!locked && !attackIsActive && (down.click() || down.step()))
    {
      selectedSlotBarrier = (selectedSlotBarrier + 1) % MAX_BARRIER_SIGNALS;
      vibro(255, 20);
    }

    SimpleBarrierData data = readBarrierData(selectedSlotBarrier);
    barrierCodeMain = data.codeMain;
    barrierCodeAdd = data.codeAdd;
    barrierProtocol = data.protocol;

    if (attackIsActive)
    {
      if (checkFreqButtons())
      {
        ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);
        if (successfullyConnected)
        {
          outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_REPLAY, &currentFreqIndex, 1, outgoingData);
          communication.sendPacket(outgoingData, outgoingDataLen);
        }
      }

      if (millis() - attackTimer >= 1000 && !successfullyConnected)
      {
        if (barrierProtocol == 0)
        {
          sendANMotors(barrierCodeMain, barrierCodeAdd);
        }
        else if (barrierProtocol == 1)
        {
          sendNice(barrierCodeMain);
        }
        else if (barrierProtocol == 2)
        {
          sendCame(barrierCodeMain);
        }
        attackTimer = millis();
      }
      else if (successfullyConnected && !commandSent)
      {
        if (barrierProtocol == 0)
        {
          sendANMotors(barrierCodeMain, barrierCodeAdd);
        }
        else if (barrierProtocol == 1)
        {
          sendNice(barrierCodeMain);
        }
        else if (barrierProtocol == 2)
        {
          sendCame(barrierCodeMain);
        }
        commandSent = true;
      }
    }

    break;
  }
  /********************************** BRUTE CAME **************************************/
  case HF_BARRIER_BRUTE_CAME:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      ok.reset();
      if (!successfullyConnected)
      {
        pinMode(GD0_PIN_CC, OUTPUT);
        ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);
      }
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        if (successfullyConnected)
        {
          outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_BRUTE_CAME, &currentFreqIndex, 1, outgoingData);
          communication.sendPacket(outgoingData, outgoingDataLen);
        }

        bruteState = BRUTE_RUNNING;
        barrierBruteIndex = 4095;
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

      if (millis() - lastSendTime > 50)
      {
        lastSendTime = millis();

        if (barrierBruteIndex >= 0)
        {
          if (!successfullyConnected)
            sendCame(barrierBruteIndex);
          barrierBruteIndex--;
        }
        if (barrierBruteIndex < 0)
        {
          bruteState = BRUTE_IDLE;
          vibro(255, 50);
        }
      }
      break;
    }
    case BRUTE_PAUSED:
    {
      if (!locked && (up.click() || up.step()))
      {
        if (barrierBruteIndex == 4095)
          barrierBruteIndex = 0;
        else
          barrierBruteIndex++;
        vibro(255, 20);
      }

      if (!locked && (down.click() || down.step()))
      {
        if (barrierBruteIndex == 0)
          barrierBruteIndex = 4095;
        else
          barrierBruteIndex--;
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        sendCame(barrierBruteIndex);
        vibro(255, 50);
      }
      break;
    }
    }
    break;
  }
  /********************************** BRUTE NICE **************************************/
  case HF_BARRIER_BRUTE_NICE:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      ok.reset();
      if (!successfullyConnected)
      {
        pinMode(GD0_PIN_CC, OUTPUT);
        ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);
      }
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        if (successfullyConnected)
        {
          outgoingDataLen = communication.buildPacket(COMMAND_HF_BARRIER_BRUTE_NICE, &currentFreqIndex, 1, outgoingData);
          communication.sendPacket(outgoingData, outgoingDataLen);
        }

        bruteState = BRUTE_RUNNING;
        barrierBruteIndex = 4095;
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

      if (millis() - lastSendTime > 50)
      {
        lastSendTime = millis();

        if (barrierBruteIndex >= 0)
        {
          if (!successfullyConnected)
            sendNice(barrierBruteIndex);
          barrierBruteIndex--;
        }
        if (barrierBruteIndex < 0)
        {
          bruteState = BRUTE_IDLE;
          vibro(255, 50);
        }
      }
      break;
    }
    case BRUTE_PAUSED:
    {
      if (!locked && (up.click() || up.step()))
      {
        if (barrierBruteIndex == 4095)
          barrierBruteIndex = 0;
        else
          barrierBruteIndex++;
        vibro(255, 20);
      }

      if (!locked && (down.click() || down.step()))
      {
        if (barrierBruteIndex == 0)
          barrierBruteIndex = 4095;
        else
          barrierBruteIndex--;
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        sendNice(barrierBruteIndex);
        vibro(255, 50);
      }
      break;
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
    static uint32_t lastReceivedTime = millis();
    static bool lastConnectionStatus = successfullyConnected;

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
        lastReceivedTime = millis();
      }

      initialized = true;
    }

    if (lastConnectionStatus != successfullyConnected)
    {
      initialized = false;
      lastConnectionStatus = successfullyConnected;
      DBG("Connection status changed: %s\n", successfullyConnected ? "EXTERNAL" : "INTERNAL");
      return;
    }

    if (successfullyConnected && millis() - lastReceivedTime > CONNECTION_TIMEOUT_MS)
    {
      successfullyConnected = false;
      initialized = false;
      connState = CONN_IDLE;
      vibro(255, 30, 1);
      DBG("Connection lost: no data for %d ms\n", CONNECTION_TIMEOUT_MS);
      return;
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
          if (rssiMaxPeak[currentScanFreq] > -95)
          {
            rssiMaxPeak[currentScanFreq] -= 3;
            if (rssiMaxPeak[currentScanFreq] < -95)
              rssiMaxPeak[currentScanFreq] = -95;
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
        if (rssiMaxPeak[currentScanFreq] > -95)
        {
          rssiMaxPeak[currentScanFreq] -= 3;
          if (rssiMaxPeak[currentScanFreq] < -95)
            rssiMaxPeak[currentScanFreq] = -95;
        }
      }

      if (currentRssi > rssiAbsoluteMax[currentScanFreq])
      {
        rssiAbsoluteMax[currentScanFreq] = currentRssi;
      }

      lastReceivedTime = millis();
    }

    break;
  }
  /********************************** RF ACTIVITY **************************************/
  case HF_ACTIVITY:
  {
    static uint32_t lastStepMs = millis();
    static uint32_t lastReceivedTime = millis();
    static bool lastConnectionStatus = false;

    if (!initialized)
    {
      if (!successfullyConnected)
      {
        pinMode(GD0_PIN_CC, INPUT);
        if (settings.activeScan)
        {
          mySwitch.enableReceive(GD0_PIN_CC);
        }
        ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
      }
      else
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_ACTIVITY, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
        lastReceivedTime = millis();
      }

      initialized = true;
    }

    if (lastConnectionStatus != successfullyConnected)
    {
      initialized = false;
      lastConnectionStatus = successfullyConnected;
      DBG("Connection status changed: %s\n", successfullyConnected ? "EXTERNAL" : "INTERNAL");
      return;
    }

    if (successfullyConnected && millis() - lastReceivedTime > CONNECTION_TIMEOUT_MS)
    {
      successfullyConnected = false;
      initialized = false;
      connState = CONN_IDLE;
      vibro(255, 30, 1);
      DBG("Connection lost: no data for %d ms\n", CONNECTION_TIMEOUT_MS);
      return;
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

    // Capture code
    if (settings.activeScan && mySwitch.available())
    {
      // Check if the signal is valid
      if (mySwitch.getReceivedBitlength() < 10)
      {
        mySwitch.resetAvailable();
        break;
      }

      capturedCode = mySwitch.getReceivedValue();
      capturedLength = mySwitch.getReceivedBitlength();
      capturedProtocol = mySwitch.getReceivedProtocol();
      capturedDelay = mySwitch.getReceivedDelay();
      currentRssi = ELECHOUSE_cc1101.getRssi();

      SimpleRAData data;
      data.code = capturedCode;
      data.length = capturedLength;
      data.protocol = capturedProtocol;
      data.delay = capturedDelay;

      signalCaptured_433MHZ = true;
      signalIndicatorUntil = millis() + 20000UL;

      addHFMonitorEntry(currentFreqIndex, capturedCode, currentRssi, capturedProtocol, false, true, true, capturedLength);

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
      lastReceivedTime = millis();
    }
    break;
  }
  /********************************** HF MONITOR **************************************/
  case HF_MONITOR:
  {
    static uint32_t lastReceivedTime = millis();
    static bool lastConnectionStatus = false;

    if (!initialized)
    {
      pinMode(GD0_PIN_CC, INPUT);
      mySwitch.enableReceive(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_HF_SCAN, &currentFreqIndex, 1, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
        lastReceivedTime = millis();
      }

      initialized = true;
    }

    if (mySwitch.available())
    {
      if (mySwitch.getReceivedBitlength() < 10)
      {
        mySwitch.resetAvailable();
        break;
      }

      currentRssi = ELECHOUSE_cc1101.getRssi();
      capturedCode = mySwitch.getReceivedValue();
      capturedLength = mySwitch.getReceivedBitlength();
      capturedProtocol = mySwitch.getReceivedProtocol();
      capturedDelay = mySwitch.getReceivedDelay();

      addHFMonitorEntry(currentFreqIndex, capturedCode, currentRssi, capturedProtocol, false, true, true, capturedLength);

      SimpleRAData data;
      data.code = capturedCode;
      data.length = capturedLength;
      data.protocol = capturedProtocol;
      data.delay = capturedDelay;

      strncpy(data.name, "NEW", NAME_MAX_LEN);
      data.name[NAME_MAX_LEN] = '\0';

      vibro(255, 30);

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

    if (hfMonitorSendRequested)
    {
      mySwitch.disableReceive();
      pinMode(GD0_PIN_CC, OUTPUT);
      mySwitch.enableTransmit(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      mySwitch.setProtocol(capturedProtocol);
      mySwitch.send(capturedCode, capturedLength);

      mySwitch.disableTransmit();
      pinMode(GD0_PIN_CC, INPUT);
      mySwitch.enableReceive(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      hfMonitorSendRequested = false;
    }
    break;
  }
  /********************************** RAW CAPTURE **************************************/
  case HF_RAW_CAPTURE:
  {
    if (!initialized)
    {
      rawCapturing = false;
      rawRecorded = false;
      rawReplayRequested = false;
      rawSignalCount = 0;
      rawRecordedCount = 0;
      rawFirstEdge = true;
      rawOverflow = false;

      for (uint8_t i = 0; i < RSSI_BUFFER_SIZE; i++)
        rssiBuffer[i] = -100;
      rssiIndex = 0;
      currentRssi = -100;

      pinMode(GD0_PIN_CC, INPUT);
      attachInterrupt(digitalPinToInterrupt(GD0_PIN_CC), rawSignalISR, CHANGE);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      rawStartCapture();

      initialized = true;
    }

    if (rawOverflow)
    {
      rawOverflow = false;
      rawRecorded = true;
      rawRecordedCount = rawSignalCount;
    }

    {
      static uint32_t rawLastStepMs = 0;
      if (millis() - rawLastStepMs >= RSSI_STEP_MS)
      {
        currentRssi = ELECHOUSE_cc1101.getRssi();
        rssiBuffer[rssiIndex++] = currentRssi;
        if (rssiIndex >= RSSI_BUFFER_SIZE)
          rssiIndex = 0;
        rawLastStepMs = millis();
      }
    }

    break;
  }
  /********************************** RAW REPLAY **************************************/
  case HF_RAW_REPLAY:
  {
    if (!initialized)
    {
      rawReplaying = false;
      rawReplayRequested = false;
      rawRecorded = false;

      pinMode(GD0_PIN_CC, INPUT);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);

      initialized = true;
    }

    if (rawReplayRequested && rawRecorded && !rawReplaying)
    {
      rawReplayRequested = false;
      rawReplaying = true;

      pinMode(GD0_PIN_CC, OUTPUT);
      ELECHOUSE_cc1101.SetTx(raFrequencies[currentFreqIndex]);

      bool pinState = true;
      for (uint16_t i = 0; i < rawRecordedCount; i++)
      {
        digitalWrite(GD0_PIN_CC, pinState ? HIGH : LOW);
        delayMicroseconds(rawSignalBuffer[i]);
        pinState = !pinState;
      }
      digitalWrite(GD0_PIN_CC, LOW);

      rawReplaying = false;
      rawRecorded = false;

      // Switch back to RX
      pinMode(GD0_PIN_CC, INPUT);
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentFreqIndex]);
    }

    break;
  }
  /*============================= InfraRed Protocol =================================*/
  /********************************** SCANNING **************************************/
  case IR_SCAN:
  {
    if (!initialized)
    {
      protocol = address = command = 0;
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
    static uint32_t lastCheck125kHz = 0;

    if (initialized)
    {
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
  case FM_RADIO:
  {
    if (!initialized)
    {
      if (successfullyConnected)
      {
        outgoingDataLen = communication.buildPacket(COMMAND_FM_RADIO, 0, 0, outgoingData);
        communication.sendPacket(outgoingData, outgoingDataLen);
      }

      initialized = true;
      FMReady = false;
    }

    if (!locked && ok.click())
    {
      ok.reset();
      FMStep = !FMStep;
      AlphaFM = FMStep ? 1 : 10;
      vibro(255, 20, 1, 10);
      DBG("Step: %u, alpha: %u\n", FMStep, AlphaFM);
    }

    if (FMReady && (millis() - FMLastEditMs) >= 1000)
    {
      outgoingDataLen = communication.buildPacket(COMMAND_FM_RADIO, (uint8_t *)&FrequencyFM, sizeof(FrequencyFM), outgoingData);
      communication.sendPacket(outgoingData, outgoingDataLen);
      FMblink = true;
      FMblinkTimer = millis();
      FMReady = false;
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

  if (up.hold() && down.hold() && currentMenu != DOTS_GAME && currentMenu != SNAKE_GAME && currentMenu != FLAPPY_GAME && currentMenu != HF_REPLAY && currentMenu != HF_RAW_REPLAY && currentMenu != HF_BARRIER_REPLAY && currentMenu != IR_REPLAY && currentMenu != RFID_EMULATE)
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

  // if (isPortableInited)
  // {
  //   showPortableInited();
  // }

  if (isCharging)
  {
    showCharging();
  }

  if (!locked && (up.press() || ok.press() || down.press()))
  {
    if (isScreenDimmed || isScreenOff)
    {
      up.skipEvents();
      ok.skipEvents();
      down.skipEvents();
    }
    resetBrightness();
    resetScreenOff();
  }

  if (!locked && ok.hold())
  {
    bool isSimpleMenuExit = false;
    MenuState lastMenu = currentMenu;

    if (currentMenu == MAIN_MENU)
    {
      vibro(255, 50, 3, 100);
      return;
    }

    if (gamesOnlyMode)
    {
      currentMenu = GAMES;
    }
    else
    {
      currentMenu = parentMenu;
      parentMenu = grandParentMenu;
      grandParentMenu = MAIN_MENU;
    }

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
      saveStartConnection();
      break;
    }
    case TELEMETRY:
    {
      telemetryReset();
      initialized = false;
      isSimpleMenuExit = true;
      break;
    }
    case GAMES:
    {
      if (gamesOnlyMode)
      {
        vibro(255, 50, 3, 100);
        return;
      }
    }
    case UHF_MENU:
    case IR_MENU:
    case RFID_MENU:
    case HF_MENU:
    case HF_AIR_MENU:
    case HF_RAW_MENU:
    case HF_COMMON_MENU:
    case HF_BARRIER_MENU:
    case HF_BARRIER_BRUTE_MENU:
    case TORCH:
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
      // isPortableInited = false;
      if (successfullyConnected)
      {
        sendStopCommandToSlave(6);
      }
      if (settings.activeScan)
      {
        mySwitch.disableReceive();
        mySwitch.disableTransmit();
      }
      vibro(255, 50);
      break;
    }
    case HF_RAW_CAPTURE:
    {
      detachInterrupt(digitalPinToInterrupt(GD0_PIN_CC));
      rawCapturing = false;
      initialized = false;
      vibro(255, 50);
      break;
    }
    case HF_RAW_REPLAY:
    {
      rawReplaying = false;
      rawReplayRequested = false;
      rawRecorded = false;
      RAMenu = false;
      RANameEdit = false;
      RANamePos = 0;
      initialized = false;
      vibro(255, 50);
      break;
    }
    case HF_REPLAY:
    {
      attackIsActive = false;
    }
    case HF_MONITOR:
    {
    }
    case HF_SCAN:
    {
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      signalCaptured_433MHZ = false;
      isSimpleMenuExit = true;
      break;
    }
    case HF_BARRIER_SCAN:
    {
      detachInterrupt(GD0_PIN_CC);
    }
    case HF_BARRIER_BRUTE_CAME:
    case HF_BARRIER_BRUTE_NICE:
    case HF_BARRIER_REPLAY:
    {
      isSimpleMenuExit = true;
      attackIsActive = false;
      bruteState = BRUTE_IDLE;
      break;
    }
    case UHF_SPECTRUM:
    {
      stopRadioAttack();
      communication.setMasterMode();
      communication.init();
      connectionInited = true;
      initialized = false;
      // isPortableInited = false;
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
      connectionInited = true;
      initialized = false;
      // isPortableInited = false;
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
      digitalWrite(RFID_POWER_PIN, HIGH);
      vibro(255, 50);
      break;
    }
    case DOTS_GAME:
    case SNAKE_GAME:
    case FLAPPY_GAME:
    {
      vibro(255, 50);
      break;
    }
    case FM_RADIO:
    {
      isSimpleMenuExit = true;
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
      // isPortableInited = false;
      vibro(255, 50);
      return;
    }
  }

  switch (currentMenu)
  {
  case MAIN_MENU:
  {
    if (gamesOnlyMode)
    {
      currentMenu = GAMES;
      break;
    }

    uint8_t visibleMenuCount = successfullyConnected ? mainMenuCount : mainMenuCount - 1;
    if (MAINmenuIndex >= visibleMenuCount)
      MAINmenuIndex = visibleMenuCount - 1;
    menuButtons(MAINmenuIndex, visibleMenuCount);
    drawMenu(mainMenuItems, visibleMenuCount, MAINmenuIndex);

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
      // Save IR
      case 0:
        settings.saveIR = !settings.saveIR;
        break;
      // Save RA
      case 1:
        settings.saveRA = !settings.saveRA;
        break;
      // Vibro
      case 2:
        settings.vibroOn = !settings.vibroOn;
        break;
      // Active Scan
      case 3:
        settings.activeScan = !settings.activeScan;
        break;
      // Battery display (percent/voltage)
      case 4:
        settings.showPercent = !settings.showPercent;
        break;
      // Clear IR
      case 5:
        clearAllIRData();
        showClearConfirmation("IR");
        vibro(255, 80);
        delay(500);
        break;
      // Clear RA
      case 6:
        clearAllRAData();
        showClearConfirmation("RA");
        vibro(255, 80);
        delay(500);
        break;
      // Clear RAW
      case 7:
        clearAllRawData();
        showClearConfirmation("RAW");
        vibro(255, 80);
        delay(500);
        break;
      // Clear All
      case 8:
        clearAllRAData();
        clearAllIRData();
        clearAllRawData();
        showClearConfirmation("All");
        vibro(255, 80);
        delay(500);
        break;
      // Reset Settings
      case 9:
        resetSettings();
        showClearConfirmation("SET");
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
  case HF_BARRIER_MENU:
  {
    menuButtons(barrierMenuIndex, barrierMenuCount);
    drawMenu(barrierMenuItems, barrierMenuCount, barrierMenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_BARRIER_SCAN + barrierMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case HF_BARRIER_BRUTE_MENU:
  {
    menuButtons(barrierBruteMenuIndex, barrierBruteMenuCount);
    drawMenu(barrierBruteMenuItems, barrierBruteMenuCount, barrierBruteMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_BARRIER_BRUTE_CAME + barrierBruteMenuIndex);
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
      currentMenu = static_cast<MenuState>(HF_ACTIVITY + RAsignalMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case HF_RAW_MENU:
  {
    menuButtons(rawMenuIndex, rawMenuCount);
    drawMenu(rawMenuItems, rawMenuCount, rawMenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(HF_RAW_CAPTURE + rawMenuIndex);
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
    if (!locked && up.hold() && down.hold() && capturedCode != 0)
    {
      RAMenu = true;
      RAMenuIndex = 0;
      vibro(255, 50);
    }

    if (RANameEdit)
    {
      if (!locked && (down.click() || down.step()))
      {
        slotName[RANamePos] = nextChar(slotName[RANamePos]);
        vibro(255, 20);
      }

      if (!locked && (up.click() || up.step()))
      {
        slotName[RANamePos] = prevChar(slotName[RANamePos]);
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        RANamePos++;
        vibro(255, 60);

        if (RANamePos >= NAME_MAX_LEN)
        {
          slotName[NAME_MAX_LEN] = '\0';

          SimpleRAData d = readRAData(selectedSlotRA);
          memcpy(d.name, slotName, NAME_MAX_LEN + 1);

          writeRAData(selectedSlotRA, d);

          RANameEdit = false;
          RAMenu = false;
          RANamePos = 0;
          vibro(255, 60, 2);
        }
      }

      ShowRANameEdit();
      break;
    }
    else
    {
      SimpleRAData data = readRAData(selectedSlotRA);
      capturedCode = data.code;
      capturedLength = data.length;
      capturedProtocol = data.protocol;
      capturedDelay = data.delay;

      if (data.code == 0)
      {
        strcpy(slotName, "None");
      }
      else
      {
        memcpy(slotName, data.name, NAME_MAX_LEN + 1);
        slotName[NAME_MAX_LEN] = '\0';
      }
    }

    if (!locked && ok.click() && capturedCode != 0 && !RAMenu && !RANameEdit)
    {
      attackIsActive = !attackIsActive;
      vibro(255, 50);
    }

    if (RAMenu)
    {
      if (!locked && (up.click() || up.step() || down.click() || down.step()))
      {
        RAMenuIndex = (RAMenuIndex == 0) ? 1 : 0;
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        if (RAMenuIndex == 0)
        {
          RANamePos = 0;
          RANameEdit = true;
        }
        else
        {
          clearRAData(selectedSlotRA);
          vibro(255, 200);
        }

        RAMenu = false;
        vibro(255, 50);
      }

      ShowRAMenu();
    }
    else
    {
      if (attackIsActive)
      {
        ShowAttack_HF();
      }
      else
      {
        ShowSavedSignal_HF();
      }
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
  case HF_MONITOR:
  {
    if (!locked && (up.click() || up.step()) && hfMonitorCount > 0)
    {
      hfMonitorAutoFollow = false;
      if (hfMonitorCursorIndex > 0)
      {
        hfMonitorCursorIndex--;
      }
      vibro(255, 20);
    }

    if (!locked && (down.click() || down.step()) && hfMonitorCount > 0)
    {
      hfMonitorAutoFollow = false;
      if (hfMonitorCount > 0 && hfMonitorCursorIndex < hfMonitorCount - 1)
      {
        hfMonitorCursorIndex++;
      }
      vibro(255, 20);
    }

    if (!locked && ok.click() && hfMonitorCount > 0)
    {
      uint8_t ringIndex = (hfMonitorHead + hfMonitorCursorIndex) % MAX_HF_MONITOR_SIGNALS;
      HFMonitorEntry entry = hfMonitorEntries[ringIndex];

      if (entry.codeValid)
      {
        capturedCode = entry.code;
        capturedProtocol = entry.protocol;
        capturedLength = entry.bitLength;
        hfMonitorSendRequested = true;
        vibro(255, 50);
      }
    }

    ShowMonitor_HF();
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
  case HF_RAW_CAPTURE:
  {
    if (!locked && ok.click())
    {
      if (rawCapturing)
      {
        rawStopCapture();
        vibro(255, 50);
      }
    }

    if (!locked && up.click())
    {
      if (!rawCapturing && rawRecorded)
      {
        uint8_t slot = findNextFreeRawSlot();
        writeRawData(slot, currentFreqIndex);
        vibro(255, 80);

        rawRecorded = false;
        rawRecordedCount = 0;
        rawSignalCount = 0;
        rawStartCapture();
      }
    }

    if (!locked && down.click())
    {
      if (!rawCapturing && rawRecorded)
      {
        rawRecorded = false;
        rawRecordedCount = 0;
        rawSignalCount = 0;
        rawStartCapture();
        vibro(255, 30);
      }
    }

    DrawRAWOscillogram_HF();
    break;
  }
  case HF_RAW_REPLAY:
  {
    if (!locked && up.hold() && down.hold() && isRawSlotOccupied(selectedSlotRAW))
    {
      RAMenu = true;
      RAMenuIndex = 0;
      vibro(255, 50);
    }

    if (RANameEdit)
    {
      if (!locked && (down.click() || down.step()))
      {
        slotName[RANamePos] = nextChar(slotName[RANamePos]);
        vibro(255, 20);
      }

      if (!locked && (up.click() || up.step()))
      {
        slotName[RANamePos] = prevChar(slotName[RANamePos]);
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        RANamePos++;
        vibro(255, 60);

        if (RANamePos >= NAME_MAX_LEN)
        {
          slotName[NAME_MAX_LEN] = '\0';
          writeRawSlotName(selectedSlotRAW, slotName);
          RANameEdit = false;
          RAMenu = false;
          RANamePos = 0;
          vibro(255, 60, 2);
        }
      }

      ShowRANameEdit();
      break;
    }

    if (RAMenu)
    {
      if (!locked && (up.click() || up.step() || down.click() || down.step()))
      {
        RAMenuIndex = (RAMenuIndex == 0) ? 1 : 0;
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        if (RAMenuIndex == 0)
        {
          readRawSlotName(selectedSlotRAW, slotName);
          RANamePos = 0;
          RANameEdit = true;
        }
        else
        {
          clearRawData(selectedSlotRAW);
          vibro(255, 200);
        }

        RAMenu = false;
        vibro(255, 50);
      }

      ShowRAMenu();
    }
    else
    {
      if (!locked && (up.click() || up.step()))
      {
        selectedSlotRAW = (selectedSlotRAW == 0) ? MAX_RAW_SIGNALS - 1 : selectedSlotRAW - 1;
        vibro(255, 20);
      }
      if (!locked && (down.click() || down.step()))
      {
        selectedSlotRAW = (selectedSlotRAW + 1) % MAX_RAW_SIGNALS;
        vibro(255, 20);
      }

      if (!locked && ok.click())
      {
        if (isRawSlotOccupied(selectedSlotRAW))
        {
          readRawData(selectedSlotRAW);
          rawReplayRequested = true;
          rawSendIndicatorUntil = millis() + 500;
          vibro(255, 30);
        }
        else
        {
          vibro(255, 100);
        }
      }

      DrawRAWReplay();
    }
    break;
  }
  case HF_BARRIER_SCAN:
  {
    if (!signalCaptured_433MHZ)
      ShowScanning_HF();
    else
    {
      ShowCapturedBarrier_HF();
    }
    break;
  }
  case HF_BARRIER_REPLAY:
  {
    if (ok.click() && barrierCodeMain != 0)
    {
      attackIsActive = !attackIsActive;
      vibro(255, 50);
    }

    if (up.hold() && down.hold())
    {
      clearBarrierData(selectedSlotBarrier);
      vibro(255, 50);
    }

    if (attackIsActive)
    {
      ShowAttack_HF();
    }
    else
    {
      ShowSavedSignalBarrier_HF();
    }
    break;
  }
  case HF_BARRIER_BRUTE_CAME:
  {
    ShowBarrierBrute_HF(2);
    break;
  }
  case HF_BARRIER_BRUTE_NICE:
  {
    ShowBarrierBrute_HF(1);
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
    if (!initialized)
    {
      digitalWrite(RFID_POWER_PIN, LOW);
      rdm6300.begin(RFID_RX_PIN);
      nfc.begin();
      nfc.setPassiveActivationRetries(0xFF);
      nfc.powerDownMode();
      nfc.SAMConfig();
      initialized = true;
    }

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
  case FM_RADIO:
  {
    if (!locked && down.click())
    {
      fmTouch(+FM_STEP * AlphaFM, 20);
    }

    else if (!locked && down.step())
    {
      fmTouch(+FM_STEP * 5 * AlphaFM, 10);
    }

    if (!locked && up.click())
    {
      fmTouch(-FM_STEP * AlphaFM, 20);
    }
    else if (!locked && up.step())
    {
      fmTouch(-FM_STEP * 5 * AlphaFM, 10);
    }

    ShowFMFrequency();
    break;
  }
  case TORCH:
  {
    static bool solidMode = true;
    static uint32_t lastToggle = 0;
    static bool ledOn = true;

    if (ok.click())
    {
      solidMode = !solidMode;
      vibro(255, 50);
      DBG("Torch mode: %s\n", solidMode ? "SOLID" : "BLINKING");
    }

    if (solidMode)
    {
      oled.rect(0, 0, 128, 64, 1);
    }
    else
    {
      if (millis() - lastToggle > 1000)
      {
        lastToggle = millis();
        ledOn = !ledOn;
      }
      oled.rect(0, 0, 128, 64, ledOn ? 1 : 0);
    }

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
    static uint8_t settingsMenuIndex = 0;

    if (!connectionInited)
    {
      ok.reset();
      communication.setMasterMode();
      communication.init();
      connectionInited = true;
    }
    if (!locked && ok.click())
    {
      if (settingsMenuIndex == 0)
      {
        startConnection = !startConnection;

        if (!startConnection)
        {
          successfullyConnected = false;
          showLocalVoltage = true;
          // isPortableInited = false;
          connState = CONN_IDLE;
          DBG("Connection attempts STOPPED by user.\n");
        }

        vibro(255, 30);
      }
      else if (settingsMenuIndex == 1)
      {
        unsigned long spamStartTime = millis();
        bool success = false;
        while (millis() - spamStartTime < 1000)
        {
          success = communication.sendPacket(disableModule, sizeof(disableModule));
          if (success)
            break;
        }
        successfullyConnected = false;
        startConnection = false;
        showLocalVoltage = true;
        connState = CONN_IDLE;
        settingsMenuIndex = 0;
        DBG("Remote device turned off by user.\n");
        vibro(255, 30);
      }
    }

    if (!locked && (down.click() || down.step()) && successfullyConnected)
    {
      settingsMenuIndex = (settingsMenuIndex + 1) % 2;
      vibro(255, 20);
    }
    if (!locked && (up.click() || up.step()) && successfullyConnected)
    {
      if (settingsMenuIndex == 0)
        settingsMenuIndex = 1;
      else
        settingsMenuIndex = 0;
      vibro(255, 20);
    }

    showConnectionStatus(settingsMenuIndex);
    break;
  }
  case TELEMETRY:
  {
    if (!initialized && successfullyConnected)
    {
      telemetryReset();
      telemetryPingTimer = millis();
      telemetryStartMs = millis();
      telemetryTestActive = true;
      initialized = true;
    }

    if (telemetryTestActive && successfullyConnected)
    {
      switch (telemetryState)
      {
      case TELEM_IDLE:
      {
        while (communication.receivePacket(recievedData, &recievedDataLen))
        {
          if (recievedData[0] == PROTOCOL_HEADER && recievedData[1] == COMMAND_BATTERY_VOLTAGE)
            memcpy(&remoteVoltage, &recievedData[2], sizeof(float));
        }

        if (millis() - telemetryPingTimer >= TELEMETRY_PING_INTERVAL)
        {
          if (communication.sendPacket(ping, sizeof(ping)))
          {
            telemetrySent++;
            telemetryState = TELEM_AWAITING_PONG;
            telemetryPongTimer = millis();
            DBG("Telemetry: PING #%d sent.\n", telemetrySent);
          }
          else
          {
            telemetrySent++;
            telemetryPingTimer = millis();
            DBG("Telemetry: PING #%d send failed.\n", telemetrySent);
          }
        }
        break;
      }
      case TELEM_AWAITING_PONG:
      {
        if (communication.receivePacket(recievedData, &recievedDataLen))
        {
          if (recievedData[0] == 'P' && recievedData[1] == 'O' && recievedData[2] == 'N' && recievedData[3] == 'G')
          {
            telemetryReceived++;
            uint16_t rtt = millis() - telemetryPongTimer;
            telemetryRttSum += rtt;
            if (rtt > telemetryRttMax)
              telemetryRttMax = rtt;
            telemetryLossStreak = 0;
            checkConnectionTimer = millis();
            telemetryState = TELEM_IDLE;
            telemetryPingTimer = millis();
            DBG("Telemetry: PONG #%d received, RTT=%dms.\n", telemetryReceived, rtt);
          }
          else if (recievedData[0] == PROTOCOL_HEADER && recievedData[1] == COMMAND_BATTERY_VOLTAGE)
          {
            memcpy(&remoteVoltage, &recievedData[2], sizeof(float));
          }
        }

        if (millis() - telemetryPongTimer > TELEMETRY_PONG_TIMEOUT)
        {
          telemetryLossStreak++;
          if (telemetryLossStreak > telemetryLossStreakMax)
            telemetryLossStreakMax = telemetryLossStreak;
          telemetryState = TELEM_IDLE;
          telemetryPingTimer = millis();
          DBG("Telemetry: PONG timeout.\n");
        }
        break;
      }
      }
    }

    ShowTelemetry();
    break;
  }
  }

  if (successfullyConnected && currentMenu != TELEMETRY)
  {
    if (currentMenu != HF_ACTIVITY && currentMenu != HF_SPECTRUM && currentMenu != HF_RAW_CAPTURE && currentMenu != HF_RAW_REPLAY && !isUltraHighFrequencyMode())
    {
      if (communication.receivePacket(recievedData, &recievedDataLen))
      {
        if (recievedData[0] == PROTOCOL_HEADER && recievedData[1] == COMMAND_BATTERY_VOLTAGE)
        {
          memcpy(&remoteVoltage, &recievedData[2], sizeof(float));
          DBG("Remote voltage updated: %.2fV\n", remoteVoltage);
        }
        // else if (recievedData[0] == 'I' && recievedData[1] == 'N' && recievedData[2] == 'I' && recievedData[3] == 'T')
        // {
        // isPortableInited = true;
        // }
        else if (recievedData[0] == PROTOCOL_HEADER && recievedData[1] == 0x19)
        {
          int8_t fmLevel;
          memcpy(&fmLevel, &recievedData[2], sizeof(fmLevel));
          DBG("FM input level: %d dB\n", fmLevel);
          FmSoundLevel = fmLevel;
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
            // isPortableInited = false;
            connState = CONN_IDLE;
            vibro(255, 100, 3, 20);
            connectionAttempts = 0;
            FmSoundLevel = -100;
          }
        }
      }
    }

    drawRadioConnected();
  }
  else if (!successfullyConnected && startConnection && !isUltraHighFrequencyMode())
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

  if (!isHighFrequencyMode() && currentMenu != TORCH)
  {
    setMinBrightness();
  }

  if (!isScreenOff && isActiveMode() && millis() - offTimer > SCREENOFF_TIME)
  {
    isScreenOff = true;
  }
  else if (isScreenOff)
  {
    oled.clear();

    if ((millis() / 1000) % 2)
    {
      oled.circle(122, 4, 1, 1);
    }
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
