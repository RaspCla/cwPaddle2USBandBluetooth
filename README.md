# cwPaddle2USBandBluetooth
This project (adapter) converts the signals of a paddle to a USB or Bluetooth keyboard or MIDI interface via ESP32-S3 Mini.
Additionally, the adapter contains a keyer that can create a CW tone on a USB audio (microphone) interface. PTT support via CAT interface or serial pins is available as well.

This makes the adapter ideal for connecting a morse paddle to, for example:
* Morse-it (iPhone app)
* morsecode.world (via web browser)
* vband (via web browser)
* Simon Brown's SDR-Console (PC app): no additional software is needed here, the keyer of the adapter delivers the morse tone as a USB microphone and switches PTT via the virtual COM port (see [Adapter Configuration](docs/AdapterConfiguration.md); this requires the core version mentioned under Requirements)

Prerequisite are paddles which offer a switch for 'dit' and a second switch for 'dah'. Each switch will pull down an ESP32-S3 IO pin to ground.

My Windows app '[PaddleBridge](https://github.com/RaspCla/PaddleBridge)' is an alternative for applications that need a USB-MIDI interface.

More details can be found in the header of the ino-file and in the docs folder.

Take care to order the correct ESP32-S3 Mini board (e.g. Heemol ESP32-S3 Mini, 18 pins).

## Requirements
The sketch needs the **ESP32 Arduino core 3.3.13** from Espressif (released on 17 November 2026), or a later 3.3.x release (3.3.x with x >= 13). **Core 4.x is not supported by this sketch.** Install the core via the Arduino Boards Manager ("esp32" by Espressif Systems) and select version 3.3.13 explicitly; do not update to 4.x. The USB-Audio interface (morse tone as microphone) and the USB-Serial status pins (DCD/DSR for PTT) depend on this core; the sketch uses only the official core interfaces, no patched libraries are required. With an older core or with core 4.x the sketch stops with an error message when compiling.

Details (board settings, libraries) can be found in the header of the ino-file.

Many thanks to the Espressif team: I received great support and the extension of the core that made this possible. Without it, this project would not be available in this form.


## 3D Model
The 3D model of the case I used (developed by i-BoxIt) you can find on MakerWorld under the name "ESP32-S3 SuperMini Gehäuse".
Direct Link: https://makerworld.com/de/models/2851590-esp32-s3-supermini-case-snap-fit-options?from=search#profileId-3180623


## License
This project is licensed under the [PolyForm Noncommercial License 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0).
Noncommercial use and modification are permitted; commercial use requires prior
agreement with the author. The full license text is in [LICENSE](LICENSE).

Required Notice: Copyright (c) 2026 RaspCla (https://github.com/RaspCla)<br>
Required Notice: Project: cwPaddle2USBandBluetooth (https://github.com/RaspCla/cwPaddle2USBandBluetooth)
