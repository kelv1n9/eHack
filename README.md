![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-blue?logo=raspberry-pi)
![License](https://img.shields.io/badge/license-MIT-green)

# 🚀 eHack

![eHack](pics/eHack.PNG)

**eHack** is a versatile, all-in-one tool built on the **Raspberry Pi Pico**, designed for radio frequency analysis, penetration testing, and hardware research enthusiasts. The project integrates a wide array of tools for interacting with various wireless technologies, all housed in a compact form factor with a user-friendly OLED interface.

---

## 🛰️ Portable Module

The ***eHack Portable*** expands the reach of **eHack** beyond the main device.  
It is a lightweight RF companion that links wirelessly and can be fully managed through the interface of the primary unit.  
With it, you can launch experiments and carry out attacks remotely, bringing extra versatility to your toolkit.

More details about the portable module are available in its own repository: [eHack Portable](https://github.com/kelv1n9/eHack_Portable)

---

## 🤝 Contributor Wanted!

> For the **eHack** project, we are looking for a developer who can help with the software implementation for the **PN532 NFC module**. The goal is to add full tag reading, writing, and emulation capabilities.
>
> Your contribution to this feature would be highly appreciated! If you have the experience and a desire to contribute, please feel free to reach out.


| &nbsp; | Bands&nbsp;/&nbsp;Tech | What you can do |
|---|---|---|
| **Sub‑GHz (315 – 915 MHz)** | CC1101 | • Live spectrum and activity scan<br>• Capture & replay OOK/ASK packets <br>• **RAW** capture & replay<br>• **HF Monitor** (live log + quick resend)<br>• Gate / Barrier toolkit (capture, replay, brute‑force **CAME** & **NICE** codes)<br>• **Tesla** charge‑port opener<br>• Wide‑band noise jammer |
| **2.4 GHz** | NRF24L01+ | • Channel‑map spectrum viewer<br>• Jammers: All / Wi‑Fi / BT / BLE / USB / VIDEO / RC  <br> |
| **BLE Spam** | ESP32 C3 | • BLE Spam (iOS) |
| **Infra‑Red** | IR LED + receiver | • Capture & replay<br>• Built‑in brute‑force tables for TVs & projectors |
| **RFID / NFC** | rdm6300 + PN532 | • Read, emulate (125 kHz RFID)<br>• Basic NFC read/detect (Mifare Classic / Ultralight)<br>• Write mode placeholder (WIP) |
| **FM Radio** (***eHack Portable***) | Si4713 | • FM frequency control from main device (76–108 MHz)<br>• Remote input level indicator <br>|
| **Games** | — | • Falling Dots, Snake, Flappy Bird |
| **Quality‑of‑Life** | — | • OLED UI with 3‑button navigation<br> • Vibration feedback<br> • Battery monitor<br> • Auto‑dimming<br> • Connection Telemetry page<br> • Settings saved to EEPROM |

---
 ## 🎮 Controls

- `UP` / `DOWN` — menu navigation, frequency/slot selection, parameter adjustment
- `OK` (click) — confirm / start-stop action
- `OK` (hold) — back / exit current screen
- `UP + DOWN` (hold) — lock / unlock controls
- Hold `OK` during boot — toggle startup mode `eHack` / `eGames`

---

## 🔌 Pinout & Wiring

### Raspberry Pi Pico Pins Used

| Module | Pico pins | Note |
|---|---|---|
| I2C bus (OLED + PN532) | `SDA=GP0`, `SCL=GP1` | Shared I2C bus |
| NRF24L01+ (SPI) | `SCK=GP6`, `MOSI=GP7`, `MISO=GP4`, `CE=GP21`, `CSN=GP20` | 2.4 GHz module |
| CC1101 (SPI1) | `SCK=GP10`, `MOSI=GP11`, `MISO=GP12`, `CSN=GP13`, `GDO0=GP19` | Sub‑GHz module |
| Buttons | `UP=GP5`, `OK=GP8`, `DOWN=GP9` | Buttons to GND (`INPUT_PULLUP` in code) |
| IR | `TX=GP2`, `RX=GP3` | IR LED + IR receiver |
| RFID 125 kHz | `COIL=GP14`, `RDM6300_RX=GP15`, `RFID_POWER=GP27` | Power/enable controlled by GPIO |
| BLE trigger | `BLE_PIN=GP18` | Control pin for external BLE/ESP32 module |
| Vibro | `VIBRO=GP16` | Use transistor/driver stage |
| Battery monitor | `A3` | Battery voltage divider measurement |

---

## On‑Device Menu

```text
Main
├─ SubGHz
│   ├─ Air Scan
│   │   ├─ Spectrum
│   │   └─ Activity
│   ├─ Raw Scan
│   │   ├─ Capture
│   │   └─ Replay
│   ├─ Common
│   │   ├─ Capture
│   │   ├─ Replay
│   │   └─ Monitor
│   ├─ Barriers
│   │   ├─ Capture
│   │   ├─ Replay
│   │   └─ Brute (CAME / NICE)
│   ├─ Jammer
│   └─ Tesla
├─ 2.4 GHz
│   ├─ Spectrum
│   ├─ All Jam
│   ├─ Wi‑Fi Jam
│   ├─ BT Jam
│   ├─ BLE Jam
│   ├─ USB Jam
│   ├─ VIDEO Jam
│   ├─ RC Jam
│   └─ BLE Spam
├─ IR Tools
│   ├─ Capture
│   ├─ Replay
│   ├─ TV Brute
│   └─ Projector Brute
├─ FM Radio
├─ RFID
│   ├─ Read
│   ├─ Emulate
│   └─ Write
├─ Games
│   ├─ Falling Dots
│   ├─ Snake
│   └─ Flappy Bird
├─ Torch
├─ Connect
├─ Settings
└─ Telemetry
```
---

## Disclaimer 🚨

> **Educational & Research Use Only**  
> Use this hardware and software responsibly and **only on frequencies and systems you are legally allowed to transmit on**.  
> The author (**Elvin Gadirov**) accepts **no liability** for any damage, data loss, or legal consequences that may result from its use.  
> **All actions are at your own risk.** Always comply with local laws and regulations.
