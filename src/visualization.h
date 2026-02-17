#include "functions.h"

enum MenuState
{
  MAIN_MENU,

  HF_MENU,
  UHF_MENU,
  IR_MENU,
  FM_RADIO,
  RFID_MENU,
  GAMES,
  TORCH,
  CONNECTION,
  SETTINGS,
  TELEMETRY,

  DOTS_GAME,
  SNAKE_GAME,
  FLAPPY_GAME,

  HF_AIR_MENU,
  HF_RAW_MENU,
  HF_COMMON_MENU,
  HF_BARRIER_MENU,
  HF_JAMMER,
  HF_TESLA,

  HF_ACTIVITY,
  HF_SPECTRUM,

  HF_RAW_CAPTURE,
  HF_RAW_REPLAY,

  HF_SCAN,
  HF_REPLAY,
  HF_MONITOR,

  HF_BARRIER_SCAN,
  HF_BARRIER_REPLAY,
  HF_BARRIER_BRUTE_MENU,
  HF_BARRIER_BRUTE_CAME,
  HF_BARRIER_BRUTE_NICE,

  IR_SCAN,
  IR_REPLAY,
  IR_BRUTE_TV,
  IR_BRUTE_PROJECTOR,

  UHF_SPECTRUM,
  UHF_ALL_JAMMER,
  UHF_WIFI_JAMMER,
  UHF_BT_JAMMER,
  UHF_BLE_JAMMER,
  UHF_USB_JAMMER,
  UHF_VIDEO_JAMMER,
  UHF_RC_JAMMER,
  UHF_BLE_SPAM,

  RFID_SCAN,
  RFID_EMULATE,
  RFID_WRITE,
};

const char PROGMEM *mainMenuItems[] = {
    "SubGHz",
    "2.4 GHz",
    "IR Tools",
    "FM Radio",
    "RFID",
    "Games",
    "Torch",
    "Connect",
    "Settings",
    "Telemetry",
};

const char PROGMEM *hfMenuItems[] = {
    "Air Scan",
    "Raw Scan",
    "Common",
    "Gates",
    "Jammer",
    "Tesla",
};

const char PROGMEM *hfCommonMenuItems[] = {
    "Capture",
    "Replay",
    "Monitor",
};

const char PROGMEM *irMenuItems[] = {
    "Capture",
    "Replay",
    "TV BF",
    "Proj. BF",
};

const char PROGMEM *uhfMenuItems[] = {
    "Spectrum",
    "All jam",
    "Wi-Fi jam",
    "BT jam",
    "BLE jam",
    "USB jam",
    "VIDEO jam",
    "RC jam",
    "BLE spam",
};

const char PROGMEM *RFIDMenuItems[] = {
    "Read",
    "Emulate",
    "Write",
};

const char PROGMEM *gamesMenuItems[] = {
    "Dots",
    "Snake",
    "Flappy",
};

const char PROGMEM *RAsignalMenuItems[] = {
    "Activity",
    "Spectrum",
};

const char PROGMEM *rawMenuItems[] = {
    "Capture",
    "Replay",
};

const char PROGMEM *barrierMenuItems[] = {
    "Capture",
    "Replay",
    "Brute",
};

const char PROGMEM *barrierBruteMenuItems[] = {
    "CAME",
    "NICE",
};

const char PROGMEM *settingsItems[] = {
    "Save IR:",
    "Save RA:",
    "Vibro:",
    "Active Scan:",
    "Battery:",
};

const char PROGMEM *settingsItemsAdditional[] = {
    "Remove IR Data",
    "Remove RA Data",
    "Remove RAW Data",
    "Remove ALL Data",
    "Reset Settings",
};

MenuState currentMenu = MAIN_MENU;
MenuState parentMenu = MAIN_MENU;
MenuState grandParentMenu = MAIN_MENU;
uint16_t sineOffset = 0;

bool isGameOrFullScreenActivity()
{
  return (currentMenu == DOTS_GAME ||
          currentMenu == SNAKE_GAME ||
          currentMenu == FLAPPY_GAME ||
          currentMenu == HF_ACTIVITY ||
          currentMenu == HF_RAW_CAPTURE ||
          currentMenu == HF_MONITOR ||
          currentMenu == TORCH);
}

bool isHighFrequencyMode()
{
  return (currentMenu == HF_ACTIVITY ||
          currentMenu == HF_SPECTRUM ||
          currentMenu == HF_RAW_CAPTURE ||
          currentMenu == HF_RAW_REPLAY ||
          currentMenu == HF_SCAN ||
          currentMenu == HF_MONITOR);
}

bool isUltraHighFrequencyMode()
{
  return (currentMenu == UHF_SPECTRUM ||
          currentMenu == UHF_ALL_JAMMER ||
          currentMenu == UHF_WIFI_JAMMER ||
          currentMenu == UHF_BT_JAMMER ||
          currentMenu == UHF_BLE_JAMMER ||
          currentMenu == UHF_USB_JAMMER ||
          currentMenu == UHF_VIDEO_JAMMER ||
          currentMenu == UHF_RC_JAMMER);
}

bool isActiveMode()
{
  return (
      currentMenu == HF_JAMMER ||
      currentMenu == HF_TESLA ||
      currentMenu == HF_ACTIVITY ||
      currentMenu == HF_SPECTRUM ||
      currentMenu == HF_SCAN ||
      currentMenu == HF_MONITOR ||
      currentMenu == HF_REPLAY ||
      currentMenu == HF_RAW_CAPTURE ||
      currentMenu == HF_RAW_REPLAY ||

      currentMenu == HF_BARRIER_SCAN ||
      currentMenu == HF_BARRIER_REPLAY ||
      currentMenu == HF_BARRIER_BRUTE_MENU ||
      currentMenu == HF_BARRIER_BRUTE_CAME ||
      currentMenu == HF_BARRIER_BRUTE_NICE ||

      currentMenu == IR_SCAN ||
      currentMenu == IR_REPLAY ||
      currentMenu == IR_BRUTE_TV ||
      currentMenu == IR_BRUTE_PROJECTOR ||

      currentMenu == UHF_SPECTRUM ||
      currentMenu == UHF_ALL_JAMMER ||
      currentMenu == UHF_WIFI_JAMMER ||
      currentMenu == UHF_BT_JAMMER ||
      currentMenu == UHF_BLE_JAMMER ||
      currentMenu == UHF_USB_JAMMER ||
      currentMenu == UHF_VIDEO_JAMMER ||
      currentMenu == UHF_RC_JAMMER ||
      currentMenu == UHF_BLE_SPAM ||

      currentMenu == TELEMETRY ||

      currentMenu == RFID_SCAN ||
      currentMenu == RFID_EMULATE ||
      currentMenu == RFID_WRITE);
}

