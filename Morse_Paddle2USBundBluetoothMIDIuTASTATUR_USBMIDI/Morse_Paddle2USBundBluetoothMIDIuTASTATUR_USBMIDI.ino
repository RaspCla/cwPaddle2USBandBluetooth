/*
Required Notice: Copyright (c) 2026 RaspCla (https://github.com/RaspCla)

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
Lizenz: PolyForm Noncommercial License 1.0.0
  https://polyformproject.org/licenses/noncommercial/1.0.0
Nicht-kommerzielle Nutzung und Veraenderung erlaubt. Kommerzielle Nutzung nur mit
Genehmigung des Autors.

Diese Software wurde mit Unterstuetzung von Claude (Anthropic) angepasst und erweitert.
*/

/*
* ==============================================================================
* PROJEKT: Drahtloses Morse-Paddle zu USB-HID & Bluetooth Interface (S3)
* HARDWARE: Heemol ESP32-S3 Mini Entwicklungsboard Typ-C (von Amazon)
* 
* BESCHREIBUNG:
* Sendet via USB-Kabel PC-Tastaturbefehle (Standard: Strg-Links / Strg-Rechts).
* Sendet via Bluetooth-Funk wahlweise:
*   a) BLE-MIDI Noten (Standard: 18 und 38, z.B. an Morse-It auf dem iPhone), ODER
* Sendet zusaetzlich optional per USB-Kabel echte USB-MIDI-Noten (eigene Tonhoehen,
*   unabhaengig von den BLE-MIDI-Noten), parallel zu den USB-Tastaturbefehlen. So laesst
*   sich das Paddle z.B. an eine Windows-App andocken, ohne dass "Strg links/rechts"
*   gesendet wird. Erfordert einen Neustart beim Ein-/Ausschalten, Notenaenderungen
*   wirken sofort.
* USB-Tastatur und USB-MIDI lassen sich im Webinterface JEWEILS EINZELN ein- und
*   ausschalten (z.B. nur MIDI, wenn die PC-Anwendung die Strg-Tasten als Shortcut
*   interpretiert und das stoert), oder auch beide gleichzeitig. Default: USB-Tastatur
*   an (wie bisher), USB-MIDI aus.
*   b) eine BLE-HID Tastatur mit frei waehlbaren Tasten
* Alle drei Ausgaenge werden im Web-Interface GETRENNT konfiguriert:
*   - USB-Tastatur:  Taste je Paddle (gilt nur fuer USB, unabhaengig von Bluetooth)
*   - BLE-Tastatur:  Taste je Paddle (gilt nur im Bluetooth-Modus "BLE-Tastatur")
*   - BLE-MIDI:      Note je Paddle   (gilt nur im Bluetooth-Modus "BLE-MIDI")
* Die Tastenauswahl umfasst Modifier (Strg/Umschalt/Alt/Windows), Sondertasten,
* Buchstaben, Ziffern, Funktionstasten sowie Sonderzeichen der deutschen Tastatur
* (inkl. Zeichen, die Umschalt bzw. AltGr benoetigen). In allen Auswahllisten ist
* der jeweilige Default-Wert mit "(default)" gekennzeichnet.
* Bietet ein Web-Interface auf Core 0 zur Konfiguration aller Parameter.
* Web Adresse: 192.168.40.1, SID: Morse-S3-Config
* Geht nach Inaktivität in den Tiefschlaf. Wacht per Paddledruck auf.
* 
* ==============================================================================
* VERKABELUNG LAUT SCHALTPLANN
* - Linkes Paddle (Dot) -> Pin GPIO3 (gegen GND schaltend)
* - Rechtes Paddle (Dash) -> Pin GPIO5 (gegen GND schaltend, USB-Fix)
* - WS2812 Datenleitung -> Pin GPIO21 (verbunden mit Din der LED)
* 
* ==============================================================================
* Board: "ESP32" -> "ESP32-S3 Dev Module"
*
* AUSGEWÄHLTE BOARD-EINSTELLUNGEN:
* - USB CDC On Boot: "Enabled" (Zwingend aktiv für COM-Port und Upload!)
* - CPU Frequency: "240MHz (WiFi)"
* - Core Debug Level: "None"
* - USB DFU On Boot: "Disabled"
* - Erase All Flash Before Sketch Upload: "Disabled"
* - Events Run On: "Core1
* - Flash Mode: "QIO 80Mhz"
* - Flash Size: "4MB (32Mb)
* - JTAG Adapter: "Dissabled"
* - Arduino Run On: "Core 1"
* - USB Firmware MC On Boot: "Disabled"
* - Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
* - PSRAM: "Disabled"
* - Upload Mode: "USB-OTG CDC (TinyUSB)" nicht "UART0 / Hardware CDC"!
* - Upload Speed: "921600" oder niedriger (Max. Speed begrenzen für stabilen Flash!)
* - USB Mode: "USB-OTG (TinyUSB)" (Aktiviert die native Tastatur-Emulation)
* - Zigbee Mode: "Disabled"
* 
* ==============================================================================
* WICHTIGE INBETRIEBNAHME- & FLASH-HINWEISE:
* ------------------------------------------
* MANUELLER BOOT-MODUS (Falls das Hochladen fehlschlägt):
* 
* Halte den linken Taster (BOOT) gedrückt.
* 
* Drücke kurz den rechten Taster (RESET).
* 
* Lass den linken Taster (BOOT) wieder los.
* (Blickrichtung: Von oben auf das Board, USB-C-Buchse zeigt nach OBEN)
* 
* COM-PORT WECHSEL BEACHTEN:
* Im manuellen Boot-Modus weist Windows dem Board oft eine andere (neue)
* COM-Port-Nummer zu! Diese muss vor dem Flashen in der IDE neu gewählt werden.
* Nach dem Reboot mit der neuen Software wechselt der Port wieder auf die ursprüngliche Nummer.
* 
* ERSTER KALTSTART NACH DEM FLASHEN:
* Nach erfolgreichem Hochladen bootet das Device oft nicht direkt/richtig und ist
* zunächst nicht ansprechbar. Das Interface muss einmal komplett AUS- und wieder
* EINGESCHALTET (USB-Kabel kurz abziehen) werden, damit es einwandfrei startet.
* ==============================================================================
* Benötigte Bibliotheken:
* 
* Adafruit NeoPixel (Über Sketch -> Bibliothek einbinden -> Bibliotheken verwalten)
* 
* Fuer den neuen BLE-Tastatur-Modus werden KEINE zusaetzlichen Bibliotheken
* benoetigt: BLEHIDDevice.h und BLESecurity.h sind Teil derselben "ESP32 BLE
* Arduino" Bibliothek, aus der bereits BLEDevice.h / BLEServer.h stammen.
*
* ==============================================================================
* Benötigte Board Packages:
* ESP32 by Espressif Systems
* ==============================================================================
* BLUETOOTH-MODUS & TASTENBELEGUNG (per Webinterface)
*
* - Modus "BLE-MIDI": sendet MIDI-Noten (z.B. fuer Morse-It). Note fuer linkes und
*   rechtes Paddle waehlbar (Default: 18 = Punkt, 38 = Strich).
*   Zusaetzlich waehlbar: MIDI-Lautstaerke (CC7 = Channel Volume, Default 127 = Maximum).
*   Sie wird pro Verbindung einmal mit dem ersten Ton (im selben BLE-Paket) gesendet
*   und nach dem Speichern neuer Werte erneut. Nicht jede App reagiert auf CC7.
* - Modus "BLE-Tastatur": meldet sich als Bluetooth-HID-Tastatur an und sendet
*   pro Paddle die gewaehlte Taste (Default: Strg links / Strg rechts)
* - USB-Tastatur: eigene Tastenwahl, unabhaengig vom Bluetooth-Modus
*   (Default: Strg links / Strg rechts). Wird ohne Neustart uebernommen.
* - BLE-Noten und BLE-Tasten haben einen eigenen Speichern-Button und werden ebenfalls
*   ohne Neustart uebernommen. Nur der Wechsel BLE-MIDI <-> BLE-Tastatur startet neu.
* - Die Tasten werden als HID-Usage-Codes (= Tastenposition) gesendet. Die
*   Bezeichnungen in den Listen gelten fuer ein Empfangsgeraet mit DEUTSCHER
*   Tastaturbelegung (QWERTZ).
* - Eine aeltere, gemeinsame Tastenwahl fuer USB+BLE (Preferences "key_left" /
*   "key_right") wird beim ersten Start als Startwert fuer beide uebernommen.
*
* - WICHTIG: BLE-MIDI und BLE-Tastatur funktionieren NICHT gleichzeitig. Es ist immer nur
*   ein Modus aktiv und immer nur EIN Geraet verbunden: Wer sich zuerst verbindet, belegt
*   das Geraet, bis er die Verbindung trennt.
* - Im MIDI-Modus verwendet das Geraet eine EIGENE Bluetooth-Adresse (aus der Original-
*   Adresse abgeleitet). Nur Bluetooth - WLAN-Adresse und IP bleiben unveraendert. Ein
*   Geraet, das frueher als Tastatur ("Morse-Paddle-KBD") gekoppelt wurde (z.B. ein Tablet),
*   findet den MIDI-Modus dadurch nicht mehr unter der gespeicherten Adresse und blockiert
*   ihn nicht. Der Tastatur-Modus behaelt die Original-Adresse (bestehende Kopplungen
*   bleiben gueltig). Benoetigt Arduino-ESP32 3.x (ESP-IDF 5.1 oder neuer).
*/

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBMIDI.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <esp_sleep.h>
#include <esp_mac.h>
#include <esp_idf_version.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <vector>

