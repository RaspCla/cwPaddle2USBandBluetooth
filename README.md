# cwPaddle2USBandBluetooth
This project (adapter) converts signals of a paddle to an USB or Bluetooth Keyboard or -Midi Interface via ESP32-S3 Mini

Therefore this adapter can be used ideally to connect a morse paddle e.g. to Morse-it (iPhone) or morsecode.world (via PC)
Prerequisite are paddles which offers a switch for 'dit' and a second switch for 'dah'. Each switch will pull down an ESP32-S3 IO Pin to ground.

In combination with my Windows app '[PaddleBridge]([https://github.com/RaspCla/PaddleBridge])', this adapter can also be used, for cw operation with Simon Brown's SDR-Console.

More details you can find within the header of the ino-file and docs folder. 

Take care to order the correct ESP32-S3 Mini board (e.g. Heemol ESP32-S3 Mini (18 Pins)


## 3D Model
The 3D model of the case I used (developed by i-BoxIt) you can find on MakerWorld under the name "ESP32-S3 SuperMini Gehäuse".
Direct Link: https://makerworld.com/de/models/2851590-esp32-s3-supermini-case-snap-fit-options?from=search#profileId-3180623


## License
This project is licensed under the [PolyForm Noncommercial License 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0).
Noncommercial use and modification are permitted; commercial use requires prior
agreement with the author. The full license text is in [LICENSE](LICENSE).

Required Notice: Copyright (c) 2026 RaspCla (https://github.com/RaspCla)
Required Notice: Project: cwPaddle2USBandBluetooth (https://github.com/RaspCla/cwPaddle2USBandBluetooth)