/*============================= MAIN APPEARANCE ============================================*/
const static uint8_t radioConnectedIcon[7] PROGMEM = {
    0x60, 0x00, 0x70, 0x00, 0x7c, 0x00, 0x7f};

void drawRadioConnected()
{
  oled.setCursorXY(115, 0);
  for (uint8_t i = 0; i < 7; i++)
  {
    oled.drawByte(pgm_read_byte(&(radioConnectedIcon[i])));
  }
}

void drawBattery(float batVoltage, const char *suffix = "")
{
  int16_t percentage = round((batVoltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0);
  if (percentage < 0)
    percentage = 0;
  if (percentage > 100)
    percentage = 100;

  char voltageText[10];
  oled.setCursorXY(4, 0);
  if (settings.showPercent)
  {
    sprintf(voltageText, "%u", percentage);
    oled.setScale(1);
    oled.print(voltageText);

    oled.setCursorXY(5 + getTextWidth(voltageText), 0);
    oled.print("%");
    oled.print(suffix);
  }
  else
  {
    sprintf(voltageText, "%.2f", batVoltage);
    oled.setScale(1);
    oled.print(voltageText);

    oled.setCursorXY(5 + getTextWidth(voltageText), 0);
    oled.print("V");
    oled.print(suffix);
  }
}

void drawDashedLine(int y, int startX, int endX, int dashLength = 3, int gapLength = 3)
{
  int x = startX;
  while (x < endX)
  {
    for (int i = 0; i < dashLength && (x + i) < endX; i++)
    {
      oled.dot(x + i, y);
    }
    x += dashLength + gapLength;
  }
}

void drawMenu(const char *items[], uint8_t itemCount, uint8_t selectedIndex)
{
  oled.textMode(BUF_ADD);

  if (selectedIndex == 0)
  {
    {
      const char *text = items[0];
      int textWidth = strlen(text) * 12;
      int textX = (128 - textWidth) / 2;
      int textY = 24;

      int arrowX = textX - 10;
      int arrowY = textY + 6;

      oled.line(arrowX, arrowY - 5, arrowX + 5, arrowY);
      oled.line(arrowX + 5, arrowY, arrowX, arrowY + 5);
      oled.line(arrowX, arrowY - 4, arrowX + 4, arrowY);
      oled.line(arrowX + 4, arrowY, arrowX, arrowY + 4);

      oled.setScale(2);
      oled.setCursorXY(textX, textY);
      oled.print(text);
    }

    {
      const char *text = items[1];
      int textWidth = strlen(text) * 6;
      oled.setScale(1);
      oled.setCursorXY((128 - textWidth) / 2, 50);
      oled.print(text);
    }
  }

  else if (selectedIndex == itemCount - 1)
  {
    {
      const char *text = items[itemCount - 2];
      int textWidth = strlen(text) * 6;
      oled.setScale(1);
      oled.setCursorXY((128 - textWidth) / 2, 6);
      oled.print(text);
    }

    {
      const char *text = items[itemCount - 1];
      int textWidth = strlen(text) * 12;
      int textX = (128 - textWidth) / 2;
      int textY = 24;

      int arrowX = textX - 10;
      int arrowY = textY + 6;

      oled.line(arrowX, arrowY - 5, arrowX + 5, arrowY);
      oled.line(arrowX + 5, arrowY, arrowX, arrowY + 5);
      oled.line(arrowX, arrowY - 4, arrowX + 4, arrowY);
      oled.line(arrowX + 4, arrowY, arrowX, arrowY + 4);

      oled.setScale(2);
      oled.setCursorXY(textX, textY);
      oled.print(text);
    }
  }

  else
  {
    {
      const char *text = items[selectedIndex - 1];
      int textWidth = strlen(text) * 6;
      oled.setScale(1);
      oled.setCursorXY((128 - textWidth) / 2, 6);
      oled.print(text);
    }

    {
      const char *text = items[selectedIndex];
      int textWidth = strlen(text) * 12;
      int textX = (128 - textWidth) / 2;
      int textY = 24;

      int arrowX = textX - 10;
      int arrowY = textY + 6;

      oled.line(arrowX, arrowY - 5, arrowX + 5, arrowY);
      oled.line(arrowX + 5, arrowY, arrowX, arrowY + 5);
      oled.line(arrowX, arrowY - 4, arrowX + 4, arrowY);
      oled.line(arrowX + 4, arrowY, arrowX, arrowY + 4);

      oled.setScale(2);
      oled.setCursorXY(textX, textY);
      oled.print(text);
    }

    {
      const char *text = items[selectedIndex + 1];
      int textWidth = strlen(text) * 6;
      oled.setScale(1);
      oled.setCursorXY((128 - textWidth) / 2, 50);
      oled.print(text);
    }
  }
}

void ShowSplashScreen()
{
  oled.clear();
  oled.textMode(BUF_ADD);

  oled.setScale(3);
  int nameWidth = getTextWidth(APP_NAME) * 3;
  oled.setCursorXY((128 - nameWidth) / 2, 20);
  oled.print(APP_NAME);

  oled.setScale(1);
  int verWidth = getTextWidth(APP_VERSION);
  oled.setCursorXY((128 - verWidth) / 2, 55);
  oled.print(APP_VERSION);

  oled.update();
}

void showClearConfirmation(const char *label)
{
  oled.clear();
  oled.setScale(2);

  oled.setCursorXY((128 - 2 * getTextWidth(label)) / 2, 10);
  oled.print(label);

  oled.setCursorXY((128 - 2 * getTextWidth("slots")) / 2, 25);
  oled.print("slots");

  oled.setCursorXY((128 - 2 * getTextWidth("cleared")) / 2, 40);
  oled.print("cleared");

  oled.update();
}

void showGamePaused()
{
  oled.setScale(2);
  oled.setCursorXY((128 - getTextWidth("PAUSED") * 2) / 2, 28);
  oled.print("PAUSED");
}

void setMinBrightness()
{
  if (millis() - brightnessTimer > BRIGHTNESS_TIME)
  {
    brightnessTimer = millis();
    oled.setContrast(MIN_BRIGHTNESS);
    isScreenDimmed = true;
  }
}

void drawSettingsMenu(uint8_t selectedIndex)
{
  bool values[] = {settings.saveIR, settings.saveRA, settings.vibroOn, settings.activeScan, settings.showPercent};

  const uint8_t itemCount = sizeof(settingsItems) / sizeof(settingsItems[0]);
  const uint8_t itemCountAdd = sizeof(settingsItemsAdditional) / sizeof(settingsItemsAdditional[0]);
  const uint8_t totalCount = itemCount + itemCountAdd;

  const uint8_t SCREEN_W = 128;
  const uint8_t SCREEN_H = 64;
  const uint8_t ROW_H = 10;
  const uint8_t FIRST_Y = 10;
  const uint8_t VISIBLE_ROWS = 5;

  static uint8_t topIndex = 0;
  if (selectedIndex < topIndex)
  {
    topIndex = selectedIndex;
  }
  else if (selectedIndex > topIndex + VISIBLE_ROWS - 1)
  {
    topIndex = selectedIndex - (VISIBLE_ROWS - 1);
  }
  if (totalCount > VISIBLE_ROWS && topIndex > totalCount - VISIBLE_ROWS)
  {
    topIndex = totalCount - VISIBLE_ROWS;
  }

  oled.textMode(BUF_ADD);
  oled.setScale(1);

  if (topIndex > 0)
  {
    oled.setCursorXY(SCREEN_W - 6, 0);
    oled.print("^");
  }
  if (topIndex + VISIBLE_ROWS < totalCount)
  {
    oled.setCursorXY(SCREEN_W - 6, SCREEN_H - 8);
    oled.print("v");
  }

  const uint8_t last = min<uint8_t>(totalCount, topIndex + VISIBLE_ROWS);
  for (uint8_t global = topIndex; global < last; global++)
  {
    const bool selected = (global == selectedIndex);
    if (selected)
      oled.invertText(true);

    const uint8_t row = global - topIndex;
    const uint8_t y = FIRST_Y + row * ROW_H;

    if (global < itemCount)
    {
      oled.setCursorXY(0, y);
      oled.print(settingsItems[global]);

      oled.setCursorXY(SCREEN_W - 24, y);
      if (global == 4)
      {
        oled.print(values[global] ? "%" : "V");
      }
      else
      {
        oled.print(values[global] ? "ON" : "OFF");
      }
    }
    else
    {
      const uint8_t j = global - itemCount;
      int tw = getTextWidth(settingsItemsAdditional[j]);
      oled.setCursorXY((SCREEN_W - tw) / 2, y);
      oled.print(settingsItemsAdditional[j]);
    }

    if (selected)
      oled.invertText(false);
  }
}

void showLock()
{
  oled.setCursorXY(98, 0);
  oled.print("L");
}

// void showPortableInited()
// {
//   oled.setCursorXY(108, 0);
//   oled.print("I");
// }

void showCharging()
{
  if (millis() - chargingAnimTimer >= CHARGING_ANIM_INTERVAL)
  {
    chargingAnimTimer = millis();
    chargingAnimFrame = (chargingAnimFrame + 1) % CHARGING_ANIM_FRAMES;
  }

  const uint8_t bx = 93;
  const uint8_t by = 1;
  const uint8_t bw = 9;  
  const uint8_t bh = 5;  

  oled.rect(bx, by, bx + bw, by + bh, OLED_STROKE);
  oled.rect(bx + bw + 1, by + 1, bx + bw + 2, by + bh - 1, OLED_FILL);

  uint8_t fillCols = 0;
  switch (chargingAnimFrame)
  {
  case 0:
    fillCols = 0;
    break;
  case 1:
    fillCols = 3;
    break;
  case 2:
    fillCols = 5;
    break;
  case 3:
    fillCols = 8;
    break;
  }

  if (fillCols > 0)
  {
    oled.rect(bx + 1, by + 1, bx + fillCols, by + bh - 1, OLED_FILL);
  }
}

void ShowReboot()
{
  oled.clear();
  oled.setScale(2);

  oled.setCursorXY((128 - 2 * getTextWidth("Please,")) / 2, 10);
  oled.print("Please,");

  oled.setCursorXY((128 - 2 * getTextWidth("reboot")) / 2, 25);
  oled.print("reboot");

  oled.setCursorXY((128 - 2 * getTextWidth("the device")) / 2, 40);
  oled.print("the device");

  oled.update();
}

/*============================= HF VISUALIZATION ============================================*/

void ShowScanning_HF()
{
  char Text[20];
  sprintf(Text, "Listening...");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text1[30];
  sprintf(Text1, "Frequency: %.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 29);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Click < > to set freq");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 42);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 55);
  oled.print(Text3);
}

