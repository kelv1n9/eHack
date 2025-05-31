#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 23
#define OLED_RESET 4
#define SCREEN_ADDRESS 0X3C

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup(void)
{
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println("Hello Cyborg!");

  nfc.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Dont Proceed, Loop Forever
  }
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  display.clearDisplay();
  display.setCursor(0, 0); // oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Found Reader PN532");
  display.print((versiondata >> 24) & 0xFF, HEX);

  display.setCursor(0, 8); // oled Display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Firmware ver. ");
  display.print((versiondata >> 16) & 0xFF, DEC);

  display.print(".");
  display.print((versiondata >> 8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");

  display.setCursor(0, 16); // oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Waiting for NFC Card");
  display.display();
}

void loop(void)
{
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success)
  {
    // Display some basic information about the card
    display.clearDisplay();
    Serial.println("Found an ISO14443A card");
    display.setCursor(0, 0); // oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("ISO14443A Bytes: ");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    // display.setCursor(0, 8); //oled Display
    // display.setTextSize(1);
    // display.setTextColor(WHITE);
    // display.print("UID Length:");
    display.setCursor(100, 0); // oled Display
    display.print(uidLength, DEC);
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    // Serial.println("");
    // display.setCursor(0, 8); //oled display
    // for (int i = 0; i < uidLength; i++)
    //{
    // display.print("");
    //  display.print(uid[i], HEX);
    //  display.print("");
    // };
    display.display();

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      display.clearDisplay();
      display.setCursor(0, 0); // oled display
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.print("Mifare Classic Card");
      display.setCursor(0, 8); // oled Display
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.print("UID Length:");
      display.setCursor(80, 8); // oled Display
      display.print(uidLength, DEC);
      display.setCursor(0, 16); // oled display
      for (int i = 0; i < uidLength; i++)
      {
        display.print("");
        display.print(uid[i], HEX);
        display.print(" ");
      }
      display.display();

      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];

        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        // data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
        // success = nfc.mifareclassic_WriteDataBlock (4, data);
        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);

        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");
          // display.print("Success! Block 4: ");
          // display.println("");
          // display.display();

          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Unable to Read");
          display.setCursor(0, 10);
          display.print("Whoops");
          display.display();
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Unable to Read");
        display.setCursor(0, 10);
        display.print("Whoops");
        display.display();
      }
    }

    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
      display.setCursor(0, 8);
      display.print("Mifare Ultralight");
      display.display();

      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage(4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");
        display.setCursor(0, 16);
        // display.print("");
        // display.println("Card UID:");
        for (int i = 0; i < uidLength; i++)
        {
          display.print("");
          display.print(uid[i], HEX);
          display.print(" ");
        }
        display.display();

        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Unable to Read");
        display.setCursor(0, 10);
        display.print("Something fucked up");
        display.display();
      }
    }
  }
}