# eHack v3.2.0

![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-blue?logo=raspberry-pi)
![License](https://img.shields.io/badge/license-MIT-green)

**eHack** is a pocket‑sized, multi‑band penetration‑testing toolkit powered by a **Raspberry Pi Pico**,  
a **CC1101** sub‑GHz transceiver, an **NRF24L01+** 2.4 GHz radio, a **PN532** RFID/NFC front‑end,  
and a 128 × 64 OLED display.  
Everything is written for the Arduino framework and built with **PlatformIO** for fast recompiles and easy hacking.

| &nbsp; | Bands&nbsp;/&nbsp;Tech | What you can do |
|---|---|---|
| **Sub‑GHz (315 – 915 MHz)** | CC1101 | • Live spectrum scan<br>• Capture & replay OOK/ASK 433 MHz packets (RCSwitch‑style)<br>• Gate / Barrier toolkit (capture, replay, brute‑force **CAME** & **NICE** codes)<br>• **Tesla** charge‑port opener<br>• Wide‑band noise jammer |
| **2.4 GHz** | NRF24L01+ | • Channel‑map spectrum viewer<br>• Jammers: All / Wi‑Fi‑only / BT‑only / BLE‑only<br>• BLE advertising spammer |
| **Infra‑Red** | IR LED + receiver | • Capture & replay NEC/RC5/Samsung…<br>• Built‑in brute‑force tables for TVs & projectors |
| **RFID / NFC** | PN532 | • Read, emulate, and write 13.56 MHz (MIFARE / NFC‑A) tags |
| **Games** | — | Falling Dots, Snake, Flappy Bird |
| **Quality‑of‑Life** | — | OLED UI with 3‑button navigation • vibration feedback • battery monitor • auto‑dimming • settings saved to EEPROM |

---

## On‑Device Menu

```text
Main
├─ SubGHz
│   ├─ Air Scan
│   ├─ Barriers
│   │   ├─ Capture
│   │   ├─ Replay
│   │   └─ Brute (CAME / NICE)
│   ├─ Capture
│   ├─ Replay
│   ├─ Jammer
│   └─ Tesla
├─ 2.4 GHz
│   ├─ Spectrum
│   ├─ All Jam
│   ├─ Wi‑Fi Jam
│   ├─ BT Jam
│   ├─ BLE Jam
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
└─ Settings
```

---

## License — MIT

```text
MIT License

Copyright (c) 2025 Elvin Gadirov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

---

## Disclaimer 🚨

> **Educational & Research Use Only**  
> Use this hardware and software responsibly and **only on frequencies and systems you are legally allowed to transmit on**.  
> The author (**Elvin Gadirov**) accepts **no liability** for any damage, data loss, or legal consequences that may result from its use.  
> **All actions are at your own risk.** Always comply with local laws and regulations.
