# cwPaddle2USBandBluetooth
This project (adapter) converts signals of a paddle to an USB or Bluetooth Keyboard or -Midi Interface via ESP32-S3 Mini

Therefore this adapter can be used ideally to connect a morse paddle e.g. to Morse-it (iPhone) or morsecode.world (via PC)
Prerequisite are paddles which offers a switch for 'dit' and a second switch for 'dah'. Each switch will pull down an ESP32-S3 IO Pin to ground.

The adapter can also be connected directly to Simon Brown's SDR-Console for cw operation, no additional software is needed: the keyer of the adapter delivers the morse tone as a USB microphone and switches PTT via the virtual COM port (see [Adapter Configuration](docs/AdapterConfiguration.md), this requires the core version mentioned under Requirements).

My Windows app '[PaddleBridge](https://github.com/RaspCla/PaddleBridge)' is an alternative for applications that need a USB-MIDI interface.

More details you can find within the header of the ino-file and docs folder. 

Take care to order the correct ESP32-S3 Mini board (e.g. Heemol ESP32-S3 Mini (18 Pins)


## Requirements
The USB-Audio interface (morse tone as microphone) and the USB-Serial status pins (DCD/DSR for PTT) only work with the **ESP32 Arduino core 3.3.13** from Espressif (release announced, not yet published at the time of writing) or newer. The sketch uses only the official core interfaces for this, no patched libraries are required.

All other functions (USB keyboard, USB MIDI, Bluetooth, web configuration) work with older 3.x cores as well. Details can be found in the header of the ino-file.


## 3D Model
The 3D model of the case I used (developed by i-BoxIt) you can find on MakerWorld under the name "ESP32-S3 SuperMini Gehäuse".
Direct Link: https://makerworld.com/de/models/2851590-esp32-s3-supermini-case-snap-fit-options?from=search#profileId-3180623


## License
This project is licensed under the [PolyForm Noncommercial License 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0).
Noncommercial use and modification are permitted; commercial use requires prior
agreement with the author. The full license text is in [LICENSE](LICENSE).

Required Notice: Copyright (c) 2026 RaspCla (https://github.com/RaspCla)<br>
Required Notice: Project: cwPaddle2USBandBluetooth (https://github.com/RaspCla/cwPaddle2USBandBluetooth)