USBHIDKeyboard Keyboard;
WebServer server(80);
Preferences prefs;

#define MIDI_SERVICE_UUID "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define MIDI_CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

const int pin_paddle_links = 3;
const int pin_paddle_rechts = 5;
const int pin_ws2812_data = 21;
Adafruit_NeoPixel rgbLed(1, pin_ws2812_data, NEO_GRB + NEO_KHZ800);

// Default-Werte (werden in den Auswahllisten mit "(default)" markiert)
const int DEFAULT_NOTE_DOT = 18;
const int DEFAULT_NOTE_DASH = 38;
const int DEFAULT_MIDI_VOLUME = 127;   // MIDI CC7 (Channel Volume), 127 = Maximum
const char* const DEFAULT_KEY_LEFT = "LEFT_CTRL";
const char* const DEFAULT_KEY_RIGHT = "RIGHT_CTRL";

int debounceTime = 8;
bool swapPaddles = false;
unsigned long sleepTimeoutMs = 1200000;
unsigned long webTimeoutMs = 180000;
uint32_t colorHeartbeat = 0x0000FF;
uint32_t colorDot = 0xFF0000;
uint32_t colorDash = 0x00FF00;
String ssidHome = "";
String passHome = "";
String scanResultsHtml = "";

// ---- Bluetooth-Betriebsart, MIDI-Noten und konfigurierbare Tasten ----
// btMode: 0 = BLE-MIDI, 1 = BLE-HID-Tastatur
int btMode = 0;
int midiNoteDot = DEFAULT_NOTE_DOT;        // gilt nur fuer BLE-MIDI
int midiNoteDash = DEFAULT_NOTE_DASH;      // gilt nur fuer BLE-MIDI
int midiVolume = DEFAULT_MIDI_VOLUME;      // gilt nur fuer BLE-MIDI (CC7, 0-127)

// ---- USB-MIDI (zusaetzlich zur USB-Tastatur, unabhaengig von Bluetooth) ----
bool usbKeyboardEnabled = true;                // Ein-/Ausschalten braucht Neustart (Default: an, wie bisher)
bool usbMidiEnabled = false;                   // Ein-/Ausschalten braucht Neustart
int usbMidiNoteDot = DEFAULT_NOTE_DOT;         // Notenaenderung wirkt sofort
int usbMidiNoteDash = DEFAULT_NOTE_DASH;
USBMIDI usbMidi;
volatile bool midiVolumeSent = false;      // wurde CC7 in dieser Verbindung schon gesendet?
String btAddressText = "";                 // aktuelle Bluetooth-Adresse (fuer Webinterface/Serial)
String usbKeyLeftId = DEFAULT_KEY_LEFT;    // gilt nur fuer USB
String usbKeyRightId = DEFAULT_KEY_RIGHT;  // gilt nur fuer USB
String bleKeyLeftId = DEFAULT_KEY_LEFT;    // gilt nur fuer BLE-Tastatur
String bleKeyRightId = DEFAULT_KEY_RIGHT;  // gilt nur fuer BLE-Tastatur

unsigned long lastActivityTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long lastWebActivityTime = 0;
bool wifiActive = true;
bool connected = false;

BLECharacteristic* pMidiCharacteristic = nullptr;
BLEAdvertising* pAdvertising = nullptr;
TaskHandle_t MorseTask;

// ---- NEU: BLE-HID Tastatur ----
BLEHIDDevice* hid = nullptr;
BLECharacteristic* inputKeyboard = nullptr;

// Modifier-Bits fuer den BLE-HID Tastatur-Report (Byte 0 des Reports)
#define MOD_LEFT_CTRL   0x01
#define MOD_LEFT_SHIFT  0x02
#define MOD_LEFT_ALT    0x04
#define MOD_LEFT_GUI    0x08
#define MOD_RIGHT_CTRL  0x10
#define MOD_RIGHT_SHIFT 0x20
#define MOD_RIGHT_ALT   0x40
#define MOD_RIGHT_GUI   0x80

// Standard USB-HID Report-Descriptor fuer eine "Boot Keyboard":
// Byte 0 = Modifier-Byte, Byte 1 = reserviert, Byte 2-7 = bis zu 6 gleichzeitig
// gedrueckte Tasten (HID Usage IDs). Dieses Layout ist der ueblichen Struktur
// aus den USB-HID Usage Tables nachempfunden, wie sie in praktisch jeder
// BLE-HID-Tastatur-Implementierung verwendet wird.
static const uint8_t hidKeyboardDescriptor[] = {
  0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
  0x09, 0x06,       // USAGE (Keyboard)
  0xA1, 0x01,       // COLLECTION (Application)
  0x85, 0x01,       //   REPORT_ID (1)
  0x05, 0x07,       //   USAGE_PAGE (Keyboard/Keypad)
  0x19, 0xE0,       //   USAGE_MINIMUM (0xE0)
  0x29, 0xE7,       //   USAGE_MAXIMUM (0xE7)
  0x15, 0x00,       //   LOGICAL_MINIMUM (0)
  0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
  0x75, 0x01,       //   REPORT_SIZE (1)
  0x95, 0x08,       //   REPORT_COUNT (8)
  0x81, 0x02,       //   INPUT (Data,Var,Abs)   -> Modifier-Byte
  0x95, 0x01,       //   REPORT_COUNT (1)
  0x75, 0x08,       //   REPORT_SIZE (8)
  0x81, 0x01,       //   INPUT (Cnst,Ary,Abs)   -> reserviertes Byte
  0x95, 0x05,       //   REPORT_COUNT (5)
  0x75, 0x01,       //   REPORT_SIZE (1)
  0x05, 0x08,       //   USAGE_PAGE (LEDs)
  0x19, 0x01,       //   USAGE_MINIMUM (Num Lock)
  0x29, 0x05,       //   USAGE_MAXIMUM (Kana)
  0x91, 0x02,       //   OUTPUT (Data,Var,Abs)  -> LED-Report
  0x95, 0x01,       //   REPORT_COUNT (1)
  0x75, 0x03,       //   REPORT_SIZE (3)
  0x91, 0x01,       //   OUTPUT (Cnst,Ary,Abs)  -> LED-Padding
  0x95, 0x06,       //   REPORT_COUNT (6)
  0x75, 0x08,       //   REPORT_SIZE (8)
  0x15, 0x00,       //   LOGICAL_MINIMUM (0)
  0x25, 0x65,       //   LOGICAL_MAXIMUM (0x65)
  0x05, 0x07,       //   USAGE_PAGE (Keyboard/Keypad)
  0x19, 0x00,       //   USAGE_MINIMUM (0)
  0x29, 0x65,       //   USAGE_MAXIMUM (0x65)
  0x81, 0x00,       //   INPUT (Data,Ary,Abs)   -> bis zu 6 Tasten
  0xC0              // END_COLLECTION
};