void ShowMonitor_HF()
{
  oled.setScale(1);

  // Header: current frequency
  char header[16];
  sprintf(header, "%.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setCursorXY(0, 0);
  oled.print(header);

  // Empty state
  if (hfMonitorCount == 0)
  {
    const char *txt = "Listening...";
    oled.setCursorXY((128 - getTextWidth(txt)) / 2, 28);
    oled.print(txt);

    for (int x = 20; x < 108; x++)
    {
      int y = 48 + sin((x + sineOffset) / 10.0f) * 6;
      oled.dot(x, y);
    }
    for (int x = 20; x < 108; x++)
    {
      int y = 48 + sin((x + sineOffset + 15) / 12.0f) * 6;
      oled.dot(x, y);
    }
    sineOffset += 2;
    return;
  }

  // Auto-follow newest entry
  if (hfMonitorAutoFollow)
  {
    hfMonitorCursorIndex = hfMonitorCount - 1;
  }

  // Auto-scroll to keep cursor visible
  if (hfMonitorCursorIndex < hfMonitorTopIndex)
  {
    hfMonitorTopIndex = hfMonitorCursorIndex;
  }
  else if (hfMonitorCursorIndex >= hfMonitorTopIndex + HF_MONITOR_VISIBLE_LINES)
  {
    hfMonitorTopIndex = hfMonitorCursorIndex - HF_MONITOR_VISIBLE_LINES + 1;
  }

  // Scroll up arrow
  if (hfMonitorTopIndex > 0)
  {
    oled.line(116, 15, 119, 12);
    oled.line(119, 12, 122, 15);
  }

  // Entries
  for (uint8_t row = 0; row < HF_MONITOR_VISIBLE_LINES; row++)
  {
    uint8_t logicalIndex = hfMonitorTopIndex + row;
    if (logicalIndex >= hfMonitorCount)
      break;

    uint8_t ringIndex = (hfMonitorHead + logicalIndex) % MAX_HF_MONITOR_SIGNALS;
    HFMonitorEntry entry = hfMonitorEntries[ringIndex];

    uint8_t y = 11 + row * 9;

    // Cursor indicator
    if (logicalIndex == hfMonitorCursorIndex)
    {
      oled.setCursorXY(0, y);
      oled.print(">");
    }

    // Code
    char codeBuf[12];
    if (entry.codeValid)
      sprintf(codeBuf, "%lu", entry.code);
    else
      strcpy(codeBuf, "----");
    oled.setCursorXY(8, y);
    oled.print(codeBuf);

    // RSSI mini bar
    int8_t rssiVal = entry.rssi;
    if (rssiVal < -99)
      rssiVal = -99;
    uint8_t barMax = 20;
    uint8_t barW = constrain(map(rssiVal, -100, -20, 0, barMax), 0, barMax);
    oled.rect(76, y + 2, 76 + barMax, y + 5, OLED_STROKE);
    if (barW > 0)
    {
      oled.rect(76, y + 2, 76 + barW, y + 5, OLED_FILL);
    }

    // Protocol
    if (entry.protocolValid)
    {
      uint8_t pd = (entry.protocol > 99) ? 99 : entry.protocol;
      char pBuf[4];
      sprintf(pBuf, "P%u", pd);
      oled.setCursorXY(100, y);
      oled.print(pBuf);
    }
  }

  // Scroll down arrow
  uint8_t maxTop = 0;
  if (hfMonitorCount > HF_MONITOR_VISIBLE_LINES)
    maxTop = hfMonitorCount - HF_MONITOR_VISIBLE_LINES;
  if (hfMonitorTopIndex < maxTop)
  {
    oled.line(116, 58, 119, 61);
    oled.line(119, 61, 122, 58);
  }

  // Scrollbar
  if (hfMonitorCount > HF_MONITOR_VISIBLE_LINES)
  {
    uint8_t scrollAreaTop = 17;
    uint8_t scrollAreaBottom = 57;
    uint8_t trackHeight = scrollAreaBottom - scrollAreaTop;
    uint8_t thumbHeight = max(4, (HF_MONITOR_VISIBLE_LINES * trackHeight) / hfMonitorCount);
    uint8_t scrollRange = trackHeight - thumbHeight;
    uint8_t thumbY = scrollAreaTop + (scrollRange * hfMonitorTopIndex) / maxTop;

    oled.rect(118, thumbY, 120, thumbY + thumbHeight, OLED_FILL);
  }
}

void ShowJamming_HF()
{
  char Text1[20];
  sprintf(Text1, "HF Jammer: %.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 16);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 30);
  oled.print(Text2);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 8;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)12) * 8;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void ShowCapturedSignal_HF()
{
  char Text[25];
  sprintf(Text, "RF Signal: %d dBm", currentRssi);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text2[20];
  sprintf(Text2, "Code: %d", capturedCode);
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Length: %d Bit", capturedLength);
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
  oled.print(Text3);

  char Text4[20];
  sprintf(Text4, "Protocol: %d", capturedProtocol);
  oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
  oled.print(Text4);

  char Text5[20];
  sprintf(Text5, "Delay: %d ms", capturedDelay);
  oled.setCursorXY((128 - getTextWidth(Text5)) / 2, 55);
  oled.print(Text5);
}

void ShowSavedSignal_HF()
{
  char Text[25];
  sprintf(Text, "Saved RF, slot: %d", selectedSlotRA);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text1[50];
  sprintf(Text1, "Name: %s", slotName);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 25);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Code: %d", capturedCode);
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 35);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Length: %d Bit", capturedLength);
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 45);
  oled.print(Text3);

  char Text4[20];
  sprintf(Text4, "Protocol: %d", capturedProtocol);
  oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 55);
  oled.print(Text4);
}

