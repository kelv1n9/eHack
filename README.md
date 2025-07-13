![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-blue?logo=raspberry-pi)
![License](https://img.shields.io/badge/license-MIT-green)

# 🚀 eHack Project

**eHack** is a versatile, all-in-one tool built on the **Raspberry Pi Pico**, designed for radio frequency analysis, penetration testing, and hardware research enthusiasts. The project integrates a wide array of tools for interacting with various wireless technologies, all housed in a compact form factor with a user-friendly OLED interface.

---

## 🛰️ Portable Module

To enhance its capabilities, **eHack** can be paired with the **eHack-portable-module**. This compact RF module connects wirelessly to the main unit and is controlled directly from its menu. It allows you to perform attacks and research from a distance, adding significant flexibility to your operations.

Find more information on the portable module in its dedicated repository: [eHack-portable-module](https://github.com/Mr-Proxy/eHack-portable-module)

---

## 🤝 Contributor Wanted!

> For the **eHack** project, we are looking for a developer who can help with the software implementation for the **PN532 NFC module**. The goal is to add full tag reading, writing, and emulation capabilities.
>
> Your contribution to this feature would be highly appreciated! If you have the experience and a desire to contribute, please feel free to reach out.


| &nbsp; | Bands&nbsp;/&nbsp;Tech | What you can do |
|---|---|---|
| **Sub‑GHz (315 – 915 MHz)** | CC1101 | • Live spectrum and activity scan<br>• Capture & replay OOK/ASK packets <br>• Gate / Barrier toolkit (capture, replay, brute‑force **CAME** & **NICE** codes)<br>• **Tesla** charge‑port opener<br>• Wide‑band noise jammer |
| **2.4 GHz** | NRF24L01+ | • Channel‑map spectrum viewer<br>• Jammers: All / Wi‑Fi / BT / BLE / USB / VIDEO / RC  <br> |
| **BLE Spam** | ESP32 C3 | • BLE Spam (iOS) |
| **Infra‑Red** | IR LED + receiver | • Capture & replay<br>• Built‑in brute‑force tables for TVs & projectors |
| **RFID / NFC** | rdm6300 + PN532 | • Read, emulate (only RFID) tags |
| **Games** | — | Falling Dots, Snake, Flappy Bird |
| **Quality‑of‑Life** | — | OLED UI with 3‑button navigation • vibration feedback • battery monitor • auto‑dimming • settings saved to EEPROM |

---

## On‑Device Menu

```text
Main
├─ SubGHz
│   ├─ Air Scan
│   │   ├─ Spectrum
│   │   └─ Activity
│   ├─ Common
│   │   ├─ Capture
│   │   └─ Replay
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
├─ RFID
│   ├─ Read
│   ├─ Emulate
│   └─ Write
├─ Games
│   ├─ Falling Dots
│   ├─ Snake
│   └─ Flappy Bird
├─ Connect
└─ Settings
```
---

## Disclaimer 🚨

> **Educational & Research Use Only**  
> Use this hardware and software responsibly and **only on frequencies and systems you are legally allowed to transmit on**.  
> The author (**Elvin Gadirov**) accepts **no liability** for any damage, data loss, or legal consequences that may result from its use.  
> **All actions are at your own risk.** Always comply with local laws and regulations.