// ---- Tabelle aller im Webinterface waehlbaren Tasten ----
// Alle Tasten werden als HID-Usage-Codes (= Tastenposition) gesendet, sowohl per
// USB als auch per BLE-Tastatur. Die Bezeichnungen in den Auswahllisten gelten
// fuer ein Empfangsgeraet mit DEUTSCHER Tastaturbelegung (QWERTZ).
// Zeichen, die Umschalt bzw. AltGr benoetigen, senden den passenden Modifier mit.
struct KeyOption {
  String id;          // interner Wert fuer <select>, in Preferences gespeichert
  String label;       // Anzeigename (HTML; Sonderzeichen bereits als Entity)
  const char* group;  // Gruppenname fuer <optgroup>
  uint8_t usage;      // HID-Usage-ID der Taste (0 = reine Modifiertaste)
  uint8_t modBits;    // Modifier-Bits (Strg/Umschalt/Alt/GUI), die mitgesendet werden
};

static const char* const GRP_MOD   = "Modifiertasten";
static const char* const GRP_SPEC  = "Sondertasten";
static const char* const GRP_LET   = "Buchstaben";
static const char* const GRP_DIG   = "Ziffern";
static const char* const GRP_FKEY  = "Funktionstasten";
static const char* const GRP_CHAR  = "Sonderzeichen (direkt)";
static const char* const GRP_SHIFT = "Sonderzeichen (mit Umschalt)";
static const char* const GRP_ALTGR = "Sonderzeichen (mit AltGr)";

std::vector<KeyOption> keyOptions;
const KeyOption* usbLeftKey = nullptr;
const KeyOption* usbRightKey = nullptr;
const KeyOption* bleLeftKey = nullptr;
const KeyOption* bleRightKey = nullptr;

void addKeyOption(const char* group, const String& id, const String& label, uint8_t usage, uint8_t modBits) {
  KeyOption k;
  k.id = id; k.label = label; k.group = group; k.usage = usage; k.modBits = modBits;
  keyOptions.push_back(k);
}

