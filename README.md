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
The sketch needs the **ESP32 Arduino core 3.3.13** from Espressif (released on 17 November 2026) or newer. Install it via the Arduino Boards Manager ("esp32" by Espressif Systems). The USB-Audio interface (morse tone as microphone) and the USB-Serial status pins (DCD/DSR for PTT) depend on it; the sketch uses only the official core interfaces, no patched libraries are required. With an older core the sketch stops with an error message when compiling.

Details (board settings, libraries) can be found in the header of the ino-file.
