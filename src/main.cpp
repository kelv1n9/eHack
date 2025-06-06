#include "visualization.h"

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

  radio.powerDown();

  // digitalWrite(23, HIGH); // PFM to PWM

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
  findLastUsedSlotBarrier();

  loadSettings();
  loadAllScores();

  ELECHOUSE_cc1101.setSpiPin(6, 4, 7, CSN_PIN_CC);

  ELECHOUSE_cc1101.setClb(1, 11, 13);
  ELECHOUSE_cc1101.setClb(2, 14, 17);
  ELECHOUSE_cc1101.setClb(3, 29, 33);
  ELECHOUSE_cc1101.setClb(4, 33, 34);

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2); // ASK
  ELECHOUSE_cc1101.setRxBW(135);     // 58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650 and 812 kHz
  ELECHOUSE_cc1101.setGDO0(GD0_PIN_CC);
  ELECHOUSE_cc1101.setPA(12);                // TxPower: (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
  ELECHOUSE_cc1101.setMHZ(raFrequencies[1]); // 300-348 MHZ, 387-464MHZ and 779-928MHZ
  ELECHOUSE_cc1101.setDcFilterOff(0); // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity). (leave at 0 → better sensitivity)
  ELECHOUSE_cc1101.setPQT(0);         // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted. (PQT=1 is safe for ASK)
  ELECHOUSE_cc1101.setPRE(0);         // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24 (6 or higher helps detect ASK bursts)
  ELECHOUSE_cc1101.setSyncMode(0);    // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. (2 is optimal for ASK + RCSwitch)
  ELECHOUSE_cc1101.setFEC(0);         // Enable Forward Error Correction (FEC). 0 = Disable, 1 = Enable. (leave at 0 for RCSwitch ASK)
  ELECHOUSE_cc1101.setCCMode(0);           // set config for internal transmission mode.
  ELECHOUSE_cc1101.setPktFormat(0);        // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. (needed for RCSwitch, leave at 0)
  ELECHOUSE_cc1101.setAdrChk(0);           // Controls address check configuration of received packages. 0 = No address check. (leave at 0 for RCSwitch ASK)
  ELECHOUSE_cc1101.goSleep();

  for (uint8_t i = 0; i < RSSI_BUFFER_SIZE; i++)
  {
    rssiBuffer[i] = -100;
  }

  vibro(255, 30);
}

void setup1()
{
}

