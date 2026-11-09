<img src="ConfigurationPage_.jpg" align="left">

# Adapter Configuration

There is a possibility to configure this adpater via an web frontend (wifi)

## WLAN
If the adapter is freshly flashed you have to find it inside your wireless environment.
Default SSID is "Morse-S3-Config", you don't need a password.
Once you are connected to the device you can reach the configuration page via Webbrowser (e.g. Firefox..) at http://192.168.4.1

As an first step its a good idea to connect the adapter to your home network. Therefore scroll down, click to "Rescan WLAN", Go to "Netze:" and select you home network. Finally enter your network password and click "WLAN speichern & neu starten"




## Language
Currently you can select between "English" and "German" language

## Paddle
Define an optional debounce time in ms
Swap paddles if necessary

## ESP32 behavior
Timeouts can be configured for sleep mode and configuration frontend (Web-Timeout).
If more than one interfaces (Keyboard, Midi..) is used, sleep mode should not be activated

The onboard LED will generate a hardbeat (blink every second) as soon as the ESP32 has booted or left sleep mode.
The color of this heard beat can be adjusted.
Additionally the LED will become "red"/"green" if dot/dash key is activated. This colors can be changed, also.

## USB Keyboard
The ESP32 can emulate and USB Keyboard. Default keys are "Left Ctrl" and "Write Ctrl" for "dot" and "dash".
This defaults can be used e.g. for vband, morsecode.world or "Morse Keyer" app (tested with Mose Keyer V5.3.0)

## USB Midi
Some applicatons require a USB-Midi interface (e.g. [PaddleBridge](https://github.com/RaspCla/PaddleBridge))

## USB-Serial and USB-Audio
<br>**Warning: currently you have to patch a library file to enable the audio interface, contact the author**<br><br>
This configuration allows a morse paddle directly be connected to a software Trx like SDR-Console (Simon Brown).
A keyer can be defined which will provid a morse tone via an USB-Audio interface to the PC application.<br>
You will find a new Audi interface (Microfon) within your device manager and sound configurations. Typically its named: "Microfon (x-TinyUSB (UAC1)"
Mode (lambic...), Speed, Farnswoth, Tone and Rise times can be defined.
To activate the morse tone "USB Audio" needs to be enabled, don't forget to adjust the volume.

As an option you can attach a piezo speaker as sidetone infterface (low delay) to a specific Pin 

You can also activate PTT control via serial command (CAT interface) or serial pins (DSR od DCD). 
Please take into account, this feature will be avaialbe with ESP32 board lib V4.0.0 or later (will keep you updated)



<br clear="left"/>
<br>
<br>
[← back to table of contents](README.md)