void buildKeyOptions() {
  keyOptions.clear();
  keyOptions.reserve(110);

  // Modifiertasten (links/rechts einzeln waehlbar)
  addKeyOption(GRP_MOD, "LEFT_CTRL",   "Links Strg (Ctrl)",   0, MOD_LEFT_CTRL);
  addKeyOption(GRP_MOD, "RIGHT_CTRL",  "Rechts Strg (Ctrl)",  0, MOD_RIGHT_CTRL);
  addKeyOption(GRP_MOD, "LEFT_SHIFT",  "Links Umschalt",      0, MOD_LEFT_SHIFT);
  addKeyOption(GRP_MOD, "RIGHT_SHIFT", "Rechts Umschalt",     0, MOD_RIGHT_SHIFT);
  addKeyOption(GRP_MOD, "LEFT_ALT",    "Links Alt",           0, MOD_LEFT_ALT);
  addKeyOption(GRP_MOD, "RIGHT_ALT",   "Rechts Alt (AltGr)",  0, MOD_RIGHT_ALT);
  addKeyOption(GRP_MOD, "LEFT_GUI",    "Links Windows/Cmd",   0, MOD_LEFT_GUI);
  addKeyOption(GRP_MOD, "RIGHT_GUI",   "Rechts Windows/Cmd",  0, MOD_RIGHT_GUI);

  // Sondertasten
  addKeyOption(GRP_SPEC, "SPACE", "Leertaste",    0x2C, 0);
  addKeyOption(GRP_SPEC, "ENTER", "Enter",        0x28, 0);
  addKeyOption(GRP_SPEC, "TAB",   "Tab",          0x2B, 0);
  addKeyOption(GRP_SPEC, "ESC",   "Esc",          0x29, 0);
  addKeyOption(GRP_SPEC, "UP",    "Pfeil hoch",   0x52, 0);
  addKeyOption(GRP_SPEC, "DOWN",  "Pfeil runter", 0x51, 0);
  addKeyOption(GRP_SPEC, "LEFT",  "Pfeil links",  0x50, 0);
  addKeyOption(GRP_SPEC, "RIGHT", "Pfeil rechts", 0x4F, 0);

  // Buchstaben A-Z (deutsche Belegung: Y und Z sind gegenueber US vertauscht)
  for (int i = 0; i < 26; i++) {
    char c = 'A' + i;
    String id = String((char)c);
    uint8_t usage = (uint8_t)(0x04 + i);
    if (c == 'Y') usage = 0x1D;
    if (c == 'Z') usage = 0x1C;
    addKeyOption(GRP_LET, id, id, usage, 0);
  }

  // Ziffern 1-9 und 0
  for (int d = 1; d <= 9; d++) {
    addKeyOption(GRP_DIG, String(d), String(d), (uint8_t)(0x1E + (d - 1)), 0);
  }
  addKeyOption(GRP_DIG, "0", "0", 0x27, 0);

  // Funktionstasten F1-F12
  for (int f = 1; f <= 12; f++) {
    addKeyOption(GRP_FKEY, "F" + String(f), "F" + String(f), (uint8_t)(0x3A + (f - 1)), 0);
  }

  // Sonderzeichen der deutschen Tastatur - direkt erreichbar
  addKeyOption(GRP_CHAR, "K_COMMA",  ", (Komma)",             0x36, 0);
  addKeyOption(GRP_CHAR, "K_PERIOD", ". (Punkt)",             0x37, 0);
  addKeyOption(GRP_CHAR, "K_MINUS",  "- (Bindestrich)",       0x38, 0);
  addKeyOption(GRP_CHAR, "K_HASH",   "# (Raute)",             0x32, 0);
  addKeyOption(GRP_CHAR, "K_PLUS",   "+ (Plus)",              0x30, 0);
  addKeyOption(GRP_CHAR, "K_LESS",   "&lt; (kleiner als)",    0x64, 0);
  addKeyOption(GRP_CHAR, "K_SZ",     "&szlig; (Eszett)",      0x2D, 0);
  addKeyOption(GRP_CHAR, "K_AE",     "&auml; (a-Umlaut)",     0x34, 0);
  addKeyOption(GRP_CHAR, "K_OE",     "&ouml; (o-Umlaut)",     0x33, 0);
  addKeyOption(GRP_CHAR, "K_UE",     "&uuml; (u-Umlaut)",     0x2F, 0);
  addKeyOption(GRP_CHAR, "K_ACUTE",  "&acute; (Akzent, Tottaste)",   0x2E, 0);
  addKeyOption(GRP_CHAR, "K_CIRC",   "^ (Zirkumflex, Tottaste)",     0x35, 0);

  // Sonderzeichen mit Umschalt
  addKeyOption(GRP_SHIFT, "SH_EXCL",       "! (Umschalt+1)",                 0x1E, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_QUOT",       "&quot; (Umschalt+2)",            0x1F, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_SECT",       "&sect; (Umschalt+3)",            0x20, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_DOLLAR",     "$ (Umschalt+4)",                 0x21, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_PERCENT",    "% (Umschalt+5)",                 0x22, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_AMP",        "&amp; (Umschalt+6)",             0x23, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_SLASH",      "/ (Umschalt+7)",                 0x24, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_LPAREN",     "( (Umschalt+8)",                 0x25, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_RPAREN",     ") (Umschalt+9)",                 0x26, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_EQUALS",     "= (Umschalt+0)",                 0x27, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_QUESTION",   "? (Umschalt+&szlig;)",           0x2D, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_SEMI",       "; (Umschalt+Komma)",             0x36, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_COLON",      ": (Umschalt+Punkt)",             0x37, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_UNDERSCORE", "_ (Umschalt+Bindestrich)",       0x38, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_ASTERISK",   "* (Umschalt+Plus)",              0x30, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_APOS",       "' (Umschalt+#)",                 0x32, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_GREATER",    "&gt; (Umschalt+&lt;)",           0x64, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_DEGREE",     "&deg; (Umschalt+^)",             0x35, MOD_LEFT_SHIFT);
  addKeyOption(GRP_SHIFT, "SH_GRAVE",      "` (Umschalt+&acute;, Tottaste)", 0x2E, MOD_LEFT_SHIFT);

  // Sonderzeichen mit AltGr
  addKeyOption(GRP_ALTGR, "AG_AT",        "@ (AltGr+Q)",            0x14, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_EURO",      "&euro; (AltGr+E)",       0x08, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_LBRACE",    "{ (AltGr+7)",            0x24, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_LBRACKET",  "[ (AltGr+8)",            0x25, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_RBRACKET",  "] (AltGr+9)",            0x26, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_RBRACE",    "} (AltGr+0)",            0x27, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_BACKSLASH", "\\ (AltGr+&szlig;)",     0x2D, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_TILDE",     "~ (AltGr+Plus)",         0x30, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_PIPE",      "| (AltGr+&lt;)",         0x64, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_SQUARED",   "&sup2; (AltGr+2)",       0x1F, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_CUBED",     "&sup3; (AltGr+3)",       0x20, MOD_RIGHT_ALT);
  addKeyOption(GRP_ALTGR, "AG_MICRO",     "&micro; (AltGr+M)",      0x10, MOD_RIGHT_ALT);
}

// Sucht eine Taste per ID; falls unbekannt: Fallback-ID, sonst erster Eintrag
const KeyOption* findKeyOption(const String& id, const String& fallbackId) {
  for (size_t i = 0; i < keyOptions.size(); i++) {
    if (keyOptions[i].id == id) return &keyOptions[i];
  }
  for (size_t i = 0; i < keyOptions.size(); i++) {
    if (keyOptions[i].id == fallbackId) return &keyOptions[i];
  }
  return keyOptions.empty() ? nullptr : &keyOptions[0];
}

void refreshKeyPointers() {
  usbLeftKey  = findKeyOption(usbKeyLeftId,  DEFAULT_KEY_LEFT);
  usbRightKey = findKeyOption(usbKeyRightId, DEFAULT_KEY_RIGHT);
  bleLeftKey  = findKeyOption(bleKeyLeftId,  DEFAULT_KEY_LEFT);
  bleRightKey = findKeyOption(bleKeyRightId, DEFAULT_KEY_RIGHT);
  // ungueltige gespeicherte IDs bereinigen
  if (usbLeftKey)  usbKeyLeftId  = usbLeftKey->id;
  if (usbRightKey) usbKeyRightId = usbRightKey->id;
  if (bleLeftKey)  bleKeyLeftId  = bleLeftKey->id;
  if (bleRightKey) bleKeyRightId = bleRightKey->id;
}

// Erzeugt das <select>-Element; der Default-Eintrag wird mit "(default)" markiert
String buildKeySelectHtml(const String& selectName, const String& currentId, const String& defaultId) {
  String html;
  html.reserve(9000);
  html = "<select name='" + selectName + "'>";
  const char* lastGroup = nullptr;
  for (size_t i = 0; i < keyOptions.size(); i++) {
    const KeyOption& k = keyOptions[i];
    if (lastGroup == nullptr || strcmp(lastGroup, k.group) != 0) {
      if (lastGroup != nullptr) html += "</optgroup>";
      html += "<optgroup label='";
      html += k.group;
      html += "'>";
      lastGroup = k.group;
    }
    html += "<option value='" + k.id + "'";
    if (k.id == currentId) html += " selected";
    html += ">" + k.label;
    if (k.id == defaultId) html += " (default)";
    html += "</option>";
  }
  if (lastGroup != nullptr) html += "</optgroup>";
  html += "</select>";
  return html;
}

// Erzeugt das <select>-Element fuer MIDI-Noten 0-127 (Notennamen: C4 = 60)
String buildMidiSelectHtml(const String& selectName, int currentNote, int defaultNote) {
  static const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  String html;
  html.reserve(6500);
  html = "<select name='" + selectName + "'>";
  for (int n = 0; n < 128; n++) {
    html += "<option value='" + String(n) + "'";
    if (n == currentNote) html += " selected";
    html += ">" + String(n) + " (" + noteNames[n % 12] + String(n / 12 - 1) + ")";
    if (n == defaultNote) html += " (default)";
    html += "</option>";
  }
  html += "</select>";
  return html;
}

// MIDI-Lautstaerke: Auswahl in 5-%-Schritten (Wert = CC7 0-127)
int volumeFromStep(int step) { return (step * 5 * 127 + 50) / 100; }   // step 0..20

String buildVolumeSelectHtml(const String& selectName, int currentVol, int defaultVol) {
  // naechstliegenden Eintrag als "selected" markieren
  int bestStep = 0, bestDiff = 1000;
  for (int s = 0; s <= 20; s++) {
    int d = abs(volumeFromStep(s) - currentVol);
    if (d < bestDiff) { bestDiff = d; bestStep = s; }
  }
  String html;
  html.reserve(1200);
  html = "<select name='" + selectName + "'>";
  for (int s = 20; s >= 0; s--) {
    int v = volumeFromStep(s);
    html += "<option value='" + String(v) + "'";
    if (s == bestStep) html += " selected";
    html += ">" + String(s * 5) + " % (" + String(v) + ")";
    if (v == defaultVol) html += " (default)";
    html += "</option>";
  }
  html += "</select>";
  return html;
}

class MyServerCallbacks : public BLEServerCallbacks {
void onConnect(BLEServer* pServer) { connected = true; midiVolumeSent = false; lastActivityTime = millis(); };
void onDisconnect(BLEServer* pServer) { connected = false; midiVolumeSent = false; BLEDevice::startAdvertising(); }
};
void loadSettings() {
prefs.begin("morse-config", true);
debounceTime = prefs.getInt("debounce", 8);
swapPaddles = prefs.getBool("swap", false);
sleepTimeoutMs = prefs.getULong("timeout", 1200000);
webTimeoutMs = prefs.getULong("web_timeout", 180000);
colorHeartbeat = prefs.getUInt("c_heart", 0x0000FF);
colorDot = prefs.getUInt("c_dot", 0xFF0000);
colorDash = prefs.getUInt("c_dash", 0x00FF00);
ssidHome = prefs.getString("ssid_h", "");
passHome = prefs.getString("pass_h", "");
btMode = prefs.getInt("bt_mode", 0);
midiNoteDot = constrain(prefs.getInt("midi_dot", DEFAULT_NOTE_DOT), 0, 127);
midiNoteDash = constrain(prefs.getInt("midi_dash", DEFAULT_NOTE_DASH), 0, 127);
midiVolume = constrain(prefs.getInt("midi_vol", DEFAULT_MIDI_VOLUME), 0, 127);
usbKeyboardEnabled = prefs.getBool("usbkbd_en", true);
usbMidiEnabled = prefs.getBool("usbmidi_en", false);
usbMidiNoteDot = constrain(prefs.getInt("usbmidi_dot", DEFAULT_NOTE_DOT), 0, 127);
usbMidiNoteDash = constrain(prefs.getInt("usbmidi_dash", DEFAULT_NOTE_DASH), 0, 127);
// Migration: fruehere gemeinsame Tastenwahl als Startwert fuer USB und BLE uebernehmen
String oldLeft = prefs.getString("key_left", DEFAULT_KEY_LEFT);
String oldRight = prefs.getString("key_right", DEFAULT_KEY_RIGHT);
usbKeyLeftId = prefs.getString("usb_key_l", oldLeft);
usbKeyRightId = prefs.getString("usb_key_r", oldRight);
bleKeyLeftId = prefs.getString("bt_key_l", oldLeft);
bleKeyRightId = prefs.getString("bt_key_r", oldRight);
prefs.end();
}
void saveSettings() {
prefs.begin("morse-config", false);
prefs.putInt("debounce", debounceTime);
prefs.putBool("swap", swapPaddles);
prefs.putULong("timeout", sleepTimeoutMs);
prefs.putULong("web_timeout", webTimeoutMs);
prefs.putUInt("c_heart", colorHeartbeat);
prefs.putUInt("c_dot", colorDot);
prefs.putUInt("c_dash", colorDash);
prefs.putString("ssid_h", ssidHome);
prefs.putString("pass_h", passHome);
prefs.putInt("bt_mode", btMode);
prefs.putInt("midi_dot", midiNoteDot);
prefs.putInt("midi_dash", midiNoteDash);
prefs.putInt("midi_vol", midiVolume);
prefs.putBool("usbkbd_en", usbKeyboardEnabled);
prefs.putBool("usbmidi_en", usbMidiEnabled);
prefs.putInt("usbmidi_dot", usbMidiNoteDot);
prefs.putInt("usbmidi_dash", usbMidiNoteDash);
prefs.putString("usb_key_l", usbKeyLeftId);
prefs.putString("usb_key_r", usbKeyRightId);
prefs.putString("bt_key_l", bleKeyLeftId);
prefs.putString("bt_key_r", bleKeyRightId);
prefs.end();
}
void scanNetworks() {
int n = WiFi.scanNetworks();
scanResultsHtml = "<select name='ssid_h'>";
scanResultsHtml += "<option value=''>-- Bitte waehlen --</option>";
String seenSsids = "|";
for (int i = 0; i < n; ++i) {
String currentSSID = WiFi.SSID(i);
if (currentSSID.length() == 0) continue;
if (seenSsids.indexOf("|" + currentSSID + "|") != -1) continue;
seenSsids += currentSSID + "|";
String selected = (currentSSID == ssidHome) ? " selected" : "";
scanResultsHtml += "<option value='" + currentSSID + "'" + selected + ">" + currentSSID + "</option>";
}
scanResultsHtml += "</select>";
}
void sendMidiNote(uint8_t note, bool noteOn) {
if (!connected) return;
if (noteOn && !midiVolumeSent) {
  // Erster Ton dieser Verbindung (bzw. nach Aenderung der Lautstaerke): Control Change 7
  // (Channel Volume) im selben BLE-MIDI-Paket vor dem Note-On senden.
  // Paket: Header, [Zeitstempel, CC7, Wert], [Zeitstempel, Note-On, Note, Velocity]
  uint8_t volPacket[] = {0x80, 0x80, 0xB0, 0x07, (uint8_t)midiVolume, 0x80, 0x90, note, 127};
  pMidiCharacteristic->setValue(volPacket, 9);
  pMidiCharacteristic->notify();
  midiVolumeSent = true;
  return;
}
uint8_t midiPacket[] = {0x80, 0x80, (uint8_t)(noteOn ? 0x90 : 0x80), note, (uint8_t)(noteOn ? 127 : 0)};
pMidiCharacteristic->setValue(midiPacket, 5);
pMidiCharacteristic->notify();
}

// ---- Tasten-Report bauen und senden (USB und BLE-Tastatur) ----
// Der Report wird immer komplett aus dem aktuellen Zustand beider Paddles
// berechnet: Modifier = ODER-Verknuepfung, Tasten = bis zu 2 gleichzeitig.
void buildKeyReport(const KeyOption* leftKey, bool leftDown, const KeyOption* rightKey, bool rightDown, uint8_t* mods, uint8_t* keys) {
  *mods = 0;
  for (int i = 0; i < 6; i++) keys[i] = 0;
  const KeyOption* opts[2] = { leftDown ? leftKey : nullptr, rightDown ? rightKey : nullptr };
  int n = 0;
  for (int i = 0; i < 2; i++) {
    const KeyOption* o = opts[i];
    if (o == nullptr) continue;
    *mods |= o->modBits;
    if (o->usage != 0) {
      bool dup = false;
      for (int j = 0; j < n; j++) if (keys[j] == o->usage) dup = true;
      if (!dup && n < 6) keys[n++] = o->usage;
    }
  }
}

void sendUsbKeyReport(bool leftDown, bool rightDown) {
  KeyReport rep;
  memset(&rep, 0, sizeof(rep));
  buildKeyReport(usbLeftKey, leftDown, usbRightKey, rightDown, &rep.modifiers, rep.keys);
  Keyboard.sendReport(&rep);
}

void sendBleKeyReport(bool leftDown, bool rightDown) {
  if (!connected || inputKeyboard == nullptr) return;
  uint8_t report[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  buildKeyReport(bleLeftKey, leftDown, bleRightKey, rightDown, &report[0], &report[2]);
  inputKeyboard->setValue(report, 8);
  inputKeyboard->notify();
}

uint32_t parseHexColor(String hex) {
if(hex.startsWith("#")) hex = hex.substring(1);
return strtoul(hex.c_str(), NULL, 16);
}
String getHexStr(uint32_t color) {
String h = String(color & 0xFFFFFF, HEX);
while(h.length() < 6) h = "0" + h;
return "#" + h;
}

// Statischer Seitenkopf inkl. Stylesheet
static const char PAGE_HEAD[] = R"HTML(<!DOCTYPE html><html><head><meta charset='utf-8'><title>Morse</title>
<style>
body{font-family:sans-serif;background:#1a1a1a;color:#eee;padding:15px;}
form{background:#2d2d2d;padding:25px;border-radius:10px;max-width:400px;margin:30px auto;box-shadow:0 4px 15px rgba(0,0,0,0.3);}
h2{text-align:center;color:#00d1b2;margin-top:0;}div{margin-bottom:15px;}
input[type='text'],input[type='password'],input[type='number'],select{width:95%;padding:10px;background:#444;color:#fff;border:1px solid #555;border-radius:4px;font-size:14px;}
.row{display:flex;align-items:center;margin:20px 0;}
.btn{background:#00d1b2;color:#fff;border:0;padding:12px;width:100%;border-radius:4px;cursor:pointer;font-weight:bold;font-size:15px;}
.btn-orange{background:#ffdd57;color:#333;margin-top:5px;}
.hint{font-size:12px;color:#aaa;margin:0 0 15px 0;}
fieldset{border:1px solid #555;border-radius:6px;margin:0 0 15px 0;padding:12px 10px 0 10px;}
legend{color:#ffdd57;font-size:13px;font-weight:bold;padding:0 6px;}
</style></head><body>
)HTML";

// Eine Zeile "Beschriftung + Auswahlliste" fuer Tasten bzw. Noten
void sendKeyRow(const char* label, const char* name, const String& currentId, const char* defaultId) {
  server.sendContent(String("<div>") + label + "<br>");
  server.sendContent(buildKeySelectHtml(name, currentId, defaultId));
  server.sendContent("</div>");
}
void sendMidiRow(const char* label, const char* name, int currentNote, int defaultNote) {
  server.sendContent(String("<div>") + label + "<br>");
  server.sendContent(buildMidiSelectHtml(name, currentNote, defaultNote));
  server.sendContent("</div>");
}

void sendVolumeRow(const char* label, const char* name, int currentVol, int defaultVol) {
  server.sendContent(String("<div>") + label + "<br>");
  server.sendContent(buildVolumeSelectHtml(name, currentVol, defaultVol));
  server.sendContent("</div>");
}

// Bestaetigungsseite nach "Speichern & neu starten": Der OK-Button fuehrt zurueck zur
// Konfig-Seite. Er wird erst nach ein paar Sekunden aktiv und wartet dann, bis das
// Geraet nach dem Neustart wieder erreichbar ist (statt in einer Fehlerseite zu landen).
void sendRebootPage(const char* message) {
  String html = PAGE_HEAD;
  html += "<form onsubmit='return false;'><h2>Gespeichert</h2><p id='msg'>";
  html += message;
  html += "</p>";
  html += R"HTML(<input type='button' id='ok' class='btn' value='Neustart...' disabled onclick='back()'>
</form>
<script>
var ok=document.getElementById('ok'), msg=document.getElementById('msg');
setTimeout(function(){ok.value='OK';ok.disabled=false;},4000);
function back(){
  ok.disabled=true; ok.value='Warte auf Neustart...';
  var tries=0;
  var t=setInterval(function(){
    tries++;
    var c=new AbortController();
    setTimeout(function(){c.abort();},2000);
    fetch('/',{cache:'no-store',signal:c.signal}).then(function(r){
      if(r.ok){clearInterval(t);location.href='/';}
    }).catch(function(){});
    if(tries>40){
      clearInterval(t); ok.disabled=false; ok.value='OK';
      msg.innerHTML='Ger&auml;t nicht erreichbar. Nach einer WLAN-&Auml;nderung hat sich evtl. die Adresse ge&auml;ndert.';
    }
  },1500);
}
</script></body></html>
)HTML";
  server.send(200, "text/html", html);
}

// Die Seite wird in Teilen (chunked) gesendet, da die Auswahllisten sehr gross sind
void handleRoot() {
  lastWebActivityTime = millis();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(PAGE_HEAD);

  // ---- Formular 1: Allgemeine Einstellungen ----
  String html = "<form action='/save' method='POST'><h2>Morse-S3 Config</h2>";
  html += "<div>Debounce (ms):<br><input type='number' name='debounce' value='" + String(debounceTime) + "'></div>";

  String checkStr = swapPaddles ? "checked" : "";
  html += "<div class='row'><input type='checkbox' name='swap' style='width:auto;margin-right:10px;' " + checkStr + "><label>Paddles tauschen (L/R)</label></div>";

  html += "<div>Sleep-Timeout (Min):<br><input type='number' name='timeout' value='" + String(sleepTimeoutMs / 60000) + "'></div>";
  html += "<div>Web-Timeout (Min):<br><input type='number' name='web_timeout' value='" + String(webTimeoutMs / 60000) + "'></div>";
  html += "<div>Farbe Takt:<br><input type='color' name='c_heart' value='" + getHexStr(colorHeartbeat) + "'></div>";
  html += "<div>Farbe Punkt:<br><input type='color' name='c_dot' value='" + getHexStr(colorDot) + "'></div>";
  html += "<div>Farbe Strich:<br><input type='color' name='c_dash' value='" + getHexStr(colorDash) + "'></div>";
  html += "<input type='submit' class='btn' value='Einstellungen speichern'>";
  html += "</form>\n";
  server.sendContent(html);

  // ---- Formular 2: USB-Tastatur (eigene Belegung, nur USB) ----
  server.sendContent(
    "<form action='/save_usb' method='POST'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>USB-Tastatur (Kabel)</h3>"
    "<p class='hint'>Gilt nur f&uuml;r die Verbindung per USB-Kabel &ndash; unabh&auml;ngig vom Bluetooth-Modus. "
    "Ein-/Ausschalten braucht einen Neustart, die Tastenwahl wirkt sofort.</p>");
  {
    String kbdChk = usbKeyboardEnabled ? "checked" : "";
    server.sendContent("<div class='row'><input type='checkbox' name='usbkbd_en' style='width:auto;margin-right:10px;' " + kbdChk + "><label>USB-Tastatur aktivieren</label></div>");
  }
  sendKeyRow("Taste Linkes Paddle (Punkt):", "usb_left", usbKeyLeftId, DEFAULT_KEY_LEFT);
  sendKeyRow("Taste Rechtes Paddle (Strich):", "usb_right", usbKeyRightId, DEFAULT_KEY_RIGHT);
  server.sendContent(
    "<input type='submit' formaction='/save_usbkbd_en' class='btn btn-orange' value='USB-Tastatur Ein/Aus speichern & neu starten'>"
    "<p class='hint' style='margin-top:8px;'>Nach dem Speichern startet das Ger&auml;t neu.</p>"
    "<input type='submit' formaction='/save_usb' class='btn' value='USB-Tasten speichern'>"
    "<p class='hint' style='margin-top:8px;'>Wird sofort &uuml;bernommen (kein Neustart).</p>"
    "</form>\n");

  // ---- Formular 2b: USB-MIDI (zusaetzlich zur USB-Tastatur, unabhaengig von Bluetooth) ----
  server.sendContent(
    "<form action='/save_usbmidi_notes' method='POST'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>USB-MIDI (Kabel)</h3>"
    "<p class='hint'>Sendet zus&auml;tzlich zu den USB-Tastaturbefehlen echte USB-MIDI-Noten &ndash; "
    "unabh&auml;ngig von Bluetooth und von den BLE-MIDI-Noten. Ein-/Ausschalten braucht einen Neustart, "
    "Notenaenderungen wirken sofort.</p>");
  {
    String midiChk = usbMidiEnabled ? "checked" : "";
    server.sendContent("<div class='row'><input type='checkbox' name='usbmidi_en' style='width:auto;margin-right:10px;' " + midiChk + "><label>USB-MIDI aktivieren</label></div>");
  }
  sendMidiRow("Note Linkes Paddle (Punkt):", "usbmidi_dot", usbMidiNoteDot, DEFAULT_NOTE_DOT);
  sendMidiRow("Note Rechtes Paddle (Strich):", "usbmidi_dash", usbMidiNoteDash, DEFAULT_NOTE_DASH);
  server.sendContent(
    "<input type='submit' formaction='/save_usbmidi_en' class='btn btn-orange' value='USB-MIDI Ein/Aus speichern & neu starten'>"
    "<p class='hint' style='margin-top:8px;'>Nach dem Speichern startet das Ger&auml;t neu.</p>"
    "<input type='submit' formaction='/save_usbmidi_notes' class='btn' value='USB-MIDI Noten speichern'>"
    "<p class='hint' style='margin-top:8px;'>Wird sofort &uuml;bernommen (kein Neustart).</p>"
    "</form>\n");

  // ---- Formular 3: Bluetooth (Modus + getrennte Belegung fuer MIDI bzw. Tastatur) ----
  server.sendContent(
    "<form action='/save_bt' method='POST'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>Bluetooth</h3>"
    "<p class='hint'>Betriebsart w&auml;hlen und speichern. Die Noten und Tasten darunter gelten jeweils nur im "
    "angegebenen Modus (der nicht aktive Bereich ist abgeblendet) und werden getrennt gespeichert.</p>"
    "<p class='hint'><b>Hinweis:</b> BLE-MIDI und BLE-Tastatur funktionieren nicht gleichzeitig. Es ist immer nur ein "
    "Ger&auml;t verbunden &ndash; wer sich zuerst verbindet, belegt das Ger&auml;t. Im MIDI-Modus nutzt das Ger&auml;t "
    "eine eigene Bluetooth-Adresse, damit sich ein fr&uuml;her als Tastatur gekoppeltes Ger&auml;t nicht "
    "versehentlich verbindet.</p>");

  String bt = "<div class='row'><label style='margin-right:15px;'><input type='radio' name='bt_mode' value='0' onchange='upd()' style='width:auto;' ";
  bt += (btMode == 0) ? "checked" : "";
  bt += "> BLE-MIDI</label><label><input type='radio' name='bt_mode' value='1' onchange='upd()' style='width:auto;' ";
  bt += (btMode == 1) ? "checked" : "";
  bt += "> BLE-Tastatur</label></div>";
  server.sendContent(bt);
  server.sendContent(
    "<input type='submit' formaction='/save_bt' class='btn btn-orange' value='BLE-Midi / BLE-Tastatur Auswahl speichern'>"
    "<p class='hint' style='margin-top:8px;'>Nach dem Speichern startet das Ger&auml;t neu.</p>");

  server.sendContent(String("<p class='hint'>Bluetooth-Adresse: ") + btAddressText +
                     (btMode == 1 ? " (BLE-Tastatur, Original-Adresse)" : " (BLE-MIDI, eigene Adresse)") + "</p>");

  server.sendContent("<fieldset id='fs_midi'><legend>Nur im Modus BLE-MIDI: Noten &amp; Lautst&auml;rke</legend>");
  sendMidiRow("Note Linkes Paddle (Punkt):", "midi_dot", midiNoteDot, DEFAULT_NOTE_DOT);
  sendMidiRow("Note Rechtes Paddle (Strich):", "midi_dash", midiNoteDash, DEFAULT_NOTE_DASH);
  server.sendContent("<p class='hint'>Notennamen nach Konvention C4 = 60.</p>");
  sendVolumeRow("Lautst&auml;rke (MIDI CC7):", "midi_vol", midiVolume, DEFAULT_MIDI_VOLUME);
  server.sendContent("<p class='hint'>Wird pro Verbindung mit dem ersten Ton gesendet. Nicht jede App reagiert darauf.</p></fieldset>");

  server.sendContent("<fieldset id='fs_kbd'><legend>Nur im Modus BLE-Tastatur: Tasten</legend>");
  sendKeyRow("Taste Linkes Paddle (Punkt):", "bt_left", bleKeyLeftId, DEFAULT_KEY_LEFT);
  sendKeyRow("Taste Rechtes Paddle (Strich):", "bt_right", bleKeyRightId, DEFAULT_KEY_RIGHT);
  server.sendContent("</fieldset>");

  server.sendContent(R"HTML(<input type='submit' formaction='/save_bt_keys' class='btn' value='Bluetooth Midi-Noten und Bluetooth Tasten speichern'>
<p class='hint' style='margin-top:8px;'>Wird sofort &uuml;bernommen (kein Neustart).</p>
</form>
<script>
function upd(){
  var m=document.querySelector("input[name=bt_mode]:checked").value;
  document.getElementById('fs_midi').style.opacity=(m=='0')?1:0.4;
  document.getElementById('fs_kbd').style.opacity=(m=='1')?1:0.4;
}
upd();
</script>
)HTML");

  // ---- Formular 4: WLAN-Verbindung ----
  html = "<form action='/save' method='POST'>";
  html += "<h3 style='color:#ffdd57;margin-top:0;'>WLAN-Verbindung</h3>";
  html += "<div>Netze:<br>" + scanResultsHtml + "</div>";
  html += "<button type='submit' formaction='/rescan' formmethod='get' class='btn btn-orange' style='margin-bottom:15px;'>WLAN neu scannen</button>";
  html += "<div>Oder manuell eingeben (z.B. verstecktes Netz):<br><input type='text' name='ssid_manual' placeholder='SSID manuell'></div>";
  html += "<div>Passwort:<br><input type='password' name='pass_h' value='" + passHome + "'></div>";
  html += "<input type='submit' name='reboot' class='btn btn-orange' value='WLAN speichern & neu starten'></form>";
  html += "</body></html>";
  server.sendContent(html);
  server.sendContent("");  // Ende der chunked-Antwort
}


void handleRescan() {
lastWebActivityTime = millis();
scanNetworks();
server.sendHeader("Location", "/");
server.send(303);
}
void handleSave() {
if (server.hasArg("debounce")) debounceTime = server.arg("debounce").toInt();
// Checkbox nur auswerten, wenn das Allgemein-Formular gesendet wurde (sonst wuerde
// z.B. "WLAN speichern" die Einstellung "Paddles tauschen" ungewollt zuruecksetzen)
if (server.hasArg("debounce")) swapPaddles = server.hasArg("swap");
if (server.hasArg("timeout")) sleepTimeoutMs = server.arg("timeout").toInt() * 60000;
if (server.hasArg("web_timeout")) webTimeoutMs = server.arg("web_timeout").toInt() * 60000;
if (server.hasArg("c_heart")) colorHeartbeat = parseHexColor(server.arg("c_heart"));
if (server.hasArg("c_dot")) colorDot = parseHexColor(server.arg("c_dot"));
if (server.hasArg("c_dash")) colorDash = parseHexColor(server.arg("c_dash"));
if (server.hasArg("reboot")) {
String manualSsid = server.hasArg("ssid_manual") ? server.arg("ssid_manual") : "";
if (manualSsid.length() > 0) {
ssidHome = manualSsid;
} else if (server.hasArg("ssid_h")) {
ssidHome = server.arg("ssid_h");
}
if (server.hasArg("pass_h")) passHome = server.arg("pass_h");
saveSettings();
sendRebootPage("WLAN-Einstellungen gespeichert. Das Ger&auml;t startet neu ...");
delay(1000);
ESP.restart();
} else {
saveSettings();
server.send(200, "text/html", "<script>alert('Parameter saved');window.location='/';</script>");
}
}

// ---- Speichern der USB-Tastenbelegung (wird sofort uebernommen, kein Neustart) ----
// ---- Ein/Aus-Schalten der USB-Tastatur - immer mit Neustart (USB-Geraetebeschreibung aendert sich) ----
void handleSaveUsbKbdEnable() {
  usbKeyboardEnabled = server.hasArg("usbkbd_en");
  if (server.hasArg("usb_left")) usbKeyLeftId = server.arg("usb_left");
  if (server.hasArg("usb_right")) usbKeyRightId = server.arg("usb_right");
  refreshKeyPointers();
  saveSettings();
  sendRebootPage("USB-Tastatur-Einstellung gespeichert. Das Ger&auml;t startet neu ...");
  delay(1000);
  ESP.restart();
}

void handleSaveUsb() {
  if (server.hasArg("usb_left")) usbKeyLeftId = server.arg("usb_left");
  if (server.hasArg("usb_right")) usbKeyRightId = server.arg("usb_right");
  refreshKeyPointers();
  saveSettings();
  server.send(200, "text/html", "<script>alert('USB-Tasten gespeichert');window.location='/';</script>");
}

// ---- Speichern der USB-MIDI-Noten (wird sofort uebernommen, kein Neustart) ----
// Der Ein/Aus-Zustand (usbMidiEnabled) wird hier bewusst NICHT geaendert - dafuer ist ein Neustart noetig.
void handleSaveUsbMidiNotes() {
  if (server.hasArg("usbmidi_dot")) usbMidiNoteDot = constrain((int)server.arg("usbmidi_dot").toInt(), 0, 127);
  if (server.hasArg("usbmidi_dash")) usbMidiNoteDash = constrain((int)server.arg("usbmidi_dash").toInt(), 0, 127);
  saveSettings();
  server.send(200, "text/html", "<script>alert('USB-MIDI-Noten gespeichert');window.location='/';</script>");
}

// ---- Speichern von USB-MIDI Ein/Aus - immer mit Neustart (USB-Geraetebeschreibung aendert sich) ----
// Die im Formular angezeigten Noten werden dabei wie bisher mitgespeichert.
void handleSaveUsbMidiEnable() {
  usbMidiEnabled = server.hasArg("usbmidi_en");
  if (server.hasArg("usbmidi_dot")) usbMidiNoteDot = constrain((int)server.arg("usbmidi_dot").toInt(), 0, 127);
  if (server.hasArg("usbmidi_dash")) usbMidiNoteDash = constrain((int)server.arg("usbmidi_dash").toInt(), 0, 127);
  saveSettings();
  sendRebootPage("USB-MIDI-Einstellung gespeichert. Das Ger&auml;t startet neu ...");
  delay(1000);
  ESP.restart();
}

// ---- Speichern von MIDI-Noten & BLE-Tastenbelegung (wird sofort uebernommen, kein Neustart) ----
// Der Bluetooth-Modus (bt_mode) wird hier bewusst NICHT geaendert - dafuer ist ein Neustart noetig.
void handleSaveBtKeys() {
  if (server.hasArg("midi_dot")) midiNoteDot = constrain((int)server.arg("midi_dot").toInt(), 0, 127);
  if (server.hasArg("midi_dash")) midiNoteDash = constrain((int)server.arg("midi_dash").toInt(), 0, 127);
  if (server.hasArg("midi_vol")) midiVolume = constrain((int)server.arg("midi_vol").toInt(), 0, 127);
  if (server.hasArg("bt_left")) bleKeyLeftId = server.arg("bt_left");
  if (server.hasArg("bt_right")) bleKeyRightId = server.arg("bt_right");
  refreshKeyPointers();
  midiVolumeSent = false;   // Lautstaerke (CC7) mit dem naechsten Ton neu senden
  saveSettings();
  server.send(200, "text/html", "<script>alert('Bluetooth Midi-Noten, Lautst\\u00e4rke und Tasten gespeichert');window.location='/';</script>");
}

// ---- Speichern von Bluetooth-Modus (BLE-MIDI / BLE-Tastatur) - immer mit Neustart ----
// Die im Formular angezeigten Noten und Tasten werden dabei wie bisher mitgespeichert.
void handleSaveBt() {
  if (server.hasArg("bt_mode")) btMode = (server.arg("bt_mode").toInt() == 1) ? 1 : 0;
  if (server.hasArg("midi_dot")) midiNoteDot = constrain((int)server.arg("midi_dot").toInt(), 0, 127);
  if (server.hasArg("midi_dash")) midiNoteDash = constrain((int)server.arg("midi_dash").toInt(), 0, 127);
  if (server.hasArg("midi_vol")) midiVolume = constrain((int)server.arg("midi_vol").toInt(), 0, 127);
  if (server.hasArg("bt_left")) bleKeyLeftId = server.arg("bt_left");
  if (server.hasArg("bt_right")) bleKeyRightId = server.arg("bt_right");
  refreshKeyPointers();
  saveSettings();
  sendRebootPage("Bluetooth-Einstellungen gespeichert. Das Ger&auml;t startet neu ...");
  delay(1000);
  ESP.restart();
}

void goToDeepSleep() {
rgbLed.setPixelColor(0, 0);
rgbLed.show();
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
BLEDevice::deinit(true);
gpio_hold_en((gpio_num_t)pin_paddle_links);
gpio_hold_en((gpio_num_t)pin_paddle_rechts);
gpio_deep_sleep_hold_en();
esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
esp_sleep_enable_ext1_wakeup((1ULL << pin_paddle_links) | (1ULL << pin_paddle_rechts), ESP_EXT1_WAKEUP_ANY_LOW);
esp_deep_sleep_start();
}
void MorseCodeLoop(void * pvParameters) {
static bool links_alt = false;
static bool rechts_alt = false;
for(;;) {
bool raw_links = (digitalRead(pin_paddle_links) == LOW);
bool raw_rechts = (digitalRead(pin_paddle_rechts) == LOW);
bool links_neu = swapPaddles ? raw_rechts : raw_links;
bool rechts_neu = swapPaddles ? raw_links : raw_rechts;
if (links_neu || rechts_neu || links_neu != links_alt || rechts_neu != rechts_alt) {
lastActivityTime = millis();
}
if (links_neu && rechts_neu) {
rgbLed.setPixelColor(0, colorDot | colorDash); rgbLed.show();
} else if (links_neu) {
rgbLed.setPixelColor(0, colorDot); rgbLed.show();
} else if (rechts_neu) {
rgbLed.setPixelColor(0, colorDash); rgbLed.show();
}
if (links_neu != links_alt || rechts_neu != rechts_alt) {
  // USB-Tastatur (eigene Belegung, immer aktiv)
  if (usbKeyboardEnabled) sendUsbKeyReport(links_neu, rechts_neu);
  // USB-MIDI (optional, zusaetzlich, unabhaengig von Bluetooth)
  if (usbMidiEnabled) {
    if (links_neu != links_alt) { if (links_neu) usbMidi.noteOn(usbMidiNoteDot, 127, 1); else usbMidi.noteOff(usbMidiNoteDot, 0, 1); }
    if (rechts_neu != rechts_alt) { if (rechts_neu) usbMidi.noteOn(usbMidiNoteDash, 127, 1); else usbMidi.noteOff(usbMidiNoteDash, 0, 1); }
  }
  // Bluetooth: entweder BLE-Tastatur (eigene Belegung) oder BLE-MIDI (eigene Noten)
  if (btMode == 1) {
    sendBleKeyReport(links_neu, rechts_neu);
  } else {
    if (links_neu != links_alt) sendMidiNote(midiNoteDot, links_neu);
    if (rechts_neu != rechts_alt) sendMidiNote(midiNoteDash, rechts_neu);
  }
  if (!links_neu && !rechts_neu) { rgbLed.setPixelColor(0, 0); rgbLed.show(); }
  links_alt = links_neu;
  rechts_alt = rechts_neu;
}
if (!links_neu && !rechts_neu && (millis() - lastHeartbeatTime > 1000)) {
rgbLed.setPixelColor(0, colorHeartbeat); rgbLed.show(); delay(30);
rgbLed.setPixelColor(0, 0); rgbLed.show();
lastHeartbeatTime = millis();
}
if ((millis() - lastActivityTime) > sleepTimeoutMs) {
goToDeepSleep();
}
delay(debounceTime);
}
}
// ---- Eigene Bluetooth-Adresse im MIDI-Modus ----
// Aendert NUR die Bluetooth-Adresse (esp_iface_mac_addr_set mit ESP_MAC_BT); WLAN-Adresse
// und IP bleiben unveraendert. Die neue Adresse wird deterministisch aus der Original-
// Bluetooth-Adresse abgeleitet (Bit "lokal verwaltet" gesetzt) und ist damit bei jedem
// Start gleich. MUSS vor BLEDevice::init() aufgerufen werden.
void useOwnBluetoothAddressForMidi() {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
  uint8_t original[6], own[6];
  if (esp_read_mac(original, ESP_MAC_BT) != ESP_OK) return;
  if (esp_derive_local_mac(own, original) != ESP_OK) return;
  esp_iface_mac_addr_set(own, ESP_MAC_BT);
#else
  #warning "Eigene Bluetooth-Adresse im MIDI-Modus benoetigt Arduino-ESP32 3.x (ESP-IDF 5.1+) - wird uebersprungen"
#endif
}

// Liest die eingestellte Bluetooth-Adresse (fuer Webinterface und seriellen Monitor).
// Bewusst ueber esp_read_mac(): funktioniert unabhaengig davon, ob die BLE-Bibliothek
// intern Bluedroid oder NimBLE verwendet (beim ESP32-S3 in Arduino-ESP32 3.x ist es NimBLE).
void readBluetoothAddress() {
  uint8_t a[6];
  if (esp_read_mac(a, ESP_MAC_BT) != ESP_OK) return;
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", a[0], a[1], a[2], a[3], a[4], a[5]);
  btAddressText = buf;
  Serial.printf("Bluetooth-Adresse (%s): %s\n", btMode == 1 ? "BLE-Tastatur" : "BLE-MIDI", btAddressText.c_str());
}

void setup() {
Serial.begin(115200);
loadSettings();
buildKeyOptions();
refreshKeyPointers();
rgbLed.begin();
rgbLed.setBrightness(40);
rgbLed.show();
if (usbKeyboardEnabled) Keyboard.begin();
if (usbMidiEnabled) usbMidi.begin();
USB.begin();
gpio_hold_dis((gpio_num_t)pin_paddle_links);
gpio_hold_dis((gpio_num_t)pin_paddle_rechts);
pinMode(pin_paddle_links, INPUT_PULLUP);
pinMode(pin_paddle_rechts, INPUT_PULLUP);

if (btMode == 1) {
  // ---- NEU: BLE-HID Tastatur ----
  BLEDevice::init("Morse-Paddle-KBD");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  hid = new BLEHIDDevice(pServer);
  inputKeyboard = hid->inputReport(1); // Report-ID 1, muss zum Descriptor passen
  hid->manufacturer()->setValue("DIY-Morse");
  hid->pnp(0x02, 0x05ac, 0x0001, 0x0100);
  hid->hidInfo(0x00, 0x01);

  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  hid->reportMap((uint8_t*)hidKeyboardDescriptor, sizeof(hidKeyboardDescriptor));
  hid->startServices();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(0x03C1); // HID Keyboard
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  hid->setBatteryLevel(100);
} else {
  // ---- Bestehend: BLE-MIDI ----
  useOwnBluetoothAddressForMidi();   // eigene Bluetooth-Adresse nur in diesem Modus
  BLEDevice::init("Morse-Paddle-MIDI");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(BLEUUID(MIDI_SERVICE_UUID));
  pMidiCharacteristic = pService->createCharacteristic(BLEUUID(MIDI_CHARACTERISTIC_UUID), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinInterval(0x0020); pAdvertising->setMaxInterval(0x0030);
  BLEDevice::startAdvertising();
}

readBluetoothAddress();

WiFi.mode(WIFI_STA);
WiFi.scanNetworks();
scanNetworks();
if (ssidHome.length() > 0) {
WiFi.begin(ssidHome.c_str(), passHome.c_str());
int retries = 0;
while (WiFi.status() != WL_CONNECTED && retries < 16) {
delay(500);
retries++;
}
}
if (WiFi.status() != WL_CONNECTED) {
WiFi.mode(WIFI_AP);
WiFi.softAP("Morse-S3-Config", "12345678");
}
server.on("/", handleRoot);
server.on("/rescan", handleRescan);
server.on("/save", handleSave);
server.on("/save_usb", handleSaveUsb);
server.on("/save_usbkbd_en", handleSaveUsbKbdEnable);
server.on("/save_usbmidi_notes", handleSaveUsbMidiNotes);
server.on("/save_usbmidi_en", handleSaveUsbMidiEnable);
server.on("/save_bt", handleSaveBt);
server.on("/save_bt_keys", handleSaveBtKeys);
server.begin();
lastActivityTime = millis();
lastHeartbeatTime = millis();
lastWebActivityTime = millis();
xTaskCreatePinnedToCore(MorseCodeLoop, "MorseTask", 4096, NULL, 3, &MorseTask, 1);
}
void loop() {
if (wifiActive) {
server.handleClient();
if ((millis() - lastWebActivityTime) > webTimeoutMs && (millis() - lastActivityTime) > webTimeoutMs) {
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
wifiActive = false;
}
}
delay(20);
}