void loop1()
{
  uint32_t now = millis();

  switch (currentMenu)
  {
  /*============================= 433 MHz Protocol =================================*/
  /********************************** SCANNING **************************************/
  case RA_SCAN:
  {
    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableTransmit();
      mySwitch.enableReceive(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetRx(raFrequencies[1]);
      mySwitch.resetAvailable();
      initialized = true;
    }

    changeFreqButtons("RX");

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
  case RA_ATTACK:
  {
    static uint32_t attackTimer = 0;

    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.enableTransmit(GD0_PIN_CC);
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
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

    if (attackIsActive && now - attackTimer >= 1000)
    {
      mySwitch.send(capturedCode, capturedLength);
      attackTimer = now;
    }

    break;
  }
  /******************************* BARRIER SCAN **********************************/
  case BARRIER_SCAN:
  {
    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetRx(raFrequencies[1]);
      attachInterrupt(digitalPinToInterrupt(GD0_PIN_CC), captureBarrierCode, CHANGE);
      initialized = true;
    }

    changeFreqButtons("RX");

    if (anMotorsCaptured || cameCaptured || niceCaptured)
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
        anMotorsCaptured = false;
        niceCaptured = false;
        cameCaptured = false;

        break;
      }

      if (settings.saveRA)
      {
        writeBarrierData(lastUsedSlotBarrier, data);
      }

      lastUsedSlotBarrier = (lastUsedSlotBarrier + 1) % MAX_BARRIER_SIGNALS;

      anMotorsCaptured = false;
      niceCaptured = false;
      cameCaptured = false;
    }

    break;
  }
  /********************************** BARRIER REPLAY **************************************/
  case BARRIER_REPLAY:
  {
    static uint32_t attackTimer = 0;

    if (!initialized)
    {
      ok.reset();
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
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

    if (attackIsActive && now - attackTimer >= 1000)
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
      attackTimer = now;
    }

    break;
  }
  /********************************** BRUTE CAME **************************************/
  case BARRIER_BRUTE_CAME:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      ok.reset();
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_RUNNING;
        barrierBruteIndex = 4095;
        lastSendTime = now;
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

      if (now - lastSendTime > 50)
      {
        lastSendTime = now;

        if (barrierBruteIndex >= 0)
        {
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
  case BARRIER_BRUTE_NICE:
  {
    static uint32_t lastSendTime = 0;

    if (!initialized)
    {
      ok.reset();
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
      initialized = true;
    }

    switch (bruteState)
    {
    case BRUTE_IDLE:
    {
      if (!locked && ok.click())
      {
        bruteState = BRUTE_RUNNING;
        barrierBruteIndex = 4095;
        lastSendTime = now;
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

      if (barrierBruteIndex >= 0)
      {
        sendNice(barrierBruteIndex);
        barrierBruteIndex--;
      }
      if (barrierBruteIndex < 0)
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
  case RA_NOISE:
  {
    static uint32_t lastNoise = 0;
    static bool noiseState = false;
    uint32_t nowMicros = micros();

    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
      initialized = true;
    }

    changeFreqButtons("TX");

    if (nowMicros - lastNoise > 500)
    {
      noiseState = !noiseState;
      digitalWrite(GD0_PIN_CC, noiseState);
      lastNoise = nowMicros;
    }
    break;
  }
  /********************************** TESLA **************************************/
  case RA_TESLA:
  {
    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      pinMode(GD0_PIN_CC, OUTPUT);
      ELECHOUSE_cc1101.SetTx(raFrequencies[1]);
      initialized = true;
      // vibro(255, 50);
    }

    static bool toggleFreq = false;
    float freq = toggleFreq ? 315.0 : 433.92;
    ELECHOUSE_cc1101.SetTx(freq);
    toggleFreq = !toggleFreq;

    sendTeslaSignal_v1();
    delay(50);
    sendTeslaSignal_v2();
    delay(50);

    break;
  }
  /********************************** RF SPECTRUM **************************************/
  case RA_SPECTRUM:
  {
    static uint32_t spectrumTimer = 0;
    static bool waitingForSettle = false;

    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      initialized = true;

      currentScanFreq = 0;
      ELECHOUSE_cc1101.SetRx(raFrequencies[currentScanFreq]);
      spectrumTimer = millis();
      waitingForSettle = true;
    }

    if (waitingForSettle)
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

    break;
  }
  /********************************** RF ACTIVITY **************************************/
  case RA_ACTIVITY:
  {
    static uint32_t lastStepMs = millis();

    if (!initialized)
    {
      ELECHOUSE_cc1101.Init();
      mySwitch.disableReceive();
      mySwitch.disableTransmit();
      ELECHOUSE_cc1101.SetRx(raFrequencies[1]);
      initialized = true;
    }

    changeFreqButtons("RX");

    if (millis() - lastStepMs >= RSSI_STEP_MS)
    {
      currentRssi = ELECHOUSE_cc1101.getRssi();
      rssiBuffer[rssiIndex++] = currentRssi;
      if (rssiIndex >= RSSI_BUFFER_SIZE)
        rssiIndex = 0;

      lastStepMs = millis();
    }
    break;
  }
  /*============================= InfraRed Protocol =================================*/
  /********************************** SCANNING **************************************/
  case IR_RECEIVER:
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
  case IR_SENDER:
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
  case IR_TV:
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
        lastSendTime = now;
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

      if (now - lastSendTime > 50 && currentIndex < IR_COMMAND_COUNT_TV)
      {
        getIRCommand(irCommandsTV, currentIndex, protocol, address, command);
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        lastSendTime = now;
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
  case IR_PROJECTOR:
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
        lastSendTime = now;
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

      if (now - lastSendTime > 20 && currentIndex < IR_COMMAND_COUNT_PR)
      {
        getIRCommand(irCommandsPR, currentIndex, protocol, address, command);
        IrSender.write(static_cast<decode_type_t>(protocol), address, command, IR_N_REPEATS);
        delay(DELAY_AFTER_SEND);
        lastSendTime = now;
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
  case UHF_ALL:
  {
    if (!initialized)
    {
      initRadioAttack();
      initialized = true;
    }
    radioChannel = random(0, 125);
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_WIFI:
  {
    if (!initialized)
    {
      initRadioAttack();
      initialized = true;
    }
    radioChannel = random(1, 15);
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_BLUETOOTH:
  {
    if (!initialized)
    {
      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
    radioChannel = bluetooth_channels[randomIndex];
    radio.setChannel(radioChannel);
    break;
  }
  case UHF_BLE:
  {
    if (!initialized)
    {
      initRadioAttack();
      initialized = true;
    }
    int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
    radioChannel = ble_channels[randomIndex];
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
  /********************************** ENABLE WIFI **************************************/
  case BLE_SPAM:
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
    if (now - lastCheck125kHz >= 10)
    {
      lastCheck125kHz = now;

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
  }
}

void loop()
{
  up.tick();
  down.tick();
  ok.tick();

  oled.clear();

  if (up.hold() && down.hold() && currentMenu != FALLING_DOTS_GAME && currentMenu != SNAKE && currentMenu != FLAPPY && currentMenu != RA_ATTACK && currentMenu != IR_SENDER && currentMenu != BARRIER_REPLAY && currentMenu != RFID_EMULATE)
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

  if (!locked && (up.press() || ok.press() || down.press()))
  {
    resetBrightness();
  }

  if (!locked && ok.hold())
  {
    if (currentMenu == MAIN_MENU)
    {
      return;
    }

    // Exiting menu
    if (currentMenu == HF_MENU || currentMenu == UHF_MENU || currentMenu == IR_TOOLS || currentMenu == RFID_MENU || currentMenu == GAMES || currentMenu == RA_AIR || currentMenu == RA_BARRIER || currentMenu == BARRIER_BRUTE)
    {
      currentMenu = parentMenu;
      parentMenu = grandParentMenu;
      grandParentMenu = MAIN_MENU;
      vibro(255, 50);
      return;
    }

    if (currentMenu == SETTINGS)
    {
      saveSettings();
      vibro(255, 50);
    }

    if (currentMenu == RFID_SCAN)
    {
      ShowReboot();
      while (1)
      {
      }
    }

    // Menu return
    currentMenu = parentMenu;
    parentMenu = grandParentMenu;
    grandParentMenu = MAIN_MENU;

    // States
    bruteState = BRUTE_IDLE;
    initialized = false;
    signalCaptured_IR = false;
    mySwitchIsAvailable = false;
    attackIsActive = false;
    signalCaptured_433MHZ = false;
    ELECHOUSE_cc1101.SetRx(raFrequencies[1]);

    // Disable
    ELECHOUSE_cc1101.goSleep();
    mySwitch.disableReceive();
    mySwitch.disableTransmit();
    IrReceiver.disableIRIn();
    rdm6300.end();
    nfc.setRFField(0, 0);
    nfc.powerDownMode();
    detachInterrupt(GD0_PIN_CC);
    pinMode(GD0_PIN_CC, INPUT);
    digitalWrite(GD0_PIN_CC, LOW);
    digitalWrite(RFID_COIL_PIN, LOW);
    digitalWrite(BLE_PIN, LOW);
    stopRadioAttack();

    vibro(255, 50);
    return;
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
      currentMenu = static_cast<MenuState>(RA_AIR + HFmenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case IR_TOOLS:
  {
    menuButtons(IRmenuIndex, irMenuCount);
    drawMenu(irMenuItems, irMenuCount, IRmenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(IR_RECEIVER + IRmenuIndex);
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
      currentMenu = static_cast<MenuState>(FALLING_DOTS_GAME + gamesMenuIndex);
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
  case RA_BARRIER:
  {
    menuButtons(barrierMenuIndex, barrierMenuCount);
    drawMenu(barrierMenuItems, barrierMenuCount, barrierMenuIndex);

    if (!locked && ok.click())
    {
      ok.reset();
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(BARRIER_SCAN + barrierMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case BARRIER_BRUTE:
  {
    menuButtons(barrierBruteMenuIndex, barrierBruteMenuCount);
    drawMenu(barrierBruteMenuItems, barrierBruteMenuCount, barrierBruteMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(BARRIER_BRUTE_CAME + barrierBruteMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case RA_AIR:
  {
    menuButtons(RAsignalMenuIndex, RAsignalMenuCount);
    drawMenu(RAsignalMenuItems, RAsignalMenuCount, RAsignalMenuIndex);

    if (!locked && ok.click())
    {
      grandParentMenu = parentMenu;
      parentMenu = currentMenu;
      currentMenu = static_cast<MenuState>(RA_SPECTRUM + RAsignalMenuIndex);
      vibro(255, 50);
    }
    break;
  }
  case RA_SCAN:
  {
    if (!signalCaptured_433MHZ)
      ShowScanning_433MHZ();
    else
    {
      ShowCapturedSignal_433MHZ();
    }
    break;
  }
  case RA_ATTACK:
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
      ShowAttack_RA();
      changeFreqButtons("TX");
    }
    else
    {
      ShowSavedSignal_RA();
    }
    break;
  }
  case RA_NOISE:
  {
    ShowRA_NOISE();
    break;
  }
  case RA_TESLA:
  {
    ShowSend_Tesla();
    break;
  }
  case RA_ACTIVITY:
  {
    drawRSSIGraph();
    break;
  }
  case RA_SPECTRUM:
  {
    if (!locked && ok.click())
    {
      resetRFSpectrum();
    }
    drawRSSISpectrum();
    break;
  }
  case BARRIER_SCAN:
  {
    if (!signalCaptured_433MHZ)
      ShowScanning_433MHZ();
    else
    {
      ShowCapturedBarrier_433MHZ();
    }
    break;
  }
  case BARRIER_REPLAY:
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
      ShowAttack_RA();
      changeFreqButtons("TX");
    }
    else
    {
      ShowSavedSignal_Barrier();
    }
    break;
  }
  case BARRIER_BRUTE_CAME:
  {
    showBarrier_Brute(2);
    break;
  }
  case BARRIER_BRUTE_NICE:
  {
    showBarrier_Brute(1);
    break;
  }
  case IR_RECEIVER:
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
  case IR_SENDER:
  {
    if (up.hold() && down.hold())
    {
      clearIRData(selectedSlotIR);
      vibro(255, 50);
    }

    ShowSavedSignal_IR();
    break;
  }
  case IR_TV:
  {
    showIR_Brute();
    break;
  }
  case IR_PROJECTOR:
  {
    showIR_Brute();
    break;
  }
  case UHF_ALL:
  {
    ShowUHF();
    break;
  }
  case UHF_WIFI:
  {
    ShowUHF();
    break;
  }
  case UHF_BLUETOOTH:
  {
    ShowUHF();
    break;
  }
  case UHF_BLE:
  {
    ShowUHF();
    break;
  }
  case UHF_SPECTRUM:
  {
    drawSpectrum();
    break;
  }
  case BLE_SPAM:
  {
    ShowBLE();
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
  case FALLING_DOTS_GAME:
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
  case SNAKE:
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
  case FLAPPY:
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
  }

  if (millis() - batteryTimer >= BATTERY_CHECK_INTERVAL && currentMenu != FALLING_DOTS_GAME && currentMenu != SNAKE && currentMenu != FLAPPY && currentMenu != RA_ACTIVITY)
  {
    batVoltage = readBatteryVoltage();
    batteryTimer = millis();
  }

  if (currentMenu != FALLING_DOTS_GAME && currentMenu != SNAKE && currentMenu != FLAPPY && currentMenu != RA_ACTIVITY)
  {
    drawBattery(batVoltage);
  }

  if (currentMenu != RA_ACTIVITY && currentMenu != RA_SPECTRUM && currentMenu != BARRIER_SCAN && currentMenu != RA_SCAN)
  {
    setMinBrightness();
  }

  oled.update();
}