void ShowSendingTesla_HF()
{
  char Text[20];
  sprintf(Text, "Tesla Charge Door");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text1[20];
  sprintf(Text1, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 30);
  oled.print(Text1);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 8;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)12) * 8;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void ShowAttack_HF()
{
  char Text[20];
  sprintf(Text, "Freq: %.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text2[20];
  sprintf(Text2, "Click OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 30);
  oled.print(Text2);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)8) * 10;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 10;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void DrawRSSIPlot_HF()
{
  static unsigned long lastBlink = 0;
  static bool visible = false;

  const char *label = "RSSI";

  char freq[10];
  sprintf(freq, "%.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setScale(1);
  oled.setCursorXY(0, 0);
  oled.print(freq);

  for (int i = 0; i < RSSI_BUFFER_SIZE; i++)
  {
    int x = i + 25;
    int rssiValue = rssiBuffer[(rssiIndex + i) % RSSI_BUFFER_SIZE];
    uint8_t h = constrain(map(rssiValue, -100, -20, 0, 50), 0, 50);
    oled.line(x, 63, x, 63 - h, 1);
  }

  for (int i = 0; label[i] != '\0'; i++)
  {
    oled.setCursorXY(120, 23 + (i * 8));
    char ch[2] = {label[i], '\0'};
    oled.print(ch);
  }

  oled.setCursorXY(0, 55);
  oled.print("90 -");
  oled.setCursorXY(0, 35);
  oled.print("60 -");
  oled.setCursorXY(0, 15);
  oled.print("30 -");

  drawDashedLine(58, 25, 115, 1, 5);
  drawDashedLine(38, 25, 115, 1, 5);
  drawDashedLine(18, 25, 115, 1, 5);

  char Text[10];
  int cursorStart = successfullyConnected ? 85 : 102;
  sprintf(Text, "%d", findMaxValue(rssiBuffer, RSSI_BUFFER_SIZE));
  oled.setCursorXY(cursorStart - getTextWidth(Text), 0);
  oled.print(Text);
  oled.setCursorXY(cursorStart, 0);
  oled.print(" dBm");

  if ((long)(signalIndicatorUntil - millis()) > 0)
  {
    if (millis() - lastBlink > 200)
    {
      lastBlink = millis();
      visible = !visible;
    }
    if (visible)
      oled.circle(122, 60, 2, 1);
  }
  else
  {
    visible = false;
  }
}

void DrawRSSISpectrum_HF()
{
  const char *label = "RSSI";

  for (uint8_t i = 0; i < raFreqCount; i++)
  {
    int x = 20 + i * 25;

    uint8_t peakY = constrain(mapFloat(rssiMaxPeak[i], -100, -20, 0, 50), 0, 50);
    oled.rect(x, 63 - peakY, x + 18, 63, OLED_FILL);

    uint8_t absPeakY = constrain(mapFloat(rssiAbsoluteMax[i], -100, -20, 0, 50), 0, 50);
    oled.line(x, 63 - absPeakY, x + 18, 63 - absPeakY, 1);
  }

  for (int i = 0; label[i] != '\0'; i++)
  {
    oled.setCursorXY(120, 25 + (i * 8));
    char ch[2] = {label[i], '\0'};
    oled.print(ch);
  }

  oled.setCursorXY(0, 55);
  oled.print("90 -");
  oled.setCursorXY(0, 35);
  oled.print("60 -");
  oled.setCursorXY(0, 15);
  oled.print("30 -");

  drawDashedLine(58, 25, 115, 1, 5);
  drawDashedLine(38, 25, 115, 1, 5);
  drawDashedLine(18, 25, 115, 1, 5);

  char Text[25];
  sprintf(Text, "%.0f dBm", findMaxValue(rssiAbsoluteMax, raFreqCount));
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2 + 6, 0);
  oled.print(Text);
}

void DrawRAWReplay()
{
  oled.textMode(BUF_ADD);
  oled.setScale(1);

  oled.setCursorXY(35, 0);
  oled.print("RAW Replay");

  const uint8_t ROW_H = 11;
  const uint8_t FIRST_Y = 12;
  const uint8_t VISIBLE = min((uint8_t)5, (uint8_t)MAX_RAW_SIGNALS);

  static uint8_t topIndex = 0;
  if (selectedSlotRAW < topIndex)
    topIndex = selectedSlotRAW;
  else if (selectedSlotRAW >= topIndex + VISIBLE)
    topIndex = selectedSlotRAW - VISIBLE + 1;

  if (topIndex > 0)
  {
    oled.setCursorXY(122, FIRST_Y);
    oled.print("^");
  }
  if (topIndex + VISIBLE < MAX_RAW_SIGNALS)
  {
    oled.setCursorXY(122, FIRST_Y + (VISIBLE - 1) * ROW_H);
    oled.print("v");
  }

  for (uint8_t row = 0; row < VISIBLE; row++)
  {
    uint8_t i = topIndex + row;
    if (i >= MAX_RAW_SIGNALS)
      break;

    uint8_t y = FIRST_Y + row * ROW_H;
    bool occupied = isRawSlotOccupied(i);
    bool selected = (i == selectedSlotRAW);

    if (selected)
    {
      oled.setCursorXY(0, y);
      oled.print(">");
    }

    if (occupied)
    {
      char name[NAME_MAX_LEN + 1];
      readRawSlotName(i, name);
      int nx = (128 - getTextWidth(name)) / 2;
      oled.setCursorXY(nx, y);
      oled.print(name);

      if (selected && (long)(rawSendIndicatorUntil - millis()) > 0)
      {
        oled.circle(nx + getTextWidth(name) + 5, y + 3, 2, OLED_FILL);
      }
    }
    else
    {
      const char *empty = "Empty";
      oled.setCursorXY((128 - getTextWidth(empty)) / 2, y);
      oled.print(empty);
    }
  }
}

void DrawRAWOscillogram_HF()
{
  const uint8_t headerH = 9;
  const uint8_t plotTop = headerH + 2;
  const uint8_t plotBottom = 60;
  const uint8_t plotH = plotBottom - plotTop;
  const uint8_t centerY = plotTop + plotH / 2;
  const uint8_t halfH = plotH / 2 - 1;
  const uint8_t footerY = 56;

  oled.textMode(BUF_ADD);
  oled.setScale(1);

  char freq[16];
  sprintf(freq, "%.2f MHz", raFrequencies[currentFreqIndex]);
  oled.setCursorXY(0, 0);
  oled.print(freq);

  if (rawCapturing)
  {
    static unsigned long recBlink = 0;
    static bool recDot = true;
    if (millis() - recBlink > 400)
    {
      recBlink = millis();
      recDot = !recDot;
    }
    const char *rec = "REC";
    int recX = 128 - getTextWidth(rec);
    if (recDot)
      oled.circle(recX - 5, 3, 2, 1);
    oled.setCursorXY(recX, 0);
    oled.print(rec);
  }
  else
  {
    const char *st = rawRecorded ? "DONE" : "IDLE";
    oled.setCursorXY(128 - getTextWidth(st), 0);
    oled.print(st);
  }

  if (rawCapturing && rawSignalCount >= 2)
  {
    char sc[12];
    sprintf(sc, "S:%d", rawSignalCount);
    oled.setCursorXY(68, 0);
    oled.print(sc);
  }

  if (!rawRecorded)
  {
    const uint8_t axisX = 0;
    const uint8_t axisEndX = 127;

    oled.line(axisX, plotTop, axisX, plotBottom);
    oled.line(axisX, plotBottom, axisEndX, plotBottom);

    oled.line(axisX, plotTop, axisX + 2, plotTop);
    oled.line(axisX, centerY, axisX + 2, centerY);

    for (uint8_t tx = 25; tx < 127; tx += 25)
      oled.line(tx, plotBottom, tx, plotBottom - 2);

    for (uint8_t x = 3; x < 127; x += 3)
      oled.dot(x, centerY);

    if (rawCapturing)
    {
      const uint8_t plotW = 126;
      int prevH = 0;

      for (int px = 0; px < plotW; px++)
      {
        int bufIdx = (int)((long)px * RSSI_BUFFER_SIZE / plotW);
        if (bufIdx >= RSSI_BUFFER_SIZE)
          bufIdx = RSSI_BUFFER_SIZE - 1;

        int rssiValue = rssiBuffer[(rssiIndex + bufIdx) % RSSI_BUFFER_SIZE];
        int h = constrain(map(rssiValue, -100, -20, 0, halfH), 0, halfH);
        int x = 1 + px;

        if (h > 0)
        {
          oled.line(x, centerY - h, x, centerY - 1);
          oled.line(x, centerY + 1, x, centerY + h);
        }

        if (px > 0)
        {
          int ph = (prevH == 0) ? 0 : prevH;
          int ch = (h == 0) ? 0 : h;
          if (ph > 0 || ch > 0)
          {
            oled.line(x - 1, centerY - ph, x, centerY - ch);
            oled.line(x - 1, centerY + ph, x, centerY + ch);
          }
        }

        prevH = h;
      }
    }
  }
  else
  {
    oled.setScale(2);
    const char *msg = "Captured!";
    oled.setCursorXY((128 - getTextWidth(msg) * 2) / 2, 16);
    oled.print(msg);
    oled.setScale(1);

    char info[20];
    sprintf(info, "%d samples", rawRecordedCount);
    oled.setCursorXY((128 - getTextWidth(info)) / 2, 36);
    oled.print(info);

    oled.setCursorXY(2, footerY);
    oled.print("<< SAVE");

    const char *decl = "DROP >>";
    oled.setCursorXY(128 - getTextWidth(decl) - 2, footerY);
    oled.print(decl);
  }
}

char nextChar(char c)
{
  uint8_t n = sizeof(nameChars) - 1;
  uint8_t idx = 0;
  bool found = false;

  for (uint8_t i = 0; i < n; i++)
  {
    if (nameChars[i] == c)
    {
      idx = i;
      found = true;
      break;
    }
  }

  if (!found)
    idx = 0;

  idx = (idx + 1) % n;
  return nameChars[idx];
}

char prevChar(char c)
{
  uint8_t n = sizeof(nameChars) - 1;
  uint8_t idx = 0;
  bool found = false;

  for (uint8_t i = 0; i < n; i++)
  {
    if (nameChars[i] == c)
    {
      idx = i;
      found = true;
      break;
    }
  }

  if (!found)
    idx = 0;

  idx = (idx == 0) ? (n - 1) : (idx - 1);
  return nameChars[idx];
}

void ShowRANameEdit()
{
  oled.clear();
  oled.textMode(BUF_ADD);

  const char *title = "Edit name";
  oled.setScale(1);
  int titleW = getTextWidth(title);
  oled.setCursorXY((128 - titleW) / 2, 12);
  oled.print(title);

  oled.setScale(2);
  const int charW = 6 * 2;
  const int totalWidth = NAME_MAX_LEN * charW;
  const int startX = (128 - totalWidth) / 2;
  const int y = 30;

  for (uint8_t i = 0; i < NAME_MAX_LEN; i++)
  {
    char c = slotName[i];
    if (c == '\0')
      c = ' ';

    int x = startX + i * charW;

    if (i == RANamePos)
    {
      oled.rect(x - 1, y - 2, x + charW - 2, y + 16, OLED_STROKE);
    }

    char buf[2] = {c, '\0'};
    oled.setCursorXY(x, y);
    oled.print(buf);
  }
}

void ShowRAMenu()
{
  oled.textMode(BUF_ADD);

  for (uint8_t i = 0; i < 2; i++)
  {
    const char *txt = hfReplayMenuItems[i];

    oled.setScale(2);
    int w = getTextWidth(txt) * 2;
    int y = 20 + i * 20;
    int x = (128 - w) / 2;

    if (i == RAMenuIndex)
    {
      int ax = x - 8;
      int ay = y + 6;

      oled.line(ax, ay - 4, ax + 4, ay);
      oled.line(ax + 4, ay, ax, ay + 4);
      oled.line(ax, ay - 3, ax + 3, ay);
      oled.line(ax + 3, ay, ax, ay + 3);
    }

    oled.setCursorXY(x, y);
    oled.print(txt);
  }
}

/*============================= HF BARRIERS VISUALIZATION ============================================*/

void ShowCapturedBarrier_HF()
{
  if (barrierProtocol == 0)
  {
    char Text[25];
    sprintf(Text, "RF Signal: %d dBm", currentRssi);
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
    oled.print(Text);

    char Text2[20];
    sprintf(Text2, "Code1: 0x%04X", barrierCodeMain);
    oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
    oled.print(Text2);

    char Text3[20];
    sprintf(Text3, "Code2: 0x%04X", barrierCodeAdd);
    oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
    oled.print(Text3);

    char Text4[20];
    sprintf(Text4, "Protocol: %s", protocols[barrierProtocol]);
    oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
    oled.print(Text4);

    char Text5[20];
    sprintf(Text5, "Length: %d Bit", barrierBit);
    oled.setCursorXY((128 - getTextWidth(Text5)) / 2, 55);
    oled.print(Text5);
  }
  else
  {
    char Text[25];
    sprintf(Text, "RF Signal: %d dBm", currentRssi);
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
    oled.print(Text);

    char Text2[20];
    sprintf(Text2, "Code: %d", barrierCodeMain);
    oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
    oled.print(Text2);

    char Text4[20];
    sprintf(Text4, "Protocol: %s", protocols[barrierProtocol]);
    oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 35);
    oled.print(Text4);

    char Text5[20];
    sprintf(Text5, "Length: %d Bit", barrierBit);
    oled.setCursorXY((128 - getTextWidth(Text5)) / 2, 45);
    oled.print(Text5);
  }
}

void ShowSavedSignalBarrier_HF()
{
  const char *protocol_ = protocols[barrierProtocol];

  if (barrierCodeMain == 0)
  {
    protocol_ = "None";
    barrierBit = 0;
  }
  else
  {
    barrierBit = 65;
  }

  if (barrierProtocol == 0)
  {
    char Text[25];
    sprintf(Text, "Saved RF, slot: %d", selectedSlotBarrier);
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
    oled.print(Text);

    char Text2[20];
    sprintf(Text2, "Code1: 0x%04X", barrierCodeMain);
    oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
    oled.print(Text2);

    char Text3[20];
    sprintf(Text3, "Code2: 0x%04X", barrierCodeAdd);
    oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
    oled.print(Text3);

    char Text4[20];
    sprintf(Text4, "Protocol: %s", protocol_);
    oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
    oled.print(Text4);

    char Text5[20];
    sprintf(Text5, "Length: %d Bit", barrierBit);
    oled.setCursorXY((128 - getTextWidth(Text5)) / 2, 55);
    oled.print(Text5);
  }
  else
  {
    barrierBit = (barrierCodeMain >> 12) ? 24 : 12;

    char Text[25];
    sprintf(Text, "Saved RF, slot: %d", selectedSlotBarrier);
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
    oled.print(Text);

    char Text2[20];
    sprintf(Text2, "Code: %d", barrierCodeMain);
    oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
    oled.print(Text2);

    char Text4[20];
    sprintf(Text4, "Protocol: %s", protocol_);
    oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 35);
    oled.print(Text4);

    char Text5[20];
    sprintf(Text5, "Length: %d Bit", barrierBit);
    oled.setCursorXY((128 - getTextWidth(Text5)) / 2, 45);
    oled.print(Text5);
  }
}

void ShowBarrierBrute_HF(uint8_t protocol)
{
  char Text1[20];
  sprintf(Text1, "Protocol: %s", protocols[protocol]);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 25);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Command: %d", barrierBruteIndex);
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 35);
  oled.print(Text2);
}

/*============================= UHF VISUALIZATION ============================================*/

void ShowJamming_UHF()
{
  char Text1[20];
  sprintf(Text1, "UHF Jammer: %d ch", radioChannel);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 16);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 30);
  oled.print(Text2);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 8;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)12) * 8;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void ShowBLESpam_UHF()
{
  char Text[20];
  sprintf(Text, "BLE Spam");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text1[20];
  sprintf(Text1, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 30);
  oled.print(Text1);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 8;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)12) * 8;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void DrawSpectrum_UHF()
{
  oled.textMode(BUF_ADD);

  if (!locked && ok.click())
  {
    peaksDynamic = !peaksDynamic;
    vibro(250, 100);
  }

  if (!peaksDynamic)
  {
    oled.setCursorXY((SCREEN_WIDTH - 3 * 6) / 2, SPECTRUM_TOP_LIMIT - 8);
    oled.setScale(1);
    oled.print("MAX");
  }

  for (uint8_t channel = 0; channel < NUM_CHANNELS; ++channel)
  {
    uint8_t x = (barWidth * channel) + margin;
    oled.line(x, SPECTRUM_TOP_LIMIT, x, SCREEN_BOTTOM, 0);

    uint8_t cacheSum = channelStrength[channel];

    if (stored[channel].maxPeak > cacheSum * 2)
    {
      uint8_t peakHeight = SCREEN_BOTTOM - ((SCREEN_BOTTOM - SPECTRUM_TOP_LIMIT) * stored[channel].maxPeak) / (cacheMax * 2);
      oled.line(x, peakHeight, x, peakHeight, 1);

      if (peaksDynamic && stored[channel].maxPeak > 0)
      {
        stored[channel].maxPeak--;
      }
    }

    if (cacheSum > 0)
    {
      uint8_t barHeight = ((SCREEN_BOTTOM - SPECTRUM_TOP_LIMIT) * cacheSum) / cacheMax;
      oled.line(x, SCREEN_BOTTOM, x, SCREEN_BOTTOM - barHeight, 1);
    }
  }
}

/*============================= IR VISUALIZATION ============================================*/

void ShowScanning_IR()
{
  char Text[20];
  sprintf(Text, "Listening...");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text1[30];
  sprintf(Text1, "Type: InfraRed");
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 30);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 44);
  oled.print(Text2);
}

void ShowCapturedSignal_IR()
{
  char Text[20];
  sprintf(Text, "IR Signal captured");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text2[30];
  sprintf(Text2, "Protocol: %s", getProtocolString((decode_type_t)protocol));
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Address: 0x%04X", address);
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
  oled.print(Text3);

  char Text4[20];
  sprintf(Text4, "Command: 0x%04X", command);
  oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
  oled.print(Text4);
}

void ShowSavedSignal_IR()
{
  char Text[20];
  sprintf(Text, "Saved IR, slot: %d", selectedSlotIR);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text2[30];
  sprintf(Text2, "Protocol: %s", getProtocolString((decode_type_t)protocol));
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Address: 0x%04X", address);
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
  oled.print(Text3);

  char Text4[20];
  sprintf(Text4, "Command: 0x%04X", command);
  oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
  oled.print(Text4);
}

void ShowBrute_IR()
{
  char Text[20];
  sprintf(Text, "Index: %d", currentIndex);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text2[30];
  sprintf(Text2, "Protocol: %s", getProtocolString((decode_type_t)protocol));
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 25);
  oled.print(Text2);

  char Text3[20];
  sprintf(Text3, "Address: 0x%04X", address);
  oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 35);
  oled.print(Text3);

  char Text4[20];
  sprintf(Text4, "Command: 0x%04X", command);
  oled.setCursorXY((128 - getTextWidth(Text4)) / 2, 45);
  oled.print(Text4);
}

/*============================= RFID VISUALIZATION ============================================*/

void ShowEmulation_RFID()
{
  char Text[20];
  sprintf(Text, "RFID Freq: 125 kHz");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 16);
  oled.print(Text);

  char Text2[20];
  sprintf(Text2, "Click OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 30);
  oled.print(Text2);

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset + 12) / (float)8) * 10;
    oled.dot(x, y);
  }

  for (int x = 10; x < 118; x++)
  {
    int y = 50 + sin((x + sineOffset) / (float)10) * 10;
    oled.dot(x, y);
  }

  sineOffset += 2;
}

void ShowScanning_RFID()
{
  char Text[20];
  sprintf(Text, "Place RFID tag near");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text1[30];
  sprintf(Text1, "125 kHz/13.56 MHz");
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 30);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "Hold OK to stop");
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 45);
  oled.print(Text2);
}

void ShowCapturedData_RFID()
{
  if (tagDetected == 1)
  {
    char Text[25];
    sprintf(Text, "Card detected");
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
    oled.print(Text);

    char Text1[25];
    sprintf(Text1, "Type: 125 kHz");
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 30);
    oled.print(Text1);

    char Text2[20];
    sprintf(Text2, "UID: %lX", tagID_125kHz);
    oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 45);
    oled.print(Text2);
  }
  else if (tagDetected == 2)
  {
    char Text[25];
    sprintf(Text, "Card detected");
    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text)) / 2, 10);
    oled.print(Text);

    char Text1[30];
    if (nfcCardType == 1)
      sprintf(Text1, "Type: Mifare Classic");
    else if (nfcCardType == 2)
      sprintf(Text1, "Type: Ultralight");
    else
      sprintf(Text1, "Type: Unknown");

    oled.setScale(1);
    oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 20);
    oled.print(Text1);

    if (nfcCardType == 2) // Ultralight
    {
      char Text2_line1[40] = "UID: ";
      char Text2_line2[40] = "";

      for (uint8_t i = 0; i < tagIDLength_NFC; i++)
      {
        char byteStr[3];
        sprintf(byteStr, "%02X ", tagID_NFC[i]);

        if (i < (tagIDLength_NFC + 1) / 2)
          strcat(Text2_line1, byteStr);
        else
          strcat(Text2_line2, byteStr);
      }

      oled.setCursorXY((128 - getTextWidth(Text2_line1)) / 2, 30);
      oled.print(Text2_line1);

      oled.setCursorXY((128 - getTextWidth(Text2_line2)) / 2, 40);
      oled.print(Text2_line2);
    }
    else // Classic
    {
      char Text2[50] = "UID: ";
      for (uint8_t i = 0; i < tagIDLength_NFC; i++)
      {
        char byteStr[4];
        sprintf(byteStr, "%02X ", tagID_NFC[i]);
        strcat(Text2, byteStr);
      }

      oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 30);
      oled.print(Text2);
    }

    if (nfcDataValid)
    {
      if (nfcCardType == 2) // Ultralight
      {
        char Text3[70] = "DATA: ";
        for (uint8_t i = 0; i < nfcDataLength; i++)
        {
          char byteStr[4];
          sprintf(byteStr, "%02X ", nfcData[i]);
          strcat(Text3, byteStr);
        }

        oled.setCursorXY((128 - getTextWidth(Text3)) / 2, 52);
        oled.print(Text3);
      }
      else // Classic
      {
        char Text3_line1[40] = "";
        char Text3_line2[40] = "";

        for (uint8_t i = 0; i < nfcDataLength; i++)
        {
          char byteStr[3];
          sprintf(byteStr, "%02X", nfcData[i]);

          if (i < nfcDataLength / 2)
            strcat(Text3_line1, byteStr);
          else
            strcat(Text3_line2, byteStr);
        }

        oled.setCursorXY((128 - getTextWidth(Text3_line1)) / 2, 40);
        oled.print(Text3_line1);

        oled.setCursorXY((128 - getTextWidth(Text3_line2)) / 2, 52);
        oled.print(Text3_line2);
      }
    }
  }
}

void ShowSavedSignal_RFID()
{
  char Text[25];
  sprintf(Text, "Saved card");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text)) / 2, 15);
  oled.print(Text);

  char Text0[25];
  sprintf(Text0, "Saved RF, slot: %d", selectedSlotRFID);
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text0)) / 2, 25);
  oled.print(Text0);

  char Text1[25];
  sprintf(Text1, "Type: 125 kHz");
  oled.setScale(1);
  oled.setCursorXY((128 - getTextWidth(Text1)) / 2, 35);
  oled.print(Text1);

  char Text2[20];
  sprintf(Text2, "UID: %lX", tagID_125kHz);
  oled.setCursorXY((128 - getTextWidth(Text2)) / 2, 45);
  oled.print(Text2);
}

/* ============================= CONNECTION ============================================ */
void showConnectionStatus(uint8_t menuIndex)
{
  oled.setScale(1);
  char buf[20];
  sprintf(buf, "Connect: %s", startConnection ? "ON" : "OFF");
  oled.setCursorXY((128 - getTextWidth(buf)) / 2, 25);
  oled.invertText(successfullyConnected && menuIndex == 0);
  oled.print(buf);

  const char *txt;
  if (!startConnection)
    txt = "Click OK to enable";
  else
    txt = successfullyConnected ? "Turn off module" : "Connecting...";

  oled.setCursorXY((128 - getTextWidth(txt)) / 2, 45);
  oled.invertText(successfullyConnected && menuIndex == 1);
  oled.print(txt);

  oled.invertText(false);
}

/* ============================= TELEMETRY ============================================ */
void ShowTelemetry()
{
  oled.setScale(1);

  uint16_t lost = (telemetrySent > telemetryReceived) ? (telemetrySent - telemetryReceived) : 0;
  uint8_t rate = (telemetrySent > 0) ? (uint8_t)((uint32_t)telemetryReceived * 100 / telemetrySent) : 0;
  uint16_t rttAvg = (telemetryReceived > 0) ? (uint16_t)(telemetryRttSum / telemetryReceived) : 0;
  uint32_t elapsed = (millis() - telemetryStartMs) / 1000;
  uint8_t pps = (elapsed > 0) ? (uint8_t)(telemetryReceived / elapsed) : 0;

  char buf[16];

  sprintf(buf, "Sent: %d", telemetrySent);
  oled.setCursorXY(0, 14);
  oled.print(buf);

  sprintf(buf, "Recv: %d", telemetryReceived);
  oled.setCursorXY(0, 28);
  oled.print(buf);

  sprintf(buf, "Lost: %d", lost);
  oled.setCursorXY(0, 42);
  oled.print(buf);

  sprintf(buf, "Rate: %d%%", rate);
  oled.setCursorXY(0, 56);
  oled.print(buf);

  sprintf(buf, "RTTa: %dms", rttAvg);
  oled.setCursorXY(65, 14);
  oled.print(buf);

  sprintf(buf, "RTTm: %dms", telemetryRttMax);
  oled.setCursorXY(65, 28);
  oled.print(buf);

  sprintf(buf, "Strk: %d", telemetryLossStreakMax);
  oled.setCursorXY(65, 42);
  oled.print(buf);

  sprintf(buf, "PPS: %d", pps);
  oled.setCursorXY(65, 56);
  oled.print(buf);
}

/* ============================= FM RADIO ============================================ */
void ShowFMFrequency()
{
  static const int X0 = 4, X1 = 124, Y = 58, Ys = 15;

  oled.setScale(1);
  int fm_w = getTextWidth("FM");
  oled.setCursorXY((128 - fm_w) / 2, 0);
  oled.print("FM");

  char txt[20];
  sprintf(txt, "%d.%02d", FrequencyFM / 100, FrequencyFM % 100);
  oled.setScale(3);
  int w = getTextWidth(txt) * 3;
  int nx = -10 + (128 - w) / 2, ny = 23;
  oled.setCursorXY(nx, ny);
  oled.print(txt);

  oled.setScale(1);
  oled.setCursorXY(nx + w + 2, ny + 2);
  oled.print("MHz");

  auto fx = [&](uint16_t f)
  {
    if (f < FM_FREQUENCY_MIN)
      f = FM_FREQUENCY_MIN;
    if (f > FM_FREQUENCY_MAX)
      f = FM_FREQUENCY_MAX;
    return X0 + (int)((long)(f - FM_FREQUENCY_MIN) * (X1 - X0) / (FM_FREQUENCY_MAX - FM_FREQUENCY_MIN));
  };

  for (uint16_t f = ((FM_FREQUENCY_MIN + 50) / 100) * 100; f <= FM_FREQUENCY_MAX; f += 100)
  {
    int x = fx(f);
    oled.line(x, Y - 5, x, Y - 2);
  }

  int cx = fx(FrequencyFM), ty = Y - 7;
  int hw = 2 + (int)(AlphaFM / 2);
  int top = ty;
  int bot = ty + 7;
  int left = cx - hw;
  int right = cx + hw;
  oled.rect(left, top, right, bot, 2);

  int8_t lvl = FmSoundLevel;
  if (lvl > 0)
    lvl = 0;
  if (lvl < -10)
    lvl = -10;
  const int N = 30;
  int on = ((10 + lvl) * N) / 10;
  for (int i = 0; i < on; i++)
  {
    int x = X0 + (int)((long)i * (X1 - X0) / (N - 1));
    oled.line(x, Ys - 3, x, Ys - 1);
  }

  if (FMblink)
  {
    if (millis() - FMblinkTimer < 500)
      oled.circle(nx + w + 5, ny + 16, 3, 1);
    else
      FMblink = false;
  }
}
