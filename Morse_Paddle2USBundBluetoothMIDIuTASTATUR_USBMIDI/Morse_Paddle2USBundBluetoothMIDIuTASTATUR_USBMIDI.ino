/*
Required Notice: Copyright (c) 2026 RaspCla (https://github.com/RaspCla)
Required Notice: Project: cwPaddle2USBandBluetooth (https://github.com/RaspCla/cwPaddle2USBandBluetooth)

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
Lizenz: PolyForm Noncommercial License 1.0.0
  https://polyformproject.org/licenses/noncommercial/1.0.0
Nicht-kommerzielle Nutzung und Veraenderung erlaubt. Kommerzielle Nutzung nur mit
Genehmigung des Autors.

Diese Software wurde mit Unterstuetzung von Claude (Anthropic) angepasst und erweitert.
*/

/*
* ==============================================================================
*  MORSE-PADDLE-INTERFACE (ESP32-S3)  -  Paddle zu USB, Bluetooth, MIDI, Audio, COM-Port
* ==============================================================================
*
*  INHALT
*   1. Ueberblick und Funktionen
*   2. Hardware und Verkabelung
*   3. Benoetigte Software (Board-Paket, Bibliotheken, Versionen, Core-Hinweise)
*   4. Arduino-IDE-Einstellungen
*   5. Erste Inbetriebnahme, Flashen, Fehlersuche
*   6. Bedienung / Konfiguration (Webinterface)
*   7. Betriebsarten im Detail (USB, Bluetooth, USB-Serial/PTT, Keyer, USB-Audio)
*   8. Bekannte Einschraenkungen
*
* ------------------------------------------------------------------------------
*  1. UEBERBLICK
* ------------------------------------------------------------------------------
*  Zwei Paddle-Kontakte (Dot/Dash) werden entprellt und gleichzeitig an mehrere
*  Ausgaenge gemeldet. Jeder Ausgang ist im Webinterface getrennt konfigurierbar:
*
*   - USB-Tastatur (HID)     Taste je Paddle (Standard: Strg links / Strg rechts)
*   - USB-MIDI               eigene Noten je Paddle (parallel zur USB-Tastatur)
*   - Bluetooth (BLE)        ENTWEDER BLE-MIDI (Standard: Note 18 = Dot, 38 = Dash,
*                            z.B. Morse-It auf dem iPhone) ODER BLE-HID-Tastatur
*                            mit frei waehlbaren Tasten ODER Bluetooth aus
*   - USB-Serial (COM-Port)  Statuszeilen/Leitungen fuer PC-Software: Dot, Dash, Taste
*                            und PTT; PTT optional als CAT-Textbefehle (z.B. "TX;"/"RX;")
*   - USB-Audio              ESP32 meldet sich als USB-Mikrofon (mono, 48 kHz, 16 Bit)
*                            und liefert den CW-Mitlese-Ton (Sinus mit weichen Flanken)
*   - Piezo-Mithoerton       Sidetone an einem frei waehlbaren GPIO (LEDC/PWM)
*   - Iambic-Keyer           Iambic A/B oder "aus" (Paddle wie Handtaste), WpM 5-60,
*                            optional Farnsworth (Zeichen-WpM getrennt vom Gesamt-WpM),
*                            Vorlauf (Lead) und Nachlaufzeit (Hold) fuer die PTT
*   - Tiefschlaf             nach einstellbarer Inaktivitaet, Aufwecken per Paddle
*   - RGB-LED (WS2812)       Statusanzeige; Farben fuer Heartbeat/Dot/Dash einstellbar
*
*  Die Tastenauswahl umfasst Modifier (Strg/Umschalt/Alt/Windows), Sondertasten,
*  Buchstaben, Ziffern, Funktionstasten und Sonderzeichen der deutschen Tastatur
*  (QWERTZ, inkl. Zeichen mit Umschalt/AltGr). Der Default steht in den Listen als
*  "(default)". Tastatur-/MIDI-/Bluetooth-Ausgaben bekommen immer die ROHEN
*  Paddle-Signale; der Keyer wirkt nur auf Mikrofon-Ton, Piezo und Statusleitungen.
*
*  Core-Aufteilung: Webinterface (WLAN/AP) auf Core 0, Paddle-/Keyer-Task auf Core 1.
*
* ------------------------------------------------------------------------------
*  2. HARDWARE UND VERKABELUNG
* ------------------------------------------------------------------------------
*  Board:  Heemol ESP32-S3 Mini Entwicklungsboard, Typ-C (Amazon), ESP32-S3
*  - Linkes  Paddle (Dot)   -> GPIO3   (schaltet gegen GND)
*  - Rechtes Paddle (Dash)  -> GPIO5   (schaltet gegen GND, USB-Fix)
*  - WS2812 Datenleitung    -> GPIO21  (Din der LED; fest auf dem Board)
*  - Piezo (optional)       -> frei waehlbarer GPIO (Standard 7), Minus an GND.
*                              Nicht erlaubt: 19/20 (USB), 21 (LED), Paddle-Pins, 22-37.
*  GPIO19/20 sind der native USB-Anschluss (D-/D+), nicht anderweitig verwenden.
*
* ------------------------------------------------------------------------------
*  3. BENOETIGTE SOFTWARE
* ------------------------------------------------------------------------------
*  Arduino IDE (getestet unter Windows 10).
*
*  BOARD-PAKET: "esp32" by Espressif Systems  (Boardverwalter, Min. Arduino-ESP32 3.x /
*    ESP-IDF 5.1 fuer BLE-Adresse im MIDI-Modus).
*    Je nach gewuenschter Funktion braucht es einen neueren Core:
*     - USB-Audio-Mikrofon:  Core mit USBAudioCard (ab Arduino-ESP32 3.3.12 / 4.0.0-alpha).
*       Ohne diese Klasse uebersetzt der Sketch trotzdem, der Bereich meldet
*       "nicht vorhanden".
*     - Statusleitungen DCD/DSR/Break ("PTT via Pin"):  Core mit USBCDC::notifyState()
*       (PR #13037, Espressif-Issue #13091). Zusaetzlich muessen die vorkompilierten
*       Bibliotheken (esp32-arduino-libs) mit CFG_TUD_CDC_NOTIFY=1 gebaut sein; bei
*       Stand der letzten Tests war das NICHT der Fall -> die Funktion ist dann
*       im Webinterface "gesperrt" (Statuszeile zeigt "CFG_TUD_CDC_NOTIFY=0").
*       Die CAT-Textbefehle ("PTT via Serial Commands") funktionieren unabhaengig davon.
*
*  Zuletzt getestet mit:  Arduino-ESP32 Master aus Git (4.0.0-alpha1, IDF 5.5, Libs
*    "esp32-arduino-libs-idf-release_v5.5-b774170f-v3"), Stand 11/2026.
*
*  BIBLIOTHEKEN
*   - Adafruit NeoPixel  (Bibliotheksverwalter; aktuell 1.15.5, Stand 11/2026)
*   - Alles andere gehoert zum ESP32-Core (keine weitere Installation noetig):
*     USB / USBHIDKeyboard / USBMIDI / USBAudioCard, TinyUSB (tusb.h),
*     BLE (BLEDevice, BLEServer, BLEHIDDevice, BLESecurity ...), WiFi, WebServer,
*     ESPmDNS, Preferences (NVS), esp_sleep, esp_timer.
*
*  USB-AUDIO-HEADER (nur bei Core-Master / 4.0.0-alpha1):
*   Die Datei  USBAudioCardDescriptors.h  (mit IAD, Interface Association Descriptor)
*   muss im Core ersetzt werden, sonst meldet Windows das Mikrofon nicht oder doppelt:
*     <Sketchbook>\hardware\espressif\esp32\libraries\USB\src\USBAudioCardDescriptors.h
*   Nach dem Ersetzen den IDE-Build-Cache loeschen (siehe Abschnitt 5).
*
* ------------------------------------------------------------------------------
*  4. ARDUINO-IDE-EINSTELLUNGEN   (Werkzeuge-Menue)
* ------------------------------------------------------------------------------
*  Board:                                 "ESP32S3 Dev Module"
*
*  USB CDC On Boot:                       "Enabled" <- Pflicht (COM-Port und Upload)
*  CPU Frequency:                         "240MHz (WiFi)"
*  Core Debug Level:                      "None"  (zur Fehlersuche "Error": log_e erscheint dann im
*                                          seriellen Monitor, wenn dort Serial.setDebugOutput aktiv ist)
*  USB DFU On Boot:                       "Disabled"     
*  Erase All Flash Before Sketch Upload:  "Disabled" (sonst gehen Einstellungen verloren)
*  Events Runs On:                        "Core 1"
*  Flash Mode                             "QIO 80MHz"
*  Flash Size:                     	      "4MB (32Mb)"
*  JTAG Adapter:                          "Disabled"     
*  Arduino Runs On:                       "Core 1"
*  USB Firmware MSC On Boot:              "Disabled"
*  Partition Scheme:                      "Huge APP (3MB No OTA/1MB SPIFFS)"  oder
*                                         "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
*  PSRAM:                                 "Disabled"
*  Upload Mode:                           "USB-OTG CDC (TinyUSB)"  (nicht UART0 / Hardware CDC)
*  Upload Speed:                          "921600" oder niedriger (stabiler)
*  USB Mode:                              "USB-OTG (TinyUSB)"      <- Pflicht (nicht "Hardware CDC")
*  Zigbee Mode:                           "Disabled"
*
* ------------------------------------------------------------------------------
*  5. ERSTE INBETRIEBNAHME, FLASHEN, FEHLERSUCHE
* ------------------------------------------------------------------------------
*  - Manueller Boot-Modus (wenn der Upload fehlschlaegt): BOOT-Taster (links)
*    gedrueckt halten, kurz RESET (rechts) druecken, BOOT loslassen.
*    (Blick von oben auf das Board, USB-C-Buchse zeigt nach OBEN.)
*  - COM-Port-Wechsel: Im Boot-Modus bekommt das Board von Windows oft eine ANDERE
*    COM-Nummer; vor dem Flashen in der IDE neu waehlen. Nach dem Neustart mit der
*    neuen Software kehrt der urspruengliche Port zurueck.
*  - Erster Kaltstart nach dem Flashen: Das Geraet startet oft nicht richtig. Einmal
*    das USB-Kabel kurz abziehen und wieder einstecken.
*  - Build-Cache der IDE (nach Austausch von Core-/Bibliotheks-Headern):
*    veraltete Objekte -> Ordner loeschen:  %LOCALAPPDATA%\arduino\sketches
*    (bei Core-Aenderungen zusaetzlich  %LOCALAPPDATA%\arduino\cores ).
*  - Meldung "unlinkat ... .map: Der Prozess kann nicht auf die Datei zugreifen"
*    beim Kompilieren ist ein kurzer Dateizugriffskonflikt (z.B. Virenscanner):
*    einfach noch einmal kompilieren.
*  - Audio-Mikrofon doppelt/fehlt in Windows: Header-Patch (Abschnitt 3) pruefen,
*    Build-Cache loeschen, ggf. altes Geraet im Geraete-Manager entfernen.
*  - Diagnose: Im Webinterface zeigt der Bereich "USB-Serial & USB-Audio" den
*    Zustand des Cores (Statusleitungen moeglich?), ob der COM-Port am PC geoeffnet ist,
*    DTR/RTS, gesendete/fehlgeschlagene Statusmeldungen und PTT-Befehle.
*    Ist "Diagnose im seriellen Monitor" aktiv (Standardeinstellung), gibt der
*    ESP32 ausfuehrliche Meldungen auf dem COM-Port aus; dann ist USB-Serial aus
*    (beide teilen sich den COM-Port, es gibt immer nur eines von beiden).
*
* ------------------------------------------------------------------------------
*  6. BEDIENUNG / KONFIGURATION (WEBINTERFACE)
* ------------------------------------------------------------------------------
*  Zugang
*   - Ohne gespeichertes/erreichbares Heimnetz: Das Geraet oeffnet einen Hotspot
*     SSID "Morse-S3-Config", Passwort "12345678" (ggf. vor Weitergabe aendern),
*     Adresse  http://192.168.4.1
*   - Mit Heimnetz (WLAN im Webinterface waehlen/eingeben): Adresse morse-s3.local
*     oder die vom Router vergebene IP (steht im seriellen Monitor).
*   - Das WLAN schaltet sich nach dem "Web-Timeout" ab (Standard 3 min, 0 = nie);
*     der Tiefschlaf folgt nach dem "Sleep-Timeout" (Standard 20 min, 0 = nie).
*     Aufwecken per Paddle-Druck.
*  Speichern
*   - Einstellungen OHNE Neustart wirken SOFORT, sobald sie geaendert werden (Zahlen-/
*     Textfelder nach Verlassen des Feldes), sind aber erst nach "Einstellungen
*     speichern" dauerhaft. Der Speichern-Knopf BLINKT, solange etwas nicht gespeichert ist.
*   - Neustart nur bei: WLAN-Zugang, USB-Tastatur/-MIDI/-Audio ein/aus, Bluetooth-Modus.
*     Diese Werte werden erst beim Speichern uebernommen.
*   - Die Sprache der Oberflaeche (Deutsch/English) wird sofort umgeschaltet.
*
* ------------------------------------------------------------------------------
*  7. BETRIEBSARTEN IM DETAIL
* ------------------------------------------------------------------------------
*  USB-TASTATUR / USB-MIDI
*   - Beide einzeln oder gleichzeitig ein-/ausschaltbar (Standard: Tastatur an, MIDI aus).
*     Ein-/Ausschalten braucht Neustart; Tasten und Noten wirken sofort.
*   - Nur MIDI nutzen, wenn die PC-Anwendung "Strg links/rechts" als Shortcut deutet.
*   - ACHTUNG: Tastatur UND MIDI gleichzeitig koennen nach automatischem Schlafen
*     (Sleep-Timeout) die USB-Treiber am PC haengen lassen (nur PC-Neustart hilft).
*     Dann Sleep-Timeout auf 0 setzen.
*
*  BLUETOOTH  (Betriebsart im Webinterface: BLE-Tastatur / BLE-MIDI / aus)
*   - BLE-MIDI: Noten je Paddle (Standard 18/38) und MIDI-Lautstaerke (CC7, Standard 127).
*     CC7 wird je Verbindung mit dem ersten Ton gesendet und nach dem Speichern neu;
*     nicht jede App reagiert darauf. Verlangt Pairing/Bonding; iOS 16+/macOS 13+
*     verbinden sich danach automatisch wieder (ob die App das nutzt, haengt von ihr ab).
*   - BLE-Tastatur: meldet sich als Bluetooth-HID-Tastatur ("Morse-Paddle-KBD") und
*     sendet je Paddle die gewaehlte Taste (Standard Strg links/rechts).
*   - BLE-MIDI und BLE-Tastatur laufen NICHT gleichzeitig; es ist nur EIN Geraet
*     verbunden (wer zuerst verbindet, belegt das Geraet).
*   - Im MIDI-Modus ("Morse-Paddle-MIDI") nutzt das Geraet eine eigene, aus der
*     Original-Adresse abgeleitete Bluetooth-Adresse, damit ein frueher als Tastatur
*     gekoppeltes Geraet nicht versehentlich verbindet. WLAN-Adresse und IP bleiben
*     unveraendert; der Tastatur-Modus behaelt die Original-Adresse.
*   - Tasten werden als HID-Usage-Codes (Tastenposition) gesendet; die Bezeichnungen
*     gelten fuer ein Empfangsgeraet mit deutscher Belegung (QWERTZ).
*   - Aeltere gemeinsame Preferences "key_left"/"key_right" werden beim ersten Start
*     als Startwert fuer USB- und BLE-Tasten uebernommen.
*
*  USB-SERIAL (virtueller COM-Port, Baudrate beliebig - bei USB ohne Bedeutung)
*   a) Statusleitungen ("PTT via Pin"):  Dot, Dash, Taste und PTT (= Paddle gedrueckt +
*      Nachlaufzeit) werden ueber die CDC-SerialState-Bits DCD, DSR bzw. Break an den PC
*      gemeldet (USBCDC::notifyState). Die Zuordnung ist frei waehlbar. CTS ist ueber
*      USB-CDC technisch nicht moeglich. Der PC-Software (Remote CW Keyer, SDR Console ...)
*      muss DSR bzw. DCD als Eingang eingestellt werden. Voraussetzung: Core + Libs
*      siehe Abschnitt 3; ausserdem sendet der Core nur bei gesetztem DTR (COM-Port am
*      PC geoeffnet) - ohne DTR kommen keine Meldungen an.
*   b) CAT-Textbefehle ("PTT via Serial Commands"): Beim Senden-Beginn/-Ende werden
*      frei einstellbare Texte (Standard "TX;" / "RX;") ueber den COM-Port gesendet;
*      leer = nichts senden. Optional auch ohne gesetztes DTR (Standard an, z.B. SDR Console).
*   - Lead time (Vorlauf): Ton und Taste folgen erst so viel spaeter als PTT
*     (Standard 150 ms). Hold time: PTT bleibt so lange nach dem letzten Zeichen aktiv
*     (Standard 300 ms). Beides gilt fuer beide PTT-Arten.
*
*  KEYER
*   - Modus aus / Iambic A / Iambic B, Tempo 5-60 WpM (Standard 20), optional Farnsworth.
*   - Steuert USB-Mikrofon-Ton, Piezo und Statusleitung "Taste".
*
*  USB-AUDIO (Mikrofon)
*   - Nur Aufnahme, mono, 48 kHz, 16 Bit. In der PC-Software (z.B. SDR Console) als
*     Audioquelle waehlen. Frequenz 300-1500 Hz (Standard 700), Lautstaerke und
*     Flankenzeit im Webinterface (wirken sofort); Ein/Aus braucht Neustart.
*
*  PIEZO
*   - Ein/Aus, Lautstaerke (dB-Kurve) und GPIO im Webinterface (wirken sofort).
*     Die Frequenz entspricht der Einstellung "Tone"; kein Lead. Fuer einen kleinen
*     Lautsprecher statt Piezo ist ggf. ein Treiber/Transistor noetig.
*
* ------------------------------------------------------------------------------
*  8. BEKANNTE EINSCHRAENKUNGEN
* ------------------------------------------------------------------------------
*   - "PTT via Pin" funktioniert erst, wenn Core UND vorkompilierte Libs
*     CFG_TUD_CDC_NOTIFY=1 enthalten (siehe Abschnitt 3, Issue #13091).
*   - Ob Windows (usbser.sys) die gesendeten SerialState-Bits DSR/DCD an Anwendungen
*     weitergibt, ist noch nicht abschliessend verifiziert.
*   - DTR/RTS vom PC werden direkt aus TinyUSB gelesen (die Core-Ereignisse sind verlustbehaftet).
*   - Webinterface ist unverschluesselt (HTTP) und der Hotspot-Schluessel steht im Quelltext.
* ==============================================================================
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
#include <ESPmDNS.h>
#include <Preferences.h>
#include <vector>
#include "sdkconfig.h"
#include "tusb.h"
// Groesse des TinyUSB-CDC-Sendepuffers (zur Berechnung der belegten Bytes = Groesse - freier Platz)
#if defined(CONFIG_TINYUSB_CDC_TX_BUFSIZE)
#define SER_TX_FIFO_SIZE CONFIG_TINYUSB_CDC_TX_BUFSIZE
#elif defined(CFG_TUD_CDC_TX_BUFSIZE)
#define SER_TX_FIFO_SIZE CFG_TUD_CDC_TX_BUFSIZE
#else
#define SER_TX_FIFO_SIZE 64
#endif
#if __has_include("USBAudioCard.h") && CONFIG_TINYUSB_AUDIO_ENABLED
#include "USBAudioCard.h"
#define MORSE_HAS_AUDIO 1
#else
#define MORSE_HAS_AUDIO 0
#endif
#include <math.h>
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <type_traits>
#include <utility>

USBHIDKeyboard* usbKeyboard = NULL;   // wird nur bei Aktivierung angelegt (siehe UsbEarlyInit)
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
int resetReasonCode = -1;                 // Grund des letzten Neustarts (esp_reset_reason), Diagnose
String wifiScanInfo = "";             // Ergebnis des letzten Scans fuer das gespeicherte Heimnetz
volatile int lastWifiReason = 0;      // letzter Abbruchgrund der WLAN-Verbindung (wifi_err_reason_t), 0 = keiner

// ---- Bluetooth-Betriebsart, MIDI-Noten und konfigurierbare Tasten ----
// btMode: 0 = BLE-MIDI, 1 = BLE-HID-Tastatur, 2 = Bluetooth aus
int btMode = 0;
int midiNoteDot = DEFAULT_NOTE_DOT;        // gilt nur fuer BLE-MIDI
int midiNoteDash = DEFAULT_NOTE_DASH;      // gilt nur fuer BLE-MIDI
int midiVolume = DEFAULT_MIDI_VOLUME;      // gilt nur fuer BLE-MIDI (CC7, 0-127)

// ---- USB-MIDI (zusaetzlich zur USB-Tastatur, unabhaengig von Bluetooth) ----
bool usbKeyboardEnabled = true;                // Ein-/Ausschalten braucht Neustart (Default: an, wie bisher)
bool usbMidiEnabled = false;                   // Ein-/Ausschalten braucht Neustart
int usbMidiNoteDot = DEFAULT_NOTE_DOT;         // Notenaenderung wirkt sofort
int usbMidiNoteDash = DEFAULT_NOTE_DASH;
USBMIDI* usbMidi = NULL;               // wird nur bei Aktivierung angelegt (siehe UsbEarlyInit)

// ---- USB-Serial-Modus: Statusleitungen DCD / DSR / Break (Geraet -> PC) ----
// Der virtuelle COM-Port kann dem PC ueber eine "Serial State"-Meldung mitteilen, ob DCD, DSR
// bzw. Break aktiv sind (Software wie "Remote CW Keyer" oder SDR Console liest diese Leitungen).
// Dafuer braucht es USBCDC::notifyState() (Arduino-ESP32 PR #13037, bei Erstellung dieses
// Sketches noch nicht in einem Release) UND vorkompilierte Bibliotheken mit CFG_TUD_CDC_NOTIFY.
// Damit der Sketch auch mit aelteren Cores uebersetzt, wird das Vorhandensein der Methode zur
// Compile-Zeit erkannt: Fehlt sie, bleibt der Bereich im Webinterface ohne Wirkung und meldet das.
// (Als Struktur statt als Funktion geschrieben, damit die Arduino-IDE daraus keine fehlerhaften
// Funktionsprototypen erzeugt.)
template <typename T, typename = void>
struct CdcNotifyApi {
  static constexpr bool available = false;
  static bool send(T&, bool, bool, bool) { return false; }
};
template <typename T>
struct CdcNotifyApi<T, std::void_t<decltype(std::declval<T&>().notifyState(false, false, false))>> {
  static constexpr bool available = true;
  static bool send(T& port, bool dcd, bool dsr, bool brk) { return port.notifyState(dcd, dsr, brk); }
};
typedef CdcNotifyApi<USBCDC> CdcNotify;
// Wert des TinyUSB-Schalters, mit dem die Core-Bibliotheken gebaut wurden (nur zur Anzeige, steuert nichts)
#if defined(CFG_TUD_CDC_NOTIFY)
  #if CFG_TUD_CDC_NOTIFY
    static const char* const CDC_NOTIFY_LIB = "1";
  #else
    static const char* const CDC_NOTIFY_LIB = "0";
  #endif
#else
  static const char* const CDC_NOTIFY_LIB = "unbekannt";
#endif
// Belegung: 0 = aus, 1 = DCD, 2 = DSR, 3 = Break (Break ist ein Ereignis, kein Dauerpegel - nicht empfohlen)
bool serialDebug = true;                   // Diagnose-Ausgaben im seriellen Monitor (schliesst USB-Serial aus: gleicher COM-Port)
#define DBG(...) do { if (serialDebug) Serial.printf(__VA_ARGS__); } while (0)
bool usbSerialEnabled = false;             // wirkt sofort, kein Neustart
int serMapDot = 1;                         // Punkt  -> DCD
int serMapDash = 2;                        // Strich -> DSR
int serMapPtt = 0;                         // PTT    -> aus
int serPttHangMs = 300;                    // PTT-Nachlaufzeit nach Loslassen beider Paddles
bool serIgnoreDtr = true;                  // TX/RX-Befehle auch ohne DTR senden (z.B. SDR Console)
volatile int serTxPending = 0;             // Bytes, die noch im USB-Sendepuffer auf Abholung durch den PC warten
bool serSentDcd = false;                   // zuletzt erfolgreich gemeldete Pegel
bool serSentDsr = false;
bool serSentBrk = false;
volatile bool serResync = false;           // Port wurde (neu) geoeffnet -> Pegel erneut melden
volatile bool serPortOpen = false;         // DTR vom PC aktiv (Port geoeffnet)
volatile bool serDtrRaw = false;           // Diagnose: zuletzt empfangene Leitungen vom PC
volatile bool serRtsRaw = false;
volatile uint32_t serLineEvents = 0;       // Diagnose: Anzahl der empfangenen Line-State-Ereignisse
volatile uint32_t serNotifyOk = 0;         // Diagnose: notifyState() erfolgreich / fehlgeschlagen
volatile uint32_t serNotifyFail = 0;
// ---- Piezo-Mithoerton (sofort, ohne Lead time; Frequenz wie der USB-Audio-Ton) ----
bool piezoEnabled = false;
int piezoVolumePct = 50;                   // 1 - 100 %
int piezoPin = 7;                          // GPIO des Piezos (Plus-Anschluss, Minus an GND)
int uiLang = 0;                            // Weboberflaeche: 0 = Deutsch, 1 = English
int serMapKey = 0;                         // Keyer-Taste (verzoegert) -> Leitung, 0 = aus
String serTxCmd = "TX;";                   // PTT via serielle Befehle: Text beim Senden-Beginn
String serRxCmd = "RX;";                   // ... und beim Senden-Ende (leer = nichts senden)
volatile bool serCmdActive = true;         // mindestens ein serieller PTT-Befehl ist gesetzt
volatile uint32_t keyerTicks = 0;          // Diagnose: Zaehler Keyer-Task
volatile uint32_t audioFramesSent = 0;     // Diagnose: an den PC gesendete Audio-Pakete
volatile uint32_t serCmdSent = 0;          // Diagnose: gesendete PTT-Befehle
volatile uint32_t serCmdDropped = 0;       // Diagnose: verworfene PTT-Befehle (Port am PC nicht geoeffnet)

// ---- Keyer ----
int keyerMode = 0;                         // 0 = aus (Paddle direkt, wie Handtaste), 1 = Iambic A, 2 = Iambic B
int keyerWpm = 20;                         // Tempo 5 - 60 WpM
bool keyerFarn = false;                    // Farnsworth: Zeichenabstand dehnen
int keyerCharWpm = 18;                     // Zeichentempo bei Farnsworth (Standard: 18 WPM)
int keyLeadMs = 150;                       // Vorlauf Ton/Taste gegenueber PTT (wirkt nur bei aktiver PTT-Leitung)
volatile bool paddleDot = false;           // Paddle-Zustaende (nach Tauschen), von MorseCodeLoop gesetzt
volatile bool paddleDash = false;
volatile bool keyerBusy = false;           // Keyer sendet gerade

// ---- USB-Audio-Modus: Mikrofon mit Mithoerton (Ein/Aus braucht Neustart) ----
bool usbAudioEnabled = false;
int audioFreqHz = 700;                     // Tonhoehe 300 - 1500 Hz
int audioVolumePct = 50;                   // Lautstaerke 1 - 100 %
int audioRampMs = 5;                       // Anstiegs-/Abfallzeit 1 - 20 ms
volatile bool audioKeyed = false;          // wird von MorseCodeLoop gesetzt: Paddle gedrueckt
volatile bool audioStreaming = false;      // PC hat das Mikrofon geoeffnet
#if MORSE_HAS_AUDIO
USBAudioCard* audioCard = NULL;            // wird beim Programmstart angelegt (nur wenn aktiviert)
#endif

// ---- USB-Schnittstellen nur bei Aktivierung anlegen ----
// Der Arduino-Core startet den USB-Stack (bei "USB CDC On Boot: Enabled") schon VOR setup().
// Danach lassen sich keine USB-Schnittstellen mehr hinzufuegen. Die Objekte fuer Tastatur, MIDI
// und Audio muessen also beim Programmstart (als globale Konstruktoren) angelegt werden, und
// zwar nur, wenn sie eingeschaltet sind - sonst meldet das Geraet sie dem PC trotzdem an.
// Dafuer werden die drei Schalter hier direkt aus dem Flash-Speicher (NVS) gelesen.
struct UsbEarlyInit {
  UsbEarlyInit() {
    bool kbd = true, midi = false, aud = false;   // gleiche Standardwerte wie loadSettings()
    if (nvs_flash_init() == ESP_OK) {
      nvs_handle_t h;
      if (nvs_open("morse-config", NVS_READONLY, &h) == ESP_OK) {
        uint8_t v;
        if (nvs_get_u8(h, "usbkbd_en", &v) == ESP_OK) kbd = (v != 0);
        if (nvs_get_u8(h, "usbmidi_en", &v) == ESP_OK) midi = (v != 0);
        if (nvs_get_u8(h, "usbaud_en", &v) == ESP_OK) aud = (v != 0);
        nvs_close(h);
      }
    }
    if (kbd) usbKeyboard = new USBHIDKeyboard();
    if (midi) usbMidi = new USBMIDI();
#if MORSE_HAS_AUDIO
    // Nur Mikrofon (mono, 48 kHz, 16 Bit)
    if (aud) audioCard = new USBAudioCard(48000, UAC_BPS_16, UAC_SPK_NONE, UAC_MIC_MONO);
#endif
  }
};
static UsbEarlyInit usbEarlyInit;

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
// Die Auswahllisten (Tasten, Noten, Lautstaerke) werden NICHT mehr einzeln als HTML gesendet (je ~9 KB, insgesamt ~80 KB
// pro Seite - ueber ein schwaches WLAN brach die Seite dabei ab), sondern nur als leeres <select> mit data-Attributen.
// Die Optionen erzeugt ein kleines Skript am Seitenende aus einmalig gesendeten Daten (sendSelectData()).
String buildKeySelectHtml(const String& selectName, const String& currentId, const String& defaultId) {
  return "<select name='" + selectName + "' data-t='k' data-v='" + htmlAttr(currentId) + "' data-d='" + htmlAttr(defaultId) + "'></select>";
}
String buildMidiSelectHtml(const String& selectName, int currentNote, int defaultNote) {
  return "<select name='" + selectName + "' data-t='m' data-v='" + String(currentNote) + "' data-d='" + String(defaultNote) + "'></select>";
}
int volumeFromStep(int step) { return (step * 5 * 127 + 50) / 100; }   // step 0..20

String buildVolumeSelectHtml(const String& selectName, int currentVol, int defaultVol) {
  // naechstliegenden Eintrag vorauswaehlen
  int bestStep = 0, bestDiff = 1000;
  for (int st = 0; st <= 20; st++) {
    int d = abs(volumeFromStep(st) - currentVol);
    if (d < bestDiff) { bestDiff = d; bestStep = st; }
  }
  return "<select name='" + selectName + "' data-t='v' data-v='" + String(volumeFromStep(bestStep)) + "' data-d='" + String(defaultVol) + "'></select>";
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
btMode = constrain(prefs.getInt("bt_mode", 0), 0, 2);
midiNoteDot = constrain(prefs.getInt("midi_dot", DEFAULT_NOTE_DOT), 0, 127);
midiNoteDash = constrain(prefs.getInt("midi_dash", DEFAULT_NOTE_DASH), 0, 127);
midiVolume = constrain(prefs.getInt("midi_vol", DEFAULT_MIDI_VOLUME), 0, 127);
usbKeyboardEnabled = prefs.getBool("usbkbd_en", true);
usbMidiEnabled = prefs.getBool("usbmidi_en", false);
usbMidiNoteDot = constrain(prefs.getInt("usbmidi_dot", DEFAULT_NOTE_DOT), 0, 127);
usbMidiNoteDash = constrain(prefs.getInt("usbmidi_dash", DEFAULT_NOTE_DASH), 0, 127);
usbSerialEnabled = prefs.getBool("usbser_en", false);
serialDebug = prefs.getBool("dbg_en", true) && !usbSerialEnabled;
serMapDot = constrain(prefs.getInt("ser_dot", 1), 0, 3);
serMapDash = constrain(prefs.getInt("ser_dash", 2), 0, 3);
serMapPtt = constrain(prefs.getInt("ser_ptt", 0), 0, 3);
serPttHangMs = constrain(prefs.getInt("ser_hang", 300), 0, 5000);
serIgnoreDtr = prefs.getBool("ser_nodtr", true);
piezoEnabled = prefs.getBool("pz_en", false);
piezoVolumePct = constrain(prefs.getInt("pz_vol", 50), 1, 100);
piezoPin = prefs.getInt("pz_pin", 7);
uiLang = constrain(prefs.getInt("lang", 0), 0, 1);
serMapKey = constrain(prefs.getInt("ser_key", 0), 0, 3);
serTxCmd = prefs.getString("ser_tx", "TX;");
serRxCmd = prefs.getString("ser_rx", "RX;");
serCmdActive = (serTxCmd.length() > 0 || serRxCmd.length() > 0);
keyerMode = constrain(prefs.getInt("kmode", 0), 0, 2);
keyerWpm = constrain(prefs.getInt("kwpm", 20), 5, 60);
keyerFarn = prefs.getBool("kfarn", false);
keyerCharWpm = constrain(prefs.getInt("kcwpm", 18), 5, 60);
keyLeadMs = constrain(prefs.getInt("klead", 150), 0, 1000);
usbAudioEnabled = prefs.getBool("usbaud_en", false);
audioFreqHz = constrain(prefs.getInt("aud_freq", 700), 300, 1500);
audioVolumePct = constrain(prefs.getInt("aud_vol", 50), 1, 100);
audioRampMs = constrain(prefs.getInt("aud_ramp", 5), 1, 20);
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
prefs.putBool("usbser_en", usbSerialEnabled);
prefs.putBool("dbg_en", serialDebug);
prefs.putInt("ser_dot", serMapDot);
prefs.putInt("ser_dash", serMapDash);
prefs.putInt("ser_ptt", serMapPtt);
prefs.putInt("ser_hang", serPttHangMs);
prefs.putBool("ser_nodtr", serIgnoreDtr);
prefs.putBool("pz_en", piezoEnabled);
prefs.putInt("pz_vol", piezoVolumePct);
prefs.putInt("pz_pin", piezoPin);
prefs.putInt("lang", uiLang);
prefs.putInt("ser_key", serMapKey);
prefs.putString("ser_tx", serTxCmd);
prefs.putString("ser_rx", serRxCmd);
prefs.putInt("kmode", keyerMode);
prefs.putInt("kwpm", keyerWpm);
prefs.putBool("kfarn", keyerFarn);
prefs.putInt("kcwpm", keyerCharWpm);
prefs.putInt("klead", keyLeadMs);
prefs.putBool("usbaud_en", usbAudioEnabled);
prefs.putInt("aud_freq", audioFreqHz);
prefs.putInt("aud_vol", audioVolumePct);
prefs.putInt("aud_ramp", audioRampMs);
prefs.putString("usb_key_l", usbKeyLeftId);
prefs.putString("usb_key_r", usbKeyRightId);
prefs.putString("bt_key_l", bleKeyLeftId);
prefs.putString("bt_key_r", bleKeyRightId);
prefs.end();
}
String resetReasonText() {
  bool en = (uiLang == 1);
  switch ((esp_reset_reason_t)resetReasonCode) {
    case ESP_RST_POWERON: return en ? "power on" : "Einschalten";
    case ESP_RST_SW: return en ? "software restart" : "Software-Neustart";
    case ESP_RST_PANIC: return en ? "CRASH (panic)" : "ABSTURZ (Panic)";
    case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: return en ? "CRASH (watchdog)" : "ABSTURZ (Watchdog)";
    case ESP_RST_BROWNOUT: return en ? "undervoltage (brownout)" : "Unterspannung (Brownout)";
    case ESP_RST_DEEPSLEEP: return en ? "wake from deep sleep" : "Aufwachen aus Deep Sleep";
    case ESP_RST_USB: return "USB";
    default: return String("Code ") + resetReasonCode;
  }
}
void onWifiStaEvent(arduino_event_id_t event, arduino_event_info_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    lastWifiReason = info.wifi_sta_disconnected.reason;
    DBG("[%lu s] WLAN getrennt, Grund %d, Heap %u\n", (unsigned long)(millis() / 1000), (int)lastWifiReason, (unsigned)ESP.getFreeHeap());
  }
  else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) lastWifiReason = 0;
}
// Klartext zum WLAN-Abbruchgrund
String wifiReasonText() {
  switch (lastWifiReason) {
    case 0: return "kein Fehler gemeldet";
    case 201: return "Netz nicht gefunden (SSID falsch geschrieben, zu schwach oder nur 5 GHz &ndash; der ESP32-S3 kann nur 2,4 GHz)";
    case 2: case 15: case 202: case 204: case 205: case 6: case 7: return "Anmeldung abgelehnt (Passwort falsch oder Verschl&uuml;sselung nicht kompatibel, z.B. nur WPA3)";
    case 200: return "Signal verloren (Beacon-Timeout)";
    case 34: case 4: case 3: return "Verbindung abgebrochen (Signal zu schwach oder gest&ouml;rt)";
    case 8: return "Verbindung beendet (Asoc leave)";
    default: return String("Fehlercode ") + lastWifiReason;
  }
}

void scanNetworks() {
int n = WiFi.scanNetworks();
scanResultsHtml = "<select name='ssid_h'>";
scanResultsHtml += "<option value=''>-- Bitte waehlen --</option>";
String seenSsids = "|";
wifiScanInfo = "im Scan <b>nicht</b> gefunden";
for (int i = 0; i < n; ++i) {
String currentSSID = WiFi.SSID(i);
if (currentSSID.length() == 0) continue;
if (seenSsids.indexOf("|" + currentSSID + "|") != -1) continue;
seenSsids += currentSSID + "|";
String selected = (currentSSID == ssidHome) ? " selected" : "";
if (currentSSID == ssidHome) {
  const char* encN = "?";
  switch ((int)WiFi.encryptionType(i)) {
    case 0: encN = "open"; break; case 1: encN = "WEP"; break; case 2: encN = "WPA"; break; case 3: encN = "WPA2"; break;
    case 4: encN = "WPA/WPA2"; break; case 5: encN = "WPA2-Enterprise"; break; case 6: encN = "WPA3"; break; case 7: encN = "WPA2/WPA3"; break;
  }
  wifiScanInfo = String("im Scan gefunden: Kanal ") + WiFi.channel(i) + ", " + WiFi.RSSI(i) + " dBm, " + encN;
}
scanResultsHtml += "<option value='" + htmlAttr(currentSSID) + "'" + selected + ">" + htmlAttr(currentSSID) + "</option>";
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
  if (usbKeyboard != NULL) usbKeyboard->sendReport(&rep);
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
// Erlaubte Pins fuer den Piezo: nicht USB (19/20), nicht LED, nicht Paddles, nicht Flash/PSRAM (26 - 37)
bool piezoPinAllowed(int p) {
  if (p < 1 || p > 48) return false;
  if (p == pin_paddle_links || p == pin_paddle_rechts || p == pin_ws2812_data) return false;
  if (p == 19 || p == 20) return false;
  if (p >= 22 && p <= 37) return false;
  return true;
}

// ---- Sprachumschaltung der Weboberflaeche (Deutsch -> English) ----
// Der Quelltext enthaelt die deutschen Texte. Ist English gewaehlt, werden sie beim Senden ersetzt
// (laengste Eintraege zuerst). Nicht uebersetzte Texte bleiben einfach deutsch.
struct TrPair { const char* de; const char* en; };
static const TrPair TR_TABLE[] = {
  {"<b>Hinweis:</b> BLE-MIDI und BLE-Tastatur funktionieren nicht gleichzeitig. Es ist immer nur ein Ger&auml;t verbunden &ndash; wer sich zuerst verbindet, belegt das Ger&auml;t. Im MIDI-Modus nutzt das Ger&auml;t eine eigene Bluetooth-Adresse, damit sich ein fr&uuml;her als Tastatur gekoppeltes Ger&auml;t nicht versehentlich verbindet. Beide Modi verlangen Pairing (Kopplung) &ndash; danach verbinden sich unterst&uuml;tzte Ger&auml;te (z.B. iOS 16+/macOS 13+) beim Einschalten automatisch wieder. Ob die jeweilige App das nutzt, h&auml;ngt von der App ab.", "<b>Note:</b> BLE MIDI and BLE keyboard do not work at the same time. Only one device is connected at a time &ndash; whoever connects first occupies the device. In MIDI mode the device uses its own Bluetooth address so that a device previously paired as a keyboard does not connect by accident. Both modes require pairing &ndash; afterwards supported devices (e.g. iOS 16+/macOS 13+) reconnect automatically at power-on. Whether the respective app uses this depends on the app."},
  {"<b>Achtung:</b> Sind USB-Tastatur UND USB-MIDI gleichzeitig aktiviert, kann es nach einem automatischen Einschlafen (Sleep-Timeout) und Aufwachen vorkommen, dass sich die USB-Treiber am PC aufh&auml;ngen &ndash; Aus-/Einstecken hilft dann nicht mehr, nur ein PC-Neustart. Setze in diesem Fall den Sleep-Timeout oben auf 0 (nie). Das WLAN/Webinterface ist davon nicht betroffen.", "<b>Warning:</b> If USB keyboard AND USB MIDI are enabled at the same time, the USB drivers on the PC may hang after an automatic sleep (sleep timeout) and wake-up &ndash; unplugging and replugging does not help then, only a PC restart. In that case set the sleep timeout above to 0 (never). The WLAN/web interface is not affected."},
  {"Gelten f&uuml;r &bdquo;PTT via Serial Commands&ldquo; und &bdquo;PTT via Pin&ldquo;. Lead time = Vorlauf: Ton und Key-Leitung kommen erst so viel sp&auml;ter als PTT. Hold time = Haltezeit: PTT bleibt so lange nach dem letzten Zeichen aktiv.", "Apply to &ldquo;PTT via Serial Commands&rdquo; and &ldquo;PTT via Pin&rdquo;. Lead time: tone and key line come this much later than PTT. Hold time: PTT stays active this long after the last element."},
  {"Betriebsart w&auml;hlen (BLE-Tastatur, BLE-MIDI oder BLE aus) und speichern. Die Noten und Tasten darunter gelten jeweils nur im angegebenen Modus (der nicht aktive Bereich ist abgeblendet) und werden getrennt gespeichert.", "Select the mode (BLE keyboard, BLE MIDI or BLE off) and save. The notes and keys below only apply to the respective mode (the inactive area is dimmed) and are stored separately."},
  {"stehen f&uuml;r Zeilenende). Leer = nichts senden. Eine Baudrate gibt es bei USB nicht, im Terminalprogramm ist jeder Wert in Ordnung. Voraussetzung: USB-Serial ist aktiviert und der COM-Port am PC ge&ouml;ffnet.", "stand for line ending). Empty = send nothing. USB has no baud rate, any value is fine in the terminal program. Prerequisite: USB serial is enabled and the COM port is opened on the PC."},
  {"Sendet zus&auml;tzlich zu den USB-Tastaturbefehlen echte USB-MIDI-Noten &ndash; unabh&auml;ngig von Bluetooth und von den BLE-MIDI-Noten. Ein-/Ausschalten braucht einen Neustart, Notenaenderungen wirken sofort.", "Sends real USB MIDI notes in addition to the USB keyboard commands &ndash; independent of Bluetooth and of the BLE MIDI notes. Switching on/off needs a restart, note changes take effect immediately."},
  {"Die Frequenz entspricht der Einstellung &bdquo;Tone&ldquo; (Keyer). Der Piezo-Ton folgt dem Keyer sofort, ohne Lead time. Pins 19/20 (USB), 21 (LED) und die Paddle-Pins sind nicht erlaubt.", "The frequency follows the &ldquo;Tone&rdquo; setting (keyer). The piezo tone follows the keyer immediately, without lead time. Pins 19/20 (USB), 21 (LED) and the paddle pins are not allowed."},
  {"&Auml;nderungen ohne Neustart wirken sofort, sind aber erst nach dem Speichern dauerhaft &ndash; der Knopf blinkt, solange noch nicht gespeichert wurde. Ein Neustart erfolgt nur, wenn er n&ouml;tig ist (WLAN, USB-Tastatur, USB-MIDI, USB-Audio, Bluetooth-Betriebsart).", "Changes that need no restart take effect immediately but are only permanent after saving &ndash; the button blinks while there are unsaved changes. A restart only happens when it is needed (WiFi, USB keyboard, USB MIDI, USB audio, Bluetooth mode)."},
  {"Einstellungen gespeichert (", "Settings saved ("},
  {"). Das Ger&auml;t startet neu ...", "). The device is restarting ..."},
  {"Der ESP32 meldet sich als virtueller COM-Port (Statusleitungen und PTT-Befehle) und optional als USB-Mikrofon mit CW-Ton. Alles au&szlig;er dem Ein/Aus des Mikrofons wirkt sofort.", "The ESP32 appears as a virtual COM port (status lines and PTT commands) and optionally as a USB microphone with CW tone. Everything except the microphone on/off takes effect immediately."},
  {"Gilt nur f&uuml;r die Verbindung per USB-Kabel &ndash; unabh&auml;ngig vom Bluetooth-Modus. Ein-/Ausschalten braucht einen Neustart, die Tastenwahl wirkt sofort.", "Applies to the USB cable connection only &ndash; independent of the Bluetooth mode. Switching on/off needs a restart, the key selection takes effect immediately."},
  {"Ger&auml;t nicht erreichbar. Nach einer WLAN-&Auml;nderung hat sich evtl. die Adresse ge&auml;ndert.", "Device not reachable. After a WLAN change the address may have changed."},
  {"Gesperrt: der installierte Arduino-Core kann die Statusleitungen noch nicht setzen ", "Locked: the installed Arduino core cannot set the status lines yet "},
  {"Wird pro Verbindung mit dem ersten Ton gesendet. Nicht jede App reagiert darauf.", "Sent once per connection with the first note. Not every app reacts to it."},
  {"USB-Tastatur-Einstellung gespeichert. Das Ger&auml;t startet neu ...", "USB keyboard setting saved. The device is restarting ..."},
  {"Bluetooth-Einstellungen gespeichert. Das Ger&auml;t startet neu ...", "Bluetooth settings saved. The device is restarting ..."},
  {"Die Befehle werden als Text &uuml;ber den COM-Port gesendet (z.B. ", "The commands are sent as text over the COM port (e.g. "},
  {"USB-Audio-Einstellung gespeichert. Das Ger&auml;t startet neu ...", "USB audio setting saved. The device is restarting ..."},
  {"alert('Bluetooth Midi-Noten, Lautstaerke und Tasten gespeichert')", "alert('Bluetooth MIDI notes, volume and keys saved')"},
  {"USB-MIDI-Einstellung gespeichert. Das Ger&auml;t startet neu ...", "USB MIDI setting saved. The device is restarting ..."},
  {"WLAN-Einstellungen gespeichert. Das Ger&auml;t startet neu ...", "WLAN settings saved. The device is restarting ..."},
  {"Speichern (Neustart nur bei &Auml;nderung des Mikrofons)", "Save (restart only when the microphone setting changes)"},
  {"Sobald der Core das kann, wird die Auswahl freigegeben.", "As soon as the core can do this, the selection is enabled."},
  {"USB-Serial aktivieren (Leitungen, Commands und PTT)", "Enable USB serial (lines, commands and PTT)"},
  {"Bluetooth Midi-Noten und Bluetooth Tasten speichern", "Save Bluetooth MIDI notes and Bluetooth keys"},
  {"Nur im Modus BLE-MIDI: Noten &amp; Lautst&auml;rke", "BLE MIDI mode only: notes &amp; volume"},
  {"USB-Audio aktivieren (Neustart bei &Auml;nderung)", "Enable USB audio (restarts on change)"},
  {"Key (Keyer-Ausgang, um Lead time verz&ouml;gert)", "Key (keyer output, delayed by lead time)"},
  {"Nach dem Speichern startet das Ger&auml;t neu.", "The device restarts after saving."},
  {"Oder manuell eingeben (z.B. verstecktes Netz):", "Or enter manually (e.g. hidden network):"},
  {"USB-Tastatur Ein/Aus speichern & neu starten", "Save USB keyboard on/off & restart"},
  {"Wird sofort &uuml;bernommen (kein Neustart).", "Takes effect immediately (no restart)."},
  {"BLE-Midi / BLE-Tastatur Auswahl speichern", "Save BLE MIDI / BLE keyboard selection"},
  {"USB-MIDI Ein/Aus speichern & neu starten", "Save USB MIDI on/off & restart"},
  {"Notennamen nach Konvention C4 = 60.", "Note names follow the C4 = 60 convention."},
  {"alert('USB-MIDI-Noten gespeichert')", "alert('USB MIDI notes saved')"},
  {" (BLE-Tastatur, Original-Adresse)", " (BLE keyboard, original address)"},
  {"Nur im Modus BLE-Tastatur: Tasten", "BLE keyboard mode only: keys"},
  {"Piezo-Mith&ouml;rton aktivieren", "Enable piezo sidetone"},
  {"alert('USB-Tasten gespeichert')", "alert('USB keys saved')"},
  {"Taste Rechtes Paddle (Strich):", "Key right paddle (dash):"},
  {"Sleep-Timeout (Min, 0 = nie):", "Sleep timeout (min, 0 = never):"},
  {"Note Rechtes Paddle (Strich):", "Note right paddle (dash):"},
  {"Arduino-Core: Statusleitungen", "Arduino core: status lines"},
  {"COM-Port am PC ge&ouml;ffnet:", "COM port opened on PC:"},
  {"<p class='hint' style='color:#ffdd57;'><b>Kein Keyer:</b> Hier wird nur der Paddle-Status weitergereicht (gedr&uuml;ckt / losgelassen). Iambic-Keyer, Tempo und Farnsworth wirken nicht &ndash; die Zeichen formt die Gegenstelle.</p>", "<p class='hint' style='color:#ffdd57;'><b>No keyer:</b> Only the paddle state is passed on here (pressed / released). The iambic keyer, speed and Farnsworth have no effect &ndash; the receiving side forms the characters.</p>"},
  {"<p class='hint'>Dit/Dah geben den reinen Paddle-Status weiter (kein Keyer). Key ist der Ausgang des Keyers (Modus, Tempo und Farnsworth wirken hier).</p>", "<p class='hint'>Dit/Dah pass on the raw paddle state (no keyer). Key is the keyer output (mode, speed and Farnsworth apply).</p>"},
  {"Farnsworth: Die Zeichen werden mit dem Zeichentempo (Standard 18 WPM) gegeben, die Pausen zwischen den Zeichen so gedehnt, dass sich das oben eingestellte Tempo als Gesamttempo ergibt (Standardformel f&uuml;r das Wort PARIS). &Auml;ndert man das Tempo, wird die Dehnung automatisch neu berechnet. Der Keyer startet das n&auml;chste Zeichen nicht vor Ablauf des gedehnten Zeichenabstands. Ist das Tempo gleich oder h&ouml;her als das Zeichentempo, passiert nichts.", "Farnsworth: characters are sent at the character speed (standard 18 WPM); the gaps between characters are stretched so that the speed set above becomes the overall speed (standard formula for the word PARIS). When the speed is changed, the stretching is recalculated automatically. The keyer does not start the next character before the stretched character gap has elapsed. If the speed is equal to or higher than the character speed, nothing changes."},
  {"Sendepuffer belegt:", "TX buffer used:"},
  {"DTR ignorieren (Befehle auch senden, wenn der Port ohne DTR ge&ouml;ffnet ist)", "Ignore DTR (send commands even if the port is opened without DTR)"},
  {"Programme, die DTR nicht setzen (z.B. die SDR Console), brauchen &bdquo;DTR ignorieren&ldquo;; ist es abgeschaltet, wird nur bei gesetztem DTR gesendet.", "Programs that do not set DTR (e.g. SDR Console) need &ldquo;Ignore DTR&rdquo;; if it is off, commands are only sent while DTR is set."},
  {"Mikrofon am PC ge&ouml;ffnet:", "Microphone opened on PC:"},
  {"Sonderzeichen (mit Umschalt)", "Special characters (with Shift)"},
  {"Taste Linkes Paddle (Punkt):", "Key left paddle (dot):"},
  {"WLAN speichern & neu starten", "Save WLAN & restart"},
  {"Web-Timeout (Min, 0 = nie):", "Web timeout (min, 0 = never):"},
  {"Note Linkes Paddle (Punkt):", "Note left paddle (dot):"},
  {" (BLE-MIDI, eigene Adresse)", " (BLE MIDI, own address)"},
  {"Lautst&auml;rke (MIDI CC7):", "Volume (MIDI CC7):"},
  {"&acute; (Akzent, Tottaste)", "&acute; (accent, dead key)"},
  {"placeholder='SSID manuell'", "placeholder='SSID manual'"},
  {"Sonderzeichen (mit AltGr)", "Special characters (with AltGr)"},
  {"^ (Zirkumflex, Tottaste)", "^ (circumflex, dead key)"},
  {"USB-MIDI Noten speichern", "Save USB MIDI notes"},
  {"Einstellungen speichern", "Save settings"},
  {"USB-Tastatur aktivieren", "Enable USB keyboard"},
  {"Sonderzeichen (direkt)", "Special characters (direct)"},
  {"Paddles tauschen (L/R)", "Swap paddles (L/R)"},
  {"Mith&ouml;rton (Piezo)", "Sidetone (piezo)"},
  {">BLE-Tastatur</option>", ">BLE keyboard</option>"},
  {"PTT-Befehle: gesendet", "PTT commands: sent"},
  {"Warte auf Neustart...", "Waiting for restart..."},
  {"Umschalt+Bindestrich", "Shift+hyphen"},
  {"USB-Tastatur (Kabel)", "USB keyboard (cable)"},
  {"USB-Tasten speichern", "Save USB keys"},
  {"Diagnose: Keyer-Takt", "Diagnostics: keyer ticks"},
  {"USB-Audio (Mikrofon)", "USB audio (microphone)"},
  {"Dah (rechtes Paddle)", "Dah (right paddle)"},
  {"<h2>Gespeichert</h2>", "<h2>Saved</h2>"},
  {"alert('Gespeichert')", "alert('Saved')"},
  {"USB-MIDI aktivieren", "Enable USB MIDI"},
  {"Dit (linkes Paddle)", "Dit (left paddle)"},
  {"Bluetooth-Adresse: ", "Bluetooth address: "},
  {"-- Bitte waehlen --", "-- Please select --"},
  {"value='Neustart...'", "value='Restarting...'"},
  {"Rechts Strg (Ctrl)", "Right Ctrl"},
  {"Rechts Alt (AltGr)", "Right Alt (AltGr)"},
  {"Rechts Windows/Cmd", "Right Windows/Cmd"},
  {"&lt; (kleiner als)", "&lt; (less than)"},
  {"</code> und <code>", "</code> and <code>"},
  {"Links Strg (Ctrl)", "Left Ctrl"},
  {"Links Windows/Cmd", "Left Windows/Cmd"},
  {"&auml; (a-Umlaut)", "&auml; (a umlaut)"},
  {"&ouml; (o-Umlaut)", "&ouml; (o umlaut)"},
  {"&uuml; (u-Umlaut)", "&uuml; (u umlaut)"},
  {"Zuletzt gemeldet:", "Last reported:"},
  {">BLE aus</option>", ">BLE off</option>"},
  {"&szlig; (Eszett)", "&szlig; (sharp s)"},
  {"USB-MIDI (Kabel)", "USB MIDI (cable)"},
  {" (Bluetooth aus)", " (Bluetooth off)"},
  {"WLAN neu scannen", "Rescan WLAN"},
  {"Funktionstasten", "Function keys"},
  {"Rechts Umschalt", "Right Shift"},
  {"- (Bindestrich)", "- (hyphen)"},
  {"NICHT vorhanden", "NOT available"},
  {"WLAN-Verbindung", "WLAN connection"},
  {"Modifiertasten", "Modifier keys"},
  {"Links Umschalt", "Left Shift"},
  {"Umschalt+Komma", "Shift+comma"},
  {"Umschalt+Punkt", "Shift+period"},
  {"Umschalt+Plus", "Shift+plus"},
  {"Farbe Strich:", "Color dash:"},
  {">Aus</option>", ">Off</option>"},
  {"Sondertasten", "Special keys"},
  {"Pfeil runter", "Arrow down"},
  {"Pfeil rechts", "Arrow right"},
  {"Farbe Punkt:", "Color dot:"},
  {"Audio-Pakete", "audio packets"},
  {"Betriebsart:", "Mode:"},
  {"Pfeil links", "Arrow left"},
  {"Farbe Takt:", "Color heartbeat:"},
  {"<b>nein</b>", "<b>no</b>"},
  {"Buchstaben", "Letters"},
  {"Pfeil hoch", "Arrow up"},
  {"Links Alt", "Left Alt"},
  {"Leertaste", "Space"},
  {", (Komma)", ", (comma)"},
  {". (Punkt)", ". (period)"},
  {"# (Raute)", "# (hash)"},
  {"Umschalt+", "Shift+"},
  {"vorhanden", "available"},
  {"<b>ja</b>", "<b>yes</b>"},
  {"Ton-Taste", "tone key"},
  {"verworfen", "dropped"},
  {"GPIO-Pin:", "GPIO pin:"},
  {"Passwort:", "Password:"},
  {"Serial-Debug-Messages (Diagnose im seriellen Monitor)", "Serial debug messages (diagnostics in the serial monitor)"},
  {"Debug-Ausgaben und USB-Serial teilen sich denselben COM-Port, daher ist nur eines von beiden m&ouml;glich: Wird eines eingeschaltet, wird das andere ausgeschaltet.", "Debug output and USB serial share the same COM port, so only one of them is possible: switching one on switches the other off."},
  {"Diagnose: Laufzeit", "Diagnostics: uptime"},
  {"</b> Byte (Minimum <b>", "</b> bytes (minimum <b>"},
  {"gr&ouml;&szlig;ter Block", "largest block"},
  {"letzter Neustart:", "last restart:"},
  {"freier Speicher", "free memory"},
  {"im Scan gefunden: Kanal", "found in scan: channel"},
  {"im Scan <b>nicht</b> gefunden", "<b>not</b> found in scan"},
  {"(WLAN-Status", "(WiFi status"},
  {"Im Heimnetz ist das Ger&auml;t au&szlig;erdem unter dem Namen <b>morse-s3</b> bzw. <b>morse-s3.local</b> erreichbar. Wird ein Netz gew&auml;hlt, aber keine Verbindung hergestellt, startet das Ger&auml;t beim Speichern neu und versucht es erneut.", "In the home network the device is also reachable under the name <b>morse-s3</b> or <b>morse-s3.local</b>. If a network is selected but no connection is made, the device restarts on saving and tries again."},
  {"Status: Verbunden mit", "Status: Connected to"},
  {"Kein Heimnetz eingetragen &ndash; der ESP32 spannt sein eigenes Netz auf.", "No home network configured &ndash; the ESP32 provides its own network."},
  {"Nicht verbunden", "Not connected"},
  {"&ldquo;. Grund:", "&rdquo;. Reason:"},
  {"Netz nicht gefunden (SSID falsch geschrieben, zu schwach oder nur 5 GHz &ndash; der ESP32-S3 kann nur 2,4 GHz)", "Network not found (SSID misspelled, too weak or 5 GHz only &ndash; the ESP32-S3 supports 2.4 GHz only)"},
  {"Anmeldung abgelehnt (Passwort falsch oder Verschl&uuml;sselung nicht kompatibel, z.B. nur WPA3)", "Login rejected (wrong password or incompatible encryption, e.g. WPA3 only)"},
  {"Signal verloren (Beacon-Timeout)", "Signal lost (beacon timeout)"},
  {"Verbindung abgebrochen (Signal zu schwach oder gest&ouml;rt)", "Connection dropped (signal too weak or disturbed)"},
  {"kein Fehler gemeldet", "no error reported"},
  {"Fehlercode", "error code"},
  {"</b> mit &bdquo;", "</b> to &bdquo;"},
  {"+ (Plus)", "+ (plus)"},
  {"Tottaste", "dead key"},
  {"Ziffern", "Digits"},
  {"Netze:", "Networks:"},
};
String trans(String t) {
  if (uiLang != 1) return t;
  for (size_t i = 0; i < sizeof(TR_TABLE) / sizeof(TR_TABLE[0]); i++) t.replace(TR_TABLE[i].de, TR_TABLE[i].en);
  return t;
}
static int sendTCount = 0;
static bool sendAborted = false;   // Browser hat die Verbindung geschlossen -> restliche Teile nicht mehr aufbauen/senden
void sendT(const String& t) {
  if (sendAborted) return;
  server.sendContent(trans(t));
  sendTCount++;
  // Diagnose: bricht der Browser/das WLAN die Seite ab, steht Teil-Nummer und freier Speicher im seriellen Monitor (einmal)
  if (!server.client().connected()) {
    sendAborted = true;
    DBG("Seite abgebrochen bei Teil %d, Heap %u, WLAN-Status %d\n", sendTCount, (unsigned)ESP.getFreeHeap(), (int)WiFi.status());
  }
}
void sendAlert(const char* msg) {
  server.send(200, "text/html", trans(String("<script>alert('") + msg + "');window.location='/';</script>"));
}

static const char PAGE_HEAD[] = R"HTML(<!DOCTYPE html><html><head><meta charset='utf-8'><title>Morse</title>
<style>
body{font-family:sans-serif;background:#1a1a1a;color:#eee;padding:15px;}
form.lang,.card{background:#2d2d2d;padding:25px;border-radius:10px;max-width:400px;margin:30px auto;box-shadow:0 4px 15px rgba(0,0,0,0.3);}
h2{text-align:center;color:#00d1b2;margin-top:0;}div{margin-bottom:15px;}
input[type='text'],input[type='password'],input[type='number'],select{width:95%;padding:10px;background:#444;color:#fff;border:1px solid #555;border-radius:4px;font-size:14px;}
.row{display:flex;align-items:center;margin:20px 0;}
.btn{background:#00d1b2;color:#fff;border:0;padding:12px;width:100%;border-radius:4px;cursor:pointer;font-weight:bold;font-size:15px;}
.btn-orange{background:#ffdd57;color:#333;margin-top:5px;}
.hint{font-size:12px;color:#aaa;margin:0 0 15px 0;}
input:disabled,select:disabled{opacity:0.4;}
.savebar{position:sticky;bottom:10px;}
@keyframes blink{0%,100%{background:#ffdd57;box-shadow:0 0 0 0 rgba(255,221,87,0);}50%{background:#ff7a1a;box-shadow:0 0 12px 3px rgba(255,122,26,0.8);}}
.btn.dirty{animation:blink 1s infinite;color:#000;}
fieldset{border:1px solid #555;border-radius:6px;margin:0 0 15px 0;padding:12px 10px 0 10px;}
legend{color:#ffdd57;font-size:13px;font-weight:bold;padding:0 6px;}
</style></head><body>
)HTML";

// Eine Zeile "Beschriftung + Auswahlliste" fuer Tasten bzw. Noten
void sendKeyRow(const char* label, const char* name, const String& currentId, const char* defaultId) {
  sendT(String("<div>") + label + "<br>");
  sendT(buildKeySelectHtml(name, currentId, defaultId));
  sendT("</div>");
}
void sendMidiRow(const char* label, const char* name, int currentNote, int defaultNote) {
  sendT(String("<div>") + label + "<br>");
  sendT(buildMidiSelectHtml(name, currentNote, defaultNote));
  sendT("</div>");
}

void sendVolumeRow(const char* label, const char* name, int currentVol, int defaultVol) {
  sendT(String("<div>") + label + "<br>");
  sendT(buildVolumeSelectHtml(name, currentVol, defaultVol));
  sendT("</div>");
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
    setTimeout(function(){c.abort();},5000);
    fetch('/ping',{cache:'no-store',signal:c.signal}).then(function(r){
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
  server.send(200, "text/html", trans(html));
}

// Die Seite wird in Teilen (chunked) gesendet, da die Auswahllisten sehr gross sind
// Auswahlfeld fuer die Zuordnung eines Signals zu einer Statusleitung
// Text fuer ein HTML-Attribut (value='...') absichern
String htmlAttr(const String& in) {
  String o;
  for (unsigned int i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '\'') o += "&#39;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

void sendSerMapRow(const char* label, const char* name, int sel) {
  const char* names[4] = {"Aus", "DCD", "DSR", "Break"};
  String h = String("<div>") + label + "<br><select name='" + name + "'>";
  for (int i = 0; i < 4; i++) {
    h += String("<option value='") + i + "'" + (i == sel ? " selected" : "") + ">" + names[i] + "</option>";
  }
  h += "</select></div>";
  sendT(h);
}

// Text fuer ein JavaScript-String-Literal in "..." (Labels sind HTML mit Entities, kein Zeilenumbruch)
String jsEsc(const String& in) {
  String o;
  for (unsigned int i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '\\' || c == '"') o += '\\';
    o += c;
  }
  return o;
}

// Daten und Skript zum Befuellen der Auswahllisten (einmalig pro Seite, in kleinen Stuecken gesendet)
void sendSelectData() {
  String g = "<script>var G=[";
  String k = "var KO=[";
  const char* lastGroup = nullptr;
  int gi = -1;
  String h = "";
  for (size_t i = 0; i < keyOptions.size(); i++) {
    const KeyOption& o = keyOptions[i];
    if (lastGroup == nullptr || strcmp(lastGroup, o.group) != 0) {
      if (gi >= 0) g += ",";
      g += String("\"") + jsEsc(String(o.group)) + "\"";
      gi++;
      lastGroup = o.group;
    }
    k += String("[\"") + jsEsc(o.id) + "\",\"" + jsEsc(o.label) + "\"," + gi + "],";
  }
  sendT(g + "];\n");
  // Key-Optionen in Stuecken von ca. 1 KB
  String chunk = "";
  size_t pos = 0;
  while (pos < k.length()) {
    int end = k.indexOf("],", pos);
    if (end < 0) end = k.length() - 2;
    chunk += k.substring(pos, end + 2);
    pos = end + 2;
    if (chunk.length() > 1000) { sendT(chunk); chunk = ""; }
  }
  if (chunk.length() > 0) sendT(chunk);
  String vv = "];\nvar VV=[";
  for (int st = 20; st >= 0; st--) vv += String(st == 20 ? "" : ",") + "[" + volumeFromStep(st) + "," + (st * 5) + "]";
  vv += "];\n";
  sendT(vv);
  sendT(R"HTML(function fill(){
var N=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
var s=document.querySelectorAll('select[data-t]');
for(var i=0;i<s.length;i++){
var e=s[i],t=e.getAttribute('data-t'),v=e.getAttribute('data-v'),d=e.getAttribute('data-d'),h='',j;
if(t=='k'){var lg=-1;
for(j=0;j<KO.length;j++){var o=KO[j];
if(o[2]!=lg){if(lg>=0)h+='</optgroup>';h+='<optgroup label="'+G[o[2]]+'">';lg=o[2];}
h+='<option value="'+o[0]+'"'+(o[0]==v?' selected':'')+'>'+o[1]+(o[0]==d?' (default)':'')+'</option>';}
if(lg>=0)h+='</optgroup>';}
else if(t=='m'){for(j=0;j<128;j++)h+='<option value="'+j+'"'+(j==v?' selected':'')+'>'+j+' ('+N[j%12]+(Math.floor(j/12)-1)+')'+(j==d?' (default)':'')+'</option>';}
else{for(j=0;j<VV.length;j++)h+='<option value="'+VV[j][0]+'"'+(VV[j][0]==v?' selected':'')+'>'+VV[j][1]+' % ('+VV[j][0]+')'+(VV[j][0]==d?' (default)':'')+'</option>';}
e.innerHTML=h;}
}
fill();
</script>
)HTML");
  // Sofort-Uebernahme: jede Aenderung geht per fetch an /apply (wirkt ohne Speichern, ohne Neustart);
  // der Speichern-Knopf blinkt, bis dauerhaft gespeichert wurde (Submit)
  sendT(R"HTML(<script>
(function(){
var f=document.querySelector("form[action='/save_all']");if(!f)return;
var b=f.querySelector("input[type=submit]"),busy=false,again=false,tm=null;
function dirty(){if(b)b.classList.add('dirty');}
function send(){
if(busy){again=true;return;}
busy=true;
fetch('/apply',{method:'POST',body:new URLSearchParams(new FormData(f))})
.catch(function(){}).then(function(){busy=false;if(again){again=false;send();}});
}
function later(){clearTimeout(tm);tm=setTimeout(send,150);}
f.addEventListener('input',function(e){
dirty();
if(e.target.type=='range')later();
});
f.addEventListener('change',function(e){dirty();later();});
f.addEventListener('submit',function(){if(b)b.classList.remove('dirty');});
})();
</script>
)HTML");
}

void handleRoot() {
  lastWebActivityTime = millis();
  sendTCount = 0;
  sendAborted = false;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(PAGE_HEAD);

  // ---- Sprachwahl ----
  {
    String l = "<form class='card' action='/save_lang' method='POST' style='padding:10px 25px;'><div style='margin:0;'>Sprache / Language:<br><select name='lang' onchange='this.form.submit()'>";
    l += String("<option value='0'") + (uiLang == 0 ? " selected" : "") + ">Deutsch</option>";
    l += String("<option value='1'") + (uiLang == 1 ? " selected" : "") + ">English</option></select></div></form>\n";
    server.sendContent(l);
  }

  // ---- Formular 1: Allgemeine Einstellungen ----
  String html = "<form action='/save_all' method='POST' autocomplete='off'><div class='card'><h2>Morse-S3 Config</h2>";
  html += "<div>Debounce (ms):<br><input type='number' name='debounce' value='" + String(debounceTime) + "'></div>";

  String checkStr = swapPaddles ? "checked" : "";
  html += "<div class='row'><input type='checkbox' name='swap' style='width:auto;margin-right:10px;' " + checkStr + "><label>Paddles tauschen (L/R)</label></div>";

  html += "<div>Sleep-Timeout (Min, 0 = nie):<br><input type='number' name='timeout' min='0' value='" + String(sleepTimeoutMs / 60000) + "'></div>";
  html += "<div>Web-Timeout (Min, 0 = nie):<br><input type='number' name='web_timeout' min='0' value='" + String(webTimeoutMs / 60000) + "'></div>";
  html += "<div>Farbe Takt:<br><input type='color' name='c_heart' value='" + getHexStr(colorHeartbeat) + "'></div>";
  html += "<div>Farbe Punkt:<br><input type='color' name='c_dot' value='" + getHexStr(colorDot) + "'></div>";
  html += "<div>Farbe Strich:<br><input type='color' name='c_dash' value='" + getHexStr(colorDash) + "'></div>";
  html += "</div>\n";
  sendT(html);

  // ---- Formular 2: USB-Tastatur (eigene Belegung, nur USB) ----
  sendT(
    "<div class='card'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>USB-Tastatur (Kabel)</h3>"
    "<p class='hint' style='color:#ffdd57;'><b>Kein Keyer:</b> Hier wird nur der Paddle-Status weitergereicht (gedr&uuml;ckt / losgelassen). Iambic-Keyer, Tempo und Farnsworth wirken nicht &ndash; die Zeichen formt die Gegenstelle.</p>"
    "<p class='hint'>Gilt nur f&uuml;r die Verbindung per USB-Kabel &ndash; unabh&auml;ngig vom Bluetooth-Modus. "
    "Ein-/Ausschalten braucht einen Neustart, die Tastenwahl wirkt sofort.</p>");
  {
    String kbdChk = usbKeyboardEnabled ? "checked" : "";
    sendT("<div class='row'><input type='checkbox' name='usbkbd_en' style='width:auto;margin-right:10px;' " + kbdChk + "><label>USB-Tastatur aktivieren</label></div>");
  }
  sendKeyRow("Taste Linkes Paddle (Punkt):", "usb_left", usbKeyLeftId, DEFAULT_KEY_LEFT);
  sendKeyRow("Taste Rechtes Paddle (Strich):", "usb_right", usbKeyRightId, DEFAULT_KEY_RIGHT);
  sendT("</div>\n");

  // ---- Formular 2b: USB-MIDI (zusaetzlich zur USB-Tastatur, unabhaengig von Bluetooth) ----
  sendT(
    "<div class='card'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>USB-MIDI (Kabel)</h3>"
    "<p class='hint' style='color:#ffdd57;'><b>Kein Keyer:</b> Hier wird nur der Paddle-Status weitergereicht (gedr&uuml;ckt / losgelassen). Iambic-Keyer, Tempo und Farnsworth wirken nicht &ndash; die Zeichen formt die Gegenstelle.</p>"
    "<p class='hint'>Sendet zus&auml;tzlich zu den USB-Tastaturbefehlen echte USB-MIDI-Noten &ndash; "
    "unabh&auml;ngig von Bluetooth und von den BLE-MIDI-Noten. Ein-/Ausschalten braucht einen Neustart, "
    "Notenaenderungen wirken sofort.</p>"
    "<p class='hint' style='color:#ffdd57;'><b>Achtung:</b> Sind USB-Tastatur UND USB-MIDI gleichzeitig "
    "aktiviert, kann es nach einem automatischen Einschlafen (Sleep-Timeout) und Aufwachen vorkommen, dass "
    "sich die USB-Treiber am PC aufh&auml;ngen &ndash; Aus-/Einstecken hilft dann nicht mehr, nur ein "
    "PC-Neustart. Setze in diesem Fall den Sleep-Timeout oben auf 0 (nie). Das WLAN/Webinterface ist davon "
    "nicht betroffen.</p>");
  {
    String midiChk = usbMidiEnabled ? "checked" : "";
    sendT("<div class='row'><input type='checkbox' name='usbmidi_en' style='width:auto;margin-right:10px;' " + midiChk + "><label>USB-MIDI aktivieren</label></div>");
  }
  sendMidiRow("Note Linkes Paddle (Punkt):", "usbmidi_dot", usbMidiNoteDot, DEFAULT_NOTE_DOT);
  sendMidiRow("Note Rechtes Paddle (Strich):", "usbmidi_dash", usbMidiNoteDash, DEFAULT_NOTE_DASH);
  sendT("</div>\n");

  // ---- Formular 2c: USB-Serial, USB-Audio, Keyer und PTT in einem Abschnitt ----
  sendT(
    "<div class='card'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>USB-Serial &amp; USB-Audio</h3>"
    "<p class='hint'>Der ESP32 meldet sich als virtueller COM-Port (Statusleitungen und PTT-Befehle) und optional als "
    "USB-Mikrofon mit CW-Ton. Alles au&szlig;er dem Ein/Aus des Mikrofons wirkt sofort.</p>");
  sendT(String("<p class='hint'>Arduino-Core: Statusleitungen <b>") +
                     (!CdcNotify::available ? "NICHT vorhanden"
                      : (String(CDC_NOTIFY_LIB) == "0" || (serNotifyOk == 0 && serNotifyFail > 0)) ? "nur deklariert, aber ohne Funktion (Core-Bibliotheken ohne CFG_TUD_CDC_NOTIFY)"
                      : "vorhanden") +
                     " (CFG_TUD_CDC_NOTIFY=" + String(CDC_NOTIFY_LIB) + ")" +
                     "</b>, USB-Audio <b>" + (MORSE_HAS_AUDIO ? "vorhanden" : "NICHT vorhanden") +
                     "</b><br>COM-Port am PC ge&ouml;ffnet: <b>" + (serPortOpen ? "ja" : "nein") +
                     "</b>, Sendepuffer belegt: <b>" + String((int)serTxPending) + "</b> Byte, Mikrofon am PC ge&ouml;ffnet: <b>" + (audioStreaming ? "ja" : "nein") +
                     "</b><br>Leitungen vom PC: DTR <b>" + (serDtrRaw ? 1 : 0) + "</b> RTS <b>" + (serRtsRaw ? 1 : 0) +
                     "</b> (Ereignisse <b>" + String((unsigned long)serLineEvents) + "</b>)"
                     "<br>Direkt von TinyUSB: DTR <b>" + ((tud_cdc_n_get_line_state(0) & 1) ? 1 : 0) +
                     "</b> RTS <b>" + ((tud_cdc_n_get_line_state(0) & 2) ? 1 : 0) + "</b><b>" +
                     "</b><br>Zuletzt gemeldet: DCD=<b>" + (serSentDcd ? 1 : 0) + "</b> DSR=<b>" + (serSentDsr ? 1 : 0) +
                     "</b> Break=<b>" + (serSentBrk ? 1 : 0) +
                     "</b>, Meldungen: ok <b>" + String((unsigned long)serNotifyOk) + "</b>, fehlgeschlagen <b>" + String((unsigned long)serNotifyFail) +
                     "</b><br>Diagnose: Keyer-Takt <b>" + String((unsigned long)keyerTicks) + "</b>, Audio-Pakete <b>" +
                     String((unsigned long)audioFramesSent) + "</b>, Paddles (Dit/Dah) <b>" + (paddleDot ? 1 : 0) + "/" + (paddleDash ? 1 : 0) +
                     "</b>, Ton-Taste <b>" + (audioKeyed ? 1 : 0) +
                     "</b><br>PTT-Befehle: gesendet <b>" + String((unsigned long)serCmdSent) + "</b>, verworfen <b>" + String((unsigned long)serCmdDropped) + "</b></p>");

  // -- Keyer (Modus, Tempo, Ton) --
  sendT("<h4 style='color:#ffdd57;margin-bottom:6px;'>Keyer</h4>");
  {
    String km = "<div>Mode:<br><select name='kmode'>";
    km += String("<option value='1'") + (keyerMode == 1 ? " selected" : "") + ">Iambic A</option>";
    km += String("<option value='2'") + (keyerMode == 2 ? " selected" : "") + ">Iambic B</option>";
    km += String("<option value='0'") + (keyerMode == 0 ? " selected" : "") + ">Straight key (paddle direct)</option>";
    km += "</select></div>";
    sendT(km);
  }
  sendT(
    "<div>Speed (WPM, 5 - 60):<br><input type='number' name='kwpm' min='5' max='60' value='" + String(keyerWpm) + "'></div>"
    "<div class='row'><input type='checkbox' id='kfarn' name='kfarn' onchange='fUpd()' style='width:auto;margin-right:10px;' " + String(keyerFarn ? "checked" : "") + "><label>Farnsworth</label></div>"
    "<div>Character speed (WPM, 5 - 60):<br><input type='number' id='kcwpm' name='kcwpm' min='5' max='60' value='" + String(keyerCharWpm) + "'" + (keyerFarn ? "" : " disabled") + "></div>"
    "<p class='hint'>Farnsworth: Die Zeichen werden mit dem Zeichentempo (Standard 18 WPM) gegeben, die Pausen zwischen den Zeichen so gedehnt, dass sich das oben eingestellte Tempo als Gesamttempo ergibt (Standardformel f&uuml;r das Wort PARIS). &Auml;ndert man das Tempo, wird die Dehnung automatisch neu berechnet. Der Keyer startet das n&auml;chste Zeichen nicht vor Ablauf des gedehnten Zeichenabstands. Ist das Tempo gleich oder h&ouml;her als das Zeichentempo, passiert nichts.</p>"
    "<script>function fUpd(){document.getElementById('kcwpm').disabled=!document.getElementById('kfarn').checked;}</script>"
    "<div>Tone (Hz, 300 - 1500):<br><input type='number' name='aud_freq' min='300' max='1500' value='" + String(audioFreqHz) + "'></div>"
    "<div>Rise time (ms, 1 - 20):<br><input type='number' name='aud_ramp' min='1' max='20' value='" + String(audioRampMs) + "'></div>");

  // -- Piezo-Mithoerton --
  sendT("<h4 style='color:#ffdd57;margin-bottom:6px;'>Mith&ouml;rton (Piezo)</h4>");
  {
    String pzChk = piezoEnabled ? "checked" : "";
    String pzDis = piezoEnabled ? "" : " disabled";
    sendT("<div class='row'><input type='checkbox' id='pz_en' name='pz_en' onchange='pzUpd()' style='width:auto;margin-right:10px;' " + pzChk + "><label>Piezo-Mith&ouml;rton aktivieren</label></div>"
          "<div>Volume: <b><span id='pz_vol_v'>" + String(piezoVolumePct) + "</span> %</b><br><input type='range' style='width:95%;' id='pz_vol' name='pz_vol' min='1' max='100' value='" + String(piezoVolumePct) + "' oninput='pz_vol_v.textContent=this.value'" + pzDis + "></div>"
          "<div>GPIO-Pin:<br><input type='number' id='pz_pin' name='pz_pin' min='1' max='48' value='" + String(piezoPin) + "'" + pzDis + "></div>"
          "<p class='hint'>Die Frequenz entspricht der Einstellung &bdquo;Tone&ldquo; (Keyer). Der Piezo-Ton folgt dem Keyer sofort, ohne Lead time. "
          "Pins 19/20 (USB), 21 (LED) und die Paddle-Pins sind nicht erlaubt.</p>"
          "<script>function pzUpd(){var e=document.getElementById('pz_en').checked;document.getElementById('pz_vol').disabled=!e;document.getElementById('pz_pin').disabled=!e;}</script>");
  }

  // -- USB-Audio --
  sendT("<h4 style='color:#ffdd57;margin-bottom:6px;'>USB-Audio (Mikrofon)</h4>");
  {
    String audChk = usbAudioEnabled ? "checked" : "";
    sendT("<div class='row'><input type='checkbox' name='usbaud_en' style='width:auto;margin-right:10px;' " + audChk + "><label>USB-Audio aktivieren (Neustart bei &Auml;nderung)</label></div>");
  }
  sendT("<div>Volume: <b><span id='aud_vol_v'>" + String(audioVolumePct) + "</span> %</b><br><input type='range' style='width:95%;' name='aud_vol' min='1' max='100' value='" + String(audioVolumePct) + "' oninput='aud_vol_v.textContent=this.value'></div>");

  // -- USB-Serial: Leitungen der Paddles und der Taste --
  sendT("<h4 style='color:#ffdd57;margin-bottom:6px;'>USB-Serial</h4>");
  {
    String serChk = usbSerialEnabled ? "checked" : "";
    sendT("<div class='row'><input type='checkbox' id='usbser_en' name='usbser_en' onchange='dUpd(1)' style='width:auto;margin-right:10px;' " + serChk + "><label>USB-Serial aktivieren (Leitungen, Commands und PTT)</label></div>"
          "<div class='row'><input type='checkbox' id='dbg_en' name='dbg_en' onchange='dUpd(0)' style='width:auto;margin-right:10px;' " + String(serialDebug ? "checked" : "") + "><label>Serial-Debug-Messages (Diagnose im seriellen Monitor)</label></div>"
          "<p class='hint'>Debug-Ausgaben und USB-Serial teilen sich denselben COM-Port, daher ist nur eines von beiden m&ouml;glich: Wird eines eingeschaltet, wird das andere ausgeschaltet.</p>"
          "<script>function dUpd(w){var d=document.getElementById('dbg_en'),u=document.getElementById('usbser_en');if(w==0&&d.checked)u.checked=false;if(w==1&&u.checked)d.checked=false;}</script>");
  }
  sendT("<p class='hint'>Dit/Dah geben den reinen Paddle-Status weiter (kein Keyer). Key ist der Ausgang des Keyers (Modus, Tempo und Farnsworth wirken hier).</p>");
  sendSerMapRow("Dit (linkes Paddle) &rarr;", "ser_dot", serMapDot);
  sendSerMapRow("Dah (rechtes Paddle) &rarr;", "ser_dash", serMapDash);
  sendSerMapRow("Key (Keyer-Ausgang, um Lead time verz&ouml;gert) &rarr;", "ser_key", serMapKey);

  // -- PTT via Serial Commands (direkt darunter: PTT via Pin) --
  sendT(
    "<h4 style='color:#ffdd57;margin-bottom:6px;'>PTT via Serial Commands (CAT Interface)</h4>"
    "<div>TX command:<br><input type='text' name='ser_tx' value='" + htmlAttr(serTxCmd) + "'></div>"
    "<div>RX command:<br><input type='text' name='ser_rx' value='" + htmlAttr(serRxCmd) + "'></div>"
    "<div class='row'><input type='checkbox' name='ser_nodtr' style='width:auto;margin-right:10px;' " + String(serIgnoreDtr ? "checked" : "") + "><label>DTR ignorieren (Befehle auch senden, wenn der Port ohne DTR ge&ouml;ffnet ist)</label></div>"
    "<p class='hint'>Die Befehle werden als Text &uuml;ber den COM-Port gesendet (z.B. <code>TX;</code> / <code>RX;</code>; "
    "<code>\\r</code> und <code>\\n</code> stehen f&uuml;r Zeilenende). Leer = nichts senden. Eine Baudrate gibt es bei "
    "USB nicht, im Terminalprogramm ist jeder Wert in Ordnung. Voraussetzung: USB-Serial ist aktiviert und der COM-Port am PC ge&ouml;ffnet. Programme, die DTR nicht setzen (z.B. die SDR Console), brauchen &bdquo;DTR ignorieren&ldquo;; ist es abgeschaltet, wird nur bei gesetztem DTR gesendet.</p>");

  // -- PTT via Pin --
  sendT("<h4 style='color:#ffdd57;margin-bottom:6px;'>PTT via Pin</h4>");
  {
    const char* names[4] = {"Aus", "DCD", "DSR", "Break"};
    String sel = String("<div>PTT &rarr;<br><select name='ser_ptt'") + (CdcNotify::available ? "" : " disabled") + ">";
    for (int i = 0; i < 4; i++) {
      sel += String("<option value='") + i + "'" + (i == serMapPtt ? " selected" : "") + ">" + names[i] + "</option>";
    }
    sel += "</select></div>";
    if (!CdcNotify::available) {
      sel += "<p class='hint'>Gesperrt: der installierte Arduino-Core kann die Statusleitungen noch nicht setzen "
             "(<code>USBCDC::notifyState()</code>, PR #13037). Sobald der Core das kann, wird die Auswahl freigegeben.</p>";
    }
    sendT(sel);
  }
  // -- PTT Timings (gelten fuer beide PTT-Methoden) --
  sendT(
    "<h4 style='color:#ffdd57;margin-bottom:6px;'>PTT Timings</h4>"
    "<div>Lead time (ms, 0 - 1000):<br><input type='number' name='klead' min='0' max='1000' value='" + String(keyLeadMs) + "'></div>"
    "<div>Hold time (ms, 0 - 5000):<br><input type='number' name='ser_hang' min='0' max='5000' value='" + String(serPttHangMs) + "'></div>"
    "<p class='hint'>Gelten f&uuml;r &bdquo;PTT via Serial Commands&ldquo; und &bdquo;PTT via Pin&ldquo;. Lead time = Vorlauf: Ton und "
    "Key-Leitung kommen erst so viel sp&auml;ter als PTT. Hold time = Haltezeit: PTT bleibt so lange nach dem letzten Zeichen aktiv.</p>");
  sendT("</div>\n");

  // ---- Formular 3: Bluetooth (Modus + getrennte Belegung fuer MIDI bzw. Tastatur) ----
  sendT(
    "<div class='card'>"
    "<h3 style='color:#ffdd57;margin-top:0;'>Bluetooth</h3>"
    "<p class='hint' style='color:#ffdd57;'><b>Kein Keyer:</b> Hier wird nur der Paddle-Status weitergereicht (gedr&uuml;ckt / losgelassen). Iambic-Keyer, Tempo und Farnsworth wirken nicht &ndash; die Zeichen formt die Gegenstelle.</p>"
    "<p class='hint'>Betriebsart w&auml;hlen (BLE-Tastatur, BLE-MIDI oder BLE aus) und speichern. Die Noten und Tasten darunter gelten jeweils nur im "
    "angegebenen Modus (der nicht aktive Bereich ist abgeblendet) und werden getrennt gespeichert.</p>"
    "<p class='hint'><b>Hinweis:</b> BLE-MIDI und BLE-Tastatur funktionieren nicht gleichzeitig. Es ist immer nur ein "
    "Ger&auml;t verbunden &ndash; wer sich zuerst verbindet, belegt das Ger&auml;t. Im MIDI-Modus nutzt das Ger&auml;t "
    "eine eigene Bluetooth-Adresse, damit sich ein fr&uuml;her als Tastatur gekoppeltes Ger&auml;t nicht "
    "versehentlich verbindet. Beide Modi verlangen Pairing (Kopplung) &ndash; danach verbinden sich unterst&uuml;tzte "
    "Ger&auml;te (z.B. iOS 16+/macOS 13+) beim Einschalten automatisch wieder. Ob die jeweilige App das nutzt, "
    "h&auml;ngt von der App ab.</p>");

  String bt = "<div>Betriebsart:<br><select name='bt_mode' onchange='upd()'>";
  bt += String("<option value='1'") + (btMode == 1 ? " selected" : "") + ">BLE-Tastatur</option>";
  bt += String("<option value='0'") + (btMode == 0 ? " selected" : "") + ">BLE-MIDI</option>";
  bt += String("<option value='2'") + (btMode == 2 ? " selected" : "") + ">BLE aus</option>";
  bt += "</select></div>";
  sendT(bt);

  sendT(String("<p class='hint'>Bluetooth-Adresse: ") + btAddressText +
                     (btMode == 2 ? " (Bluetooth aus)" : (btMode == 1 ? " (BLE-Tastatur, Original-Adresse)" : " (BLE-MIDI, eigene Adresse)")) + "</p>");

  sendT("<fieldset id='fs_midi'><legend>Nur im Modus BLE-MIDI: Noten &amp; Lautst&auml;rke</legend>");
  sendMidiRow("Note Linkes Paddle (Punkt):", "midi_dot", midiNoteDot, DEFAULT_NOTE_DOT);
  sendMidiRow("Note Rechtes Paddle (Strich):", "midi_dash", midiNoteDash, DEFAULT_NOTE_DASH);
  sendT("<p class='hint'>Notennamen nach Konvention C4 = 60.</p>");
  sendVolumeRow("Lautst&auml;rke (MIDI CC7):", "midi_vol", midiVolume, DEFAULT_MIDI_VOLUME);
  sendT("<p class='hint'>Wird pro Verbindung mit dem ersten Ton gesendet. Nicht jede App reagiert darauf.</p></fieldset>");

  sendT("<fieldset id='fs_kbd'><legend>Nur im Modus BLE-Tastatur: Tasten</legend>");
  sendKeyRow("Taste Linkes Paddle (Punkt):", "bt_left", bleKeyLeftId, DEFAULT_KEY_LEFT);
  sendKeyRow("Taste Rechtes Paddle (Strich):", "bt_right", bleKeyRightId, DEFAULT_KEY_RIGHT);
  sendT("</fieldset>");

  sendT(R"HTML(</div>
<script>
function upd(){
  var m=document.querySelector("select[name=bt_mode]").value;
  document.getElementById('fs_midi').style.opacity=(m=='0')?1:0.4;
  document.getElementById('fs_kbd').style.opacity=(m=='1')?1:0.4;
}
upd();
</script>
)HTML");

  // ---- Formular 4: WLAN-Verbindung ----
  // SSID-Namen und Passwort werden nie uebersetzt (nur die festen Texte drumherum)
  String scan = scanResultsHtml;
  if (uiLang == 1) scan.replace("-- Bitte waehlen --", "-- Please select --");
  {
    String wst;
    if (WiFi.status() == WL_CONNECTED) wst = "Verbunden mit <b>" + htmlAttr(WiFi.SSID()) + "</b>, IP <b>" + WiFi.localIP().toString() + "</b>";
    else if (ssidHome.length() == 0) wst = "Kein Heimnetz eingetragen &ndash; der ESP32 spannt sein eigenes Netz auf.";
    else wst = "<b>Nicht verbunden</b> mit &bdquo;" + htmlAttr(ssidHome) + "&ldquo;. Grund: " + wifiReasonText() + " (WLAN-Status " + String((int)WiFi.status()) + "; " + wifiScanInfo + ")";
    sendT("<div class='card'>"
          "<h3 style='color:#ffdd57;margin-top:0;'>WLAN-Verbindung</h3>"
          "<p class='hint'>Status: " + wst + "</p>"
          "<p class='hint'>Diagnose: Laufzeit <b>" + String((unsigned long)(millis() / 1000)) + "</b> s, freier Speicher <b>" + String((unsigned long)ESP.getFreeHeap()) +
          "</b> Byte (Minimum <b>" + String((unsigned long)ESP.getMinFreeHeap()) + "</b>, gr&ouml;&szlig;ter Block <b>" + String((unsigned long)ESP.getMaxAllocHeap()) +
          "</b>), letzter Neustart: <b>" + resetReasonText() + "</b></p><div>Netze:<br>");
  }
  if (scan.length() > 0) server.sendContent(scan);   // leerer Inhalt wuerde die chunked-Antwort beenden!
  sendT("</div><button type='button' onclick='location.href=\"/rescan\"' class='btn btn-orange' style='margin-bottom:15px;'>WLAN neu scannen</button>"
        "<div>Oder manuell eingeben (z.B. verstecktes Netz):<br><input type='text' name='ssid_manual' placeholder='SSID manuell' autocapitalize='off' autocorrect='off' autocomplete='off' spellcheck='false'></div>"
        "<div>Passwort:<br><input type='password' name='pass_h' value='");
  // Passwort HTML-sicher, nie leer senden: sendContent("") beendet die chunked-Antwort -> Rest der Seite (u.a. der Speichern-Knopf) fehlte
  if (passHome.length() > 0) server.sendContent(htmlAttr(passHome));
  sendT("'></div>"
        "<p class='hint'>Im Heimnetz ist das Ger&auml;t au&szlig;erdem unter dem Namen <b>morse-s3</b> bzw. <b>morse-s3.local</b> erreichbar. Wird ein Netz gew&auml;hlt, aber keine Verbindung hergestellt, startet das Ger&auml;t beim Speichern neu und versucht es erneut.</p></div>\n"
        "<div class='card savebar'><input type='submit' class='btn btn-orange' value='Einstellungen speichern'>"
        "<p class='hint' style='margin:8px 0 0 0;'>&Auml;nderungen ohne Neustart wirken sofort, sind aber erst nach dem Speichern dauerhaft &ndash; der Knopf blinkt, solange noch nicht gespeichert wurde. "
        "Ein Neustart erfolgt nur, wenn er n&ouml;tig ist (WLAN, USB-Tastatur, USB-MIDI, USB-Audio, Bluetooth-Betriebsart).</p></div></form>");
  sendSelectData();
  sendT("</body></html>");
  sendT("");  // Ende der chunked-Antwort
}


void handleRescan() {
lastWebActivityTime = millis();
scanNetworks();
server.sendHeader("Location", "/");
server.send(303);
}
// ---- Sprache der Weboberflaeche speichern ----
void handleSaveLang() {
  lastWebActivityTime = millis();
  if (server.hasArg("lang")) uiLang = constrain((int)server.arg("lang").toInt(), 0, 1);
  saveSettings();
  server.sendHeader("Location", "/");
  server.send(303);
}

// ---- Alle Einstellungen auf einmal speichern ----
// Ein Neustart erfolgt nur, wenn sich etwas geaendert hat, das ihn braucht:
// WLAN-Zugang, USB-Tastatur/-MIDI/-Audio ein/aus oder die Bluetooth-Betriebsart.
// (Der Flash-Speicher wird nur beschrieben, wenn sich ein Wert tatsaechlich geaendert hat.)
// applyWebArgs(live=true):  uebernimmt nur Einstellungen OHNE Neustart sofort in den RAM (Route /apply, kein Flash-Schreiben).
// applyWebArgs(live=false): alles, inkl. Neustart-Einstellungen (Route /save_all); 'why' sammelt die Neustart-Gruende.
void applyWebArgs(bool live, String& why) {
  auto needRestart = [&](const char* what) { if (why.length() > 0) why += ", "; why += what; };

  // Allgemein
  if (server.hasArg("debounce")) debounceTime = server.arg("debounce").toInt();
  swapPaddles = server.hasArg("swap");
  if (server.hasArg("timeout")) sleepTimeoutMs = server.arg("timeout").toInt() * 60000;
  if (server.hasArg("web_timeout")) webTimeoutMs = server.arg("web_timeout").toInt() * 60000;
  if (server.hasArg("c_heart")) colorHeartbeat = parseHexColor(server.arg("c_heart"));
  if (server.hasArg("c_dot")) colorDot = parseHexColor(server.arg("c_dot"));
  if (server.hasArg("c_dash")) colorDash = parseHexColor(server.arg("c_dash"));

  // USB-Tastatur
  bool kbd = server.hasArg("usbkbd_en");
  if (!live && kbd != usbKeyboardEnabled) { usbKeyboardEnabled = kbd; needRestart(uiLang == 1 ? "USB keyboard" : "USB-Tastatur"); }
  if (server.hasArg("usb_left")) usbKeyLeftId = server.arg("usb_left");
  if (server.hasArg("usb_right")) usbKeyRightId = server.arg("usb_right");

  // USB-MIDI
  bool umidi = server.hasArg("usbmidi_en");
  if (!live && umidi != usbMidiEnabled) { usbMidiEnabled = umidi; needRestart(uiLang == 1 ? "USB MIDI" : "USB-MIDI"); }
  if (server.hasArg("usbmidi_dot")) usbMidiNoteDot = constrain((int)server.arg("usbmidi_dot").toInt(), 0, 127);
  if (server.hasArg("usbmidi_dash")) usbMidiNoteDash = constrain((int)server.arg("usbmidi_dash").toInt(), 0, 127);

  // Keyer, Piezo, USB-Audio, USB-Serial, PTT
  serialDebug = server.hasArg("dbg_en");
  usbSerialEnabled = server.hasArg("usbser_en") && !serialDebug;   // beide teilen sich den COM-Port: nur eines von beiden
  if (server.hasArg("ser_dot")) serMapDot = constrain((int)server.arg("ser_dot").toInt(), 0, 3);
  if (server.hasArg("ser_dash")) serMapDash = constrain((int)server.arg("ser_dash").toInt(), 0, 3);
  if (server.hasArg("ser_key")) serMapKey = constrain((int)server.arg("ser_key").toInt(), 0, 3);
  if (server.hasArg("ser_ptt")) serMapPtt = constrain((int)server.arg("ser_ptt").toInt(), 0, 3);   // fehlt, solange gesperrt
  if (server.hasArg("ser_tx")) serTxCmd = server.arg("ser_tx");
  if (server.hasArg("ser_rx")) serRxCmd = server.arg("ser_rx");
  serCmdActive = (serTxCmd.length() > 0 || serRxCmd.length() > 0);
  serIgnoreDtr = server.hasArg("ser_nodtr");
  if (server.hasArg("ser_hang")) serPttHangMs = constrain((int)server.arg("ser_hang").toInt(), 0, 5000);
  if (server.hasArg("klead")) keyLeadMs = constrain((int)server.arg("klead").toInt(), 0, 1000);
  if (server.hasArg("kmode")) keyerMode = constrain((int)server.arg("kmode").toInt(), 0, 2);
  if (server.hasArg("kwpm")) keyerWpm = constrain((int)server.arg("kwpm").toInt(), 5, 60);
  keyerFarn = server.hasArg("kfarn");
  if (server.hasArg("kcwpm")) keyerCharWpm = constrain((int)server.arg("kcwpm").toInt(), 5, 60);
  if (server.hasArg("aud_freq")) audioFreqHz = constrain((int)server.arg("aud_freq").toInt(), 300, 1500);
  if (server.hasArg("aud_vol")) audioVolumePct = constrain((int)server.arg("aud_vol").toInt(), 1, 100);
  if (server.hasArg("aud_ramp")) audioRampMs = constrain((int)server.arg("aud_ramp").toInt(), 1, 20);
  piezoEnabled = server.hasArg("pz_en");
  if (server.hasArg("pz_vol")) piezoVolumePct = constrain((int)server.arg("pz_vol").toInt(), 1, 100);
  if (server.hasArg("pz_pin")) { int p = (int)server.arg("pz_pin").toInt(); if (piezoPinAllowed(p)) piezoPin = p; }
  bool aud = server.hasArg("usbaud_en");
  if (!live && aud != usbAudioEnabled) { usbAudioEnabled = aud; needRestart(uiLang == 1 ? "USB audio" : "USB-Audio"); }
  serResync = true;

  // Bluetooth
  if (!live && server.hasArg("bt_mode")) {
    int m = constrain((int)server.arg("bt_mode").toInt(), 0, 2);
    if (m != btMode) { btMode = m; needRestart("Bluetooth"); }
  }
  if (server.hasArg("midi_dot")) midiNoteDot = constrain((int)server.arg("midi_dot").toInt(), 0, 127);
  if (server.hasArg("midi_dash")) midiNoteDash = constrain((int)server.arg("midi_dash").toInt(), 0, 127);
  if (server.hasArg("midi_vol")) midiVolume = constrain((int)server.arg("midi_vol").toInt(), 0, 127);
  if (server.hasArg("bt_left")) bleKeyLeftId = server.arg("bt_left");
  if (server.hasArg("bt_right")) bleKeyRightId = server.arg("bt_right");
  refreshKeyPointers();
  midiVolumeSent = false;   // Lautstaerke (CC7) mit dem naechsten Ton neu senden

  if (live) return;   // WLAN nur beim echten Speichern

  // WLAN (leere Auswahl aendert nichts)
  String manualSsid = server.hasArg("ssid_manual") ? server.arg("ssid_manual") : "";
  manualSsid.trim();   // iOS haengt gern ein Leerzeichen an
  String newSsid = ssidHome;
  if (manualSsid.length() > 0) newSsid = manualSsid;
  else if (server.hasArg("ssid_h") && server.arg("ssid_h").length() > 0) newSsid = server.arg("ssid_h");
  String newPass = server.hasArg("pass_h") ? server.arg("pass_h") : passHome;
  bool wlanChanged = (newSsid != ssidHome || newPass != passHome);
  // Netz ausdruecklich gewaehlt/eingegeben, aber (noch) nicht verbunden: auch ohne Aenderung neu starten = neuer Verbindungsversuch
  bool retry = !wlanChanged && newSsid.length() > 0 && WiFi.status() != WL_CONNECTED &&
               (manualSsid.length() > 0 || (server.hasArg("ssid_h") && server.arg("ssid_h").length() > 0));
  ssidHome = newSsid; passHome = newPass;
  if (wlanChanged || retry) needRestart(uiLang == 1 ? "WiFi" : "WLAN");
}

// Sofort-Uebernahme (vom Formular per fetch bei jeder Aenderung aufgerufen)
void handleApply() {
  lastWebActivityTime = millis();
  String why;
  applyWebArgs(true, why);
  server.send(200, "text/plain", "ok");
}

void handleSaveAll() {
  lastWebActivityTime = millis();
  String why = "";
  applyWebArgs(false, why);
  saveSettings();
  if (why.length() > 0) {
    String msg = String("Einstellungen gespeichert (") + why + "). Das Ger&auml;t startet neu ...";
    sendRebootPage(msg.c_str());
    delay(1000);
    WiFi.disconnect(true);   // sauber vom Router abmelden, sonst klappt die Verbindung nach dem Neustart manchmal nicht
    delay(300);
    ESP.restart();
  } else {
    sendAlert("Gespeichert");
  }
}

void goToDeepSleep() {
rgbLed.setPixelColor(0, 0);
rgbLed.show();
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
if (btMode != 2) BLEDevice::deinit(true);
gpio_hold_en((gpio_num_t)pin_paddle_links);
gpio_hold_en((gpio_num_t)pin_paddle_rechts);
gpio_deep_sleep_hold_en();
esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
esp_sleep_enable_ext1_wakeup((1ULL << pin_paddle_links) | (1ULL << pin_paddle_rechts), ESP_EXT1_WAKEUP_ANY_LOW);
esp_deep_sleep_start();
}
// ---- USB-Serial: gewuenschte Pegel berechnen und bei Aenderung an den PC melden ----
void updateSerialLines(bool dot, bool dash, bool key, bool ptt) {
  {
    // "Port geoeffnet" direkt aus TinyUSB lesen (DTR, Bit 0). Die Arduino-Ereignisse gehen teilweise verloren.
    bool open = (tud_cdc_n_get_line_state(0) & 1) != 0;
    if (open && !serPortOpen) serResync = true;
    serPortOpen = open;
  }
  if (!CdcNotify::available) return;
  bool want[4] = {false, false, false, false};  // Index = Belegung (0 wird ignoriert)
  if (usbSerialEnabled) {
    want[serMapDot] |= dot;
    want[serMapDash] |= dash;
    want[serMapKey] |= key;
    want[serMapPtt] |= ptt;
  }
  bool dcd = want[1], dsr = want[2], brk = want[3];
  bool changed = (dcd != serSentDcd || dsr != serSentDsr || brk != serSentBrk);
  bool resync = serResync;
  if (!changed && !(resync && usbSerialEnabled)) return;
  // Nach einem Fehlschlag nur alle 50 ms erneut versuchen (nicht bei jedem Keyer-Takt)
  static int64_t lastFailUs = -1000000;
  int64_t nowUs = esp_timer_get_time();
  if (nowUs - lastFailUs < 50000) return;
  serResync = false;
  if (CdcNotify::send(USBSerial, dcd, dsr, brk)) {
    serSentDcd = dcd; serSentDsr = dsr; serSentBrk = brk;
    serNotifyOk = serNotifyOk + 1;
  } else {
    serNotifyFail = serNotifyFail + 1;           // Diagnose: jeder Fehlschlag, auch bei geschlossenem Port
    lastFailUs = nowUs;
    if (serPortOpen) serResync = true;           // Port offen, aber Senden fehlgeschlagen (z.B. Endpoint belegt): naechste Runde erneut
  }
}
// Wird aufgerufen, wenn der PC DTR aendert (Port geoeffnet/geschlossen)
void onCdcLineState(void* arg, esp_event_base_t base, int32_t id, void* data) {
  arduino_usb_cdc_event_data_t* d = (arduino_usb_cdc_event_data_t*)data;
  serDtrRaw = d->line_state.dtr;
  serRtsRaw = d->line_state.rts;
  serLineEvents = serLineEvents + 1;
  // serPortOpen wird in updateSerialLines() direkt aus TinyUSB gelesen (diese Ereignisse sind nicht zuverlaessig)
}
#if MORSE_HAS_AUDIO
// ---- USB-Audio: Tonerzeugung (eigener Task, liefert alle 1 ms 48 Samples an das USB-Mikrofon) ----
static int16_t audioSineTable[1024];
void onAudioEvent(void* arg, esp_event_base_t base, int32_t id, void* data) {
  if (id == ARDUINO_USB_AUDIO_CARD_INTERFACE_ENABLE_EVENT) {
    arduino_usb_audio_card_event_data_t* d = (arduino_usb_audio_card_event_data_t*)data;
    if (d->interface_enable.interface == UAC_INTERFACE_MIC) audioStreaming = d->interface_enable.enable;
  }
}
void AudioToneTask(void* pvParameters) {
  const double rate = 48000.0;
  int16_t buf[100];
  uint32_t phase = 0;
  float amp = 0.0f;
  double acc = 0.0;
  int64_t last = esp_timer_get_time();
  for (;;) {
    vTaskDelay(1);
    int64_t now = esp_timer_get_time();
    acc += (double)(now - last) * rate / 1000000.0;  // Samples seit dem letzten Durchlauf (Takt exakt halten)
    last = now;
    int n = (int)acc;
    acc -= n;
    if (n > 96) n = 96;
    if (!audioStreaming || audioCard == NULL) { amp = 0.0f; continue; }
    float target = audioKeyed ? 1.0f : 0.0f;
    float step = 1.0f / ((float)audioRampMs * 48.0f);
    float gain = (float)audioVolumePct / 100.0f;
    uint32_t inc = (uint32_t)((double)audioFreqHz * 4294967296.0 / rate);
    for (int i = 0; i < n; i++) {
      if (amp < target) { amp += step; if (amp > target) amp = target; }
      else if (amp > target) { amp -= step; if (amp < target) amp = target; }
      buf[i] = (int16_t)((float)audioSineTable[phase >> 22] * amp * gain);
      phase += inc;
    }
    if (n > 0) { audioCard->write(buf, (uint16_t)(n * 2)); audioFramesSent = audioFramesSent + 1; }
  }
}
#endif

// ---- Keyer-Task: Iambic A/B nach dem Vorbild von PaddleBridge, Zeitraster 1 ms ----
// Liefert: Ton-/Tastensignal (um den Vorlauf verzoegert), PTT mit Nachlaufzeit, und meldet die
// Statusleitungen an den PC.
struct KeyEvent { int64_t t; bool v; };
void KeyerTask(void* pvParameters) {
  enum { K_IDLE, K_ELEMENT, K_GAP };
  enum { K_DIT = 0, K_DAH = 1 };
  int state = K_IDLE, cur = -1, last = -1, mode = 0;
  bool squeeze = false, ditMem = false, dahMem = false, pDot = false, pDash = false;
  bool dot = false, dash = false;
  int64_t ditT = 0, dahT = 0, tEnd = 0, lastAct = 0, u = 60000;
  int64_t lastEnd = -1000000000000LL, charGapUs = 0;   // Ende des letzten Elements; gedehnter Zeichenabstand (Farnsworth)
  bool keyRaw = false, keyRawPrev = false, keyDel = false, ptt = false;
  KeyEvent q[128];
  int qh = 0, qt = 0;
  bool pttPrev = false;
  bool pzAttached = false;
  int pzPinCur = -1, pzFreqCur = 0, pzDutyCur = -1;
  float pzAmp = 0.0f;
  int pendingCmd = 0;   // 1 = TX-Befehl, 2 = RX-Befehl senden
  int64_t lastCmdWriteUs = 0;   // Zeitpunkt des letzten Befehls im USB-Sendepuffer

  auto decide = [&](bool fromIdle) -> int {
    // Iambic A: Squeeze im letzten Element und jetzt beide Hebel los -> Ende
    if (!fromIdle && mode == 1 && squeeze && !dot && !dash) { ditMem = false; dahMem = false; return -1; }
    bool d = dot || ditMem;
    bool h = dash || dahMem;
    if (d && h) {
      if (fromIdle) return (ditT <= dahT) ? K_DIT : K_DAH;
      return (last == K_DIT) ? K_DAH : K_DIT;
    }
    if (d) return K_DIT;
    if (h) return K_DAH;
    return -1;
  };
  auto begin = [&](int el, int64_t t0) {
    cur = last = el;
    if (el == K_DIT) ditMem = false; else dahMem = false;
    squeeze = dot && dash;
    state = K_ELEMENT;
    tEnd = t0 + ((el == K_DIT) ? u : 3 * u);
  };
  auto advance = [&]() {
    if (state == K_ELEMENT) {
      state = K_GAP;
      lastEnd = tEnd;
      tEnd += u;
    } else {
      int el = decide(false);
      if (el >= 0) begin(el, tEnd);
      else { state = K_IDLE; cur = -1; squeeze = false; }
    }
  };

  for (;;) {
    vTaskDelay(1);
    keyerTicks = keyerTicks + 1;
    int64_t now = esp_timer_get_time();
    dot = paddleDot;
    dash = paddleDash;
    mode = keyerMode;
    {
      int sW = constrain(keyerWpm, 5, 60);
      int cW = sW;
      charGapUs = 0;
      if (keyerFarn) {
        int cc = constrain(keyerCharWpm, 5, 60);
        if (cc > sW) {
          cW = cc;
          // Standard-Farnsworth: Zusatzzeit pro PARIS-Wort ta = (60*c - 37,2*s) / (c*s); Zeichenabstand = 3 * ta / 19
          double ta = (60.0 * cc - 37.2 * sW) / ((double)cc * sW);
          charGapUs = (int64_t)(3.0 * ta / 19.0 * 1e6);
        }
      }
      u = 1200000LL / cW;   // 1 Einheit in Mikrosekunden (1,2 s / WpM), bei Farnsworth mit dem Zeichentempo
    }
    if (dot && !pDot) { ditMem = true; ditT = now; }
    if (dash && !pDash) { dahMem = true; dahT = now; }
    pDot = dot; pDash = dash;

    if (mode == 0) {                              // Paddle direkt (Handtaste)
      state = K_IDLE; cur = -1; squeeze = false; ditMem = false; dahMem = false;
      keyRaw = dot || dash;
    } else {
      if (dot && dash) squeeze = true;
      if (cur == K_DIT && dash) dahMem = true;    // Gegenhebel merken (Iambic B)
      else if (cur == K_DAH && dot) ditMem = true;
      for (;;) {
        if (state == K_IDLE) {
          int el = decide(true);
          if (el < 0) break;
          if (charGapUs > 0) {   // Farnsworth: nach einer Pause >= 2 Einheiten (= neues Zeichen) erst nach dem gedehnten Zeichenabstand starten
            int64_t since = now - lastEnd;
            if (since >= 2 * u && since < charGapUs) break;
          }
          begin(el, now);
        }
        if (now < tEnd) break;
        advance();
      }
      keyRaw = (state == K_ELEMENT);
    }
    bool busy = (mode != 0) && (state != K_IDLE || ditMem || dahMem);
    keyerBusy = busy;

    // Piezo-Mithoerton: folgt dem Keyer sofort (ohne Vorlauf), mit weichen Flanken
    if (piezoEnabled && piezoPinAllowed(piezoPin)) {
      int fHz = constrain(audioFreqHz, 300, 1500);
      if (!pzAttached || pzPinCur != piezoPin) {
        if (pzAttached) { ledcWrite(pzPinCur, 0); ledcDetach(pzPinCur); }
        pzAttached = ledcAttach(piezoPin, fHz, 10);
        pzPinCur = piezoPin; pzFreqCur = fHz; pzDutyCur = -1;
      } else if (fHz != pzFreqCur) {
        ledcChangeFrequency(piezoPin, fHz, 10);
        pzFreqCur = fHz;
      }
      float step = 1.0f / (float)max(1, audioRampMs);   // eine Aenderung pro Millisekunde
      if (keyRaw) { pzAmp += step; if (pzAmp > 1.0f) pzAmp = 1.0f; }
      else { pzAmp -= step; if (pzAmp < 0.0f) pzAmp = 0.0f; }
      // Lautstaerke 1..100 % -> Amplitude der Grundwelle in dB-Skala (100 % = 0 dB, 50 % = -20 dB, 1 % = -40 dB).
      // Die Grundwelle eines Rechtecks mit Tastverhaeltnis D ist ~ sin(pi*D); daher D = asin(Amplitude)/pi.
      // (Ein lineares Tastverhaeltnis aendert die Lautstaerke kaum: 25 % statt 50 % sind nur -3 dB.)
      float pzGain = powf(10.0f, ((float)piezoVolumePct - 100.0f) / 50.0f);
      float pzA = pzAmp * pzGain; if (pzA > 1.0f) pzA = 1.0f;
      int duty = (int)(asinf(pzA) / 3.14159265f * 1024.0f + 0.5f);   // max. 512 = 50 % Tastverhaeltnis
      if (duty > 512) duty = 512;
      if (pzAttached && duty != pzDutyCur) { ledcWrite(piezoPin, duty); pzDutyCur = duty; }
    } else if (pzAttached) {
      ledcWrite(pzPinCur, 0);
      ledcDetach(pzPinCur);
      pinMode(pzPinCur, OUTPUT);
      digitalWrite(pzPinCur, LOW);
      pzAttached = false; pzAmp = 0.0f; pzDutyCur = -1;
    }

    // Vorlauf nur, wenn irgendeine PTT-Methode (Pin oder serielle Befehle) aktiv ist
    bool pttCfg = usbSerialEnabled && ((serMapPtt != 0 && CdcNotify::available) || serCmdActive);
    int lead = pttCfg ? constrain(keyLeadMs, 0, 1000) : 0;
    if (busy || dot || dash) { lastAct = now; ptt = true; }
    else if (ptt && (now - lastAct) > ((int64_t)serPttHangMs + lead) * 1000LL) ptt = false;

    // Tastensignal um den Vorlauf verzoegern
    if (keyRaw != keyRawPrev) {
      keyRawPrev = keyRaw;
      int nh = (qh + 1) % 128;
      if (nh != qt) { q[qh].t = now + (int64_t)lead * 1000LL; q[qh].v = keyRaw; qh = nh; }
    }
    while (qt != qh && q[qt].t <= now) { keyDel = q[qt].v; qt = (qt + 1) % 128; }
    audioKeyed = keyDel;

    updateSerialLines(dot, dash, keyDel, ptt);

    // PTT ueber serielle Befehle: bei jedem Wechsel TX- bzw. RX-Text senden (ohne den Task zu blockieren)
    if (ptt != pttPrev) { pttPrev = ptt; if (usbSerialEnabled && serCmdActive) pendingCmd = ptt ? 1 : 2; }
    if (pendingCmd != 0) {
      String c = (pendingCmd == 1) ? serTxCmd : serRxCmd;
      c.replace("\\r", "\r");
      c.replace("\\n", "\n");
      if (!usbSerialEnabled || c.length() == 0) pendingCmd = 0;
      else if (!serIgnoreDtr && !serPortOpen) { pendingCmd = 0; serCmdDropped = serCmdDropped + 1; }   // Port am PC ohne DTR
      else {
        // Direkt in den TinyUSB-FIFO schreiben - auch ohne DTR (manche Programme, z.B. die
        // SDR Console, oeffnen den CAT-Port ohne DTR; USBSerial.write() wuerde dann verwerfen).
        // Belegte Bytes = Puffergroesse - freier Platz. Liegen Bytes laenger als 200 ms unabgeholt
        // im Puffer (oder passt der Befehl nicht mehr hinein), liest niemand mit -> alte Befehle verwerfen.
        int64_t nowC = esp_timer_get_time();
        size_t freeB = tud_cdc_n_write_available(0);
        int used = (int)SER_TX_FIFO_SIZE - (int)freeB;
        if (used > 0 && (nowC - lastCmdWriteUs > 200000LL || freeB < c.length())) {
          tud_cdc_n_write_clear(0);
          freeB = tud_cdc_n_write_available(0);
        }
        if (freeB >= c.length()) {
          tud_cdc_n_write(0, c.c_str(), c.length());
          tud_cdc_n_write_flush(0);
          lastCmdWriteUs = nowC;
          serCmdSent = serCmdSent + 1;
        } else {
          serCmdDropped = serCmdDropped + 1;
        }
        pendingCmd = 0;
      }
    }
    {
      int u = (int)SER_TX_FIFO_SIZE - (int)tud_cdc_n_write_available(0);
      serTxPending = u > 0 ? u : 0;
    }
    while (USBSerial.available()) USBSerial.read();   // Antworten des Programms verwerfen
  }
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
  if (usbMidiEnabled && usbMidi != NULL) {
    if (links_neu != links_alt) { if (links_neu) usbMidi->noteOn(usbMidiNoteDot, 127, 1); else usbMidi->noteOff(usbMidiNoteDot, 0, 1); }
    if (rechts_neu != rechts_alt) { if (rechts_neu) usbMidi->noteOn(usbMidiNoteDash, 127, 1); else usbMidi->noteOff(usbMidiNoteDash, 0, 1); }
  }
  // Bluetooth: entweder BLE-Tastatur (eigene Belegung) oder BLE-MIDI (eigene Noten)
  if (btMode == 1) {
    sendBleKeyReport(links_neu, rechts_neu);
  } else if (btMode == 0) {
    if (links_neu != links_alt) sendMidiNote(midiNoteDot, links_neu);
    if (rechts_neu != rechts_alt) sendMidiNote(midiNoteDash, rechts_neu);
  }
  if (!links_neu && !rechts_neu) { rgbLed.setPixelColor(0, 0); rgbLed.show(); }
  links_alt = links_neu;
  rechts_alt = rechts_neu;
}
paddleDot = links_neu;
paddleDash = rechts_neu;
if (!links_neu && !rechts_neu && (millis() - lastHeartbeatTime > 1000)) {
rgbLed.setPixelColor(0, colorHeartbeat); rgbLed.show(); delay(30);
rgbLed.setPixelColor(0, 0); rgbLed.show();
lastHeartbeatTime = millis();
}
if (sleepTimeoutMs > 0 && (millis() - lastActivityTime) > sleepTimeoutMs) {
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
  DBG("Bluetooth-Adresse (%s): %s\n", btMode == 1 ? "BLE-Tastatur" : "BLE-MIDI", btAddressText.c_str());
}

void setup() {
Serial.begin(115200);
USBSerial.onEvent(ARDUINO_USB_CDC_LINE_STATE_EVENT, onCdcLineState);
loadSettings();
buildKeyOptions();
refreshKeyPointers();
rgbLed.begin();
rgbLed.setBrightness(40);
rgbLed.show();
if (usbKeyboard != NULL) usbKeyboard->begin();
if (usbMidi != NULL) usbMidi->begin();
#if MORSE_HAS_AUDIO
if (audioCard != NULL) {
  for (int i = 0; i < 1024; i++) audioSineTable[i] = (int16_t)(32000.0f * sinf(2.0f * 3.14159265f * i / 1024.0f));
  audioCard->onEvent(onAudioEvent);
  audioCard->begin();
  xTaskCreatePinnedToCore(AudioToneTask, "AudioTask", 4096, NULL, 4, NULL, 1);
}
#endif
#if MORSE_HAS_AUDIO
DBG("USB aktiv: Tastatur=%d MIDI=%d Audio=%d (Einstellung: %d/%d/%d)\n", usbKeyboard != NULL, usbMidi != NULL, audioCard != NULL, usbKeyboardEnabled, usbMidiEnabled, usbAudioEnabled);
#else
DBG("USB aktiv: Tastatur=%d MIDI=%d Audio=0 (Einstellung: %d/%d/%d)\n", usbKeyboard != NULL, usbMidi != NULL, usbKeyboardEnabled, usbMidiEnabled, usbAudioEnabled);
#endif
USB.begin();
gpio_hold_dis((gpio_num_t)pin_paddle_links);
gpio_hold_dis((gpio_num_t)pin_paddle_rechts);
pinMode(pin_paddle_links, INPUT_PULLUP);
pinMode(pin_paddle_rechts, INPUT_PULLUP);
// Notausgang: beide Paddles beim Einschalten gedrueckt halten -> Heimnetz ueberspringen, Hotspot starten
bool forceAp = (digitalRead(pin_paddle_links) == LOW && digitalRead(pin_paddle_rechts) == LOW);

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
} else if (btMode == 0) {
  // ---- Bestehend: BLE-MIDI, jetzt mit Pairing/Bonding (siehe BLE-MIDI Spezifikation) ----
  useOwnBluetoothAddressForMidi();   // eigene Bluetooth-Adresse nur in diesem Modus
  BLEDevice::init("Morse-Paddle-MIDI");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Die offizielle BLE-MIDI-Spezifikation verlangt Pairing/Verschluesselung fuer die
  // MIDI I/O Characteristic. "Secure Connections" + vollstaendiger (symmetrischer)
  // Schluesselaustausch, wie von Apples eigenen BLE-Zubehoer-Vorgaben fuer BLE-MIDI
  // empfohlen - robuster als die einfache Legacy-Kopplung aus dem BLE-Tastatur-Modus.
  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  pSecurity->setKeySize(16);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  BLEService* pService = pServer->createService(BLEUUID(MIDI_SERVICE_UUID));
  pMidiCharacteristic = pService->createCharacteristic(BLEUUID(MIDI_CHARACTERISTIC_UUID), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
  // Erzwingt, dass der Zugriff auf die Characteristic verschluesselt (= gepaart) sein muss -
  // erst dadurch stoesst das Betriebssystem des Gegenueruts das Pairing/Bonding tatsaechlich an.
  pMidiCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinInterval(0x0020); pAdvertising->setMaxInterval(0x0030);
  BLEDevice::startAdvertising();
}

if (btMode != 2) readBluetoothAddress(); else btAddressText = "-";

resetReasonCode = (int)esp_reset_reason();
DBG("Neustart-Grund: %s, freier Speicher %u Byte\n", resetReasonText().c_str(), (unsigned)ESP.getFreeHeap());
WiFi.onEvent(onWifiStaEvent);
WiFi.setHostname("morse-s3");   // Name in der Geraeteliste des Routers
WiFi.mode(WIFI_STA);
scanNetworks();
if (forceAp) DBG("Beide Paddles gedrueckt: Heimnetz wird uebersprungen, Hotspot startet\n");
if (ssidHome.length() > 0 && !forceAp) {
// Bis zu 3 Versuche (je ca. 10 s); vor jedem neuen Versuch wird das Funkmodul komplett neu gestartet.
// Nach einem Software-Neustart kommt der erste Versuch manchmal nicht durch (Router haelt noch die alte Verbindung).
for (int attempt = 0; attempt < 3 && WiFi.status() != WL_CONNECTED; attempt++) {
  if (attempt > 0) {
    DBG("WLAN-Versuch %d nicht erfolgreich (Status %d, Grund %d) - Funkmodul wird neu gestartet\n", attempt, (int)WiFi.status(), (int)lastWifiReason);
    WiFi.disconnect(true);
    delay(300);
    WiFi.setHostname("morse-s3");
    WiFi.mode(WIFI_STA);
    delay(100);
  }
  WiFi.begin(ssidHome.c_str(), passHome.c_str());
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }
}
DBG("WLAN '%s': %s, Grund %d, Status %d, Scan: %s\n", ssidHome.c_str(), WiFi.status() == WL_CONNECTED ? "verbunden" : "NICHT verbunden", (int)lastWifiReason, (int)WiFi.status(), wifiScanInfo.c_str());
}
if (WiFi.status() != WL_CONNECTED) {   // Hotspot nur, wenn keine Verbindung zum Heimnetz zustande kam
WiFi.mode(WIFI_AP);
WiFi.softAP("Morse-S3-Config", "12345678");
}
if (WiFi.status() == WL_CONNECTED) WiFi.setSleep(false);   // kein WLAN-Modem-Sleep: stabilere/schnellere Seitenauslieferung im Heimnetz
if (WiFi.status() == WL_CONNECTED) DBG("Heimnetz-IP: %s  (http://morse-s3.local)\n", WiFi.localIP().toString().c_str());
if (MDNS.begin("morse-s3")) MDNS.addService("http", "tcp", 80);
server.on("/", handleRoot);
server.on("/rescan", handleRescan);
server.on("/ping", []() { server.send(200, "text/plain", "ok"); });   // kleine Antwort fuer die Neustart-Seite (statt die ganze Konfigseite abzufragen)
server.on("/save_all", handleSaveAll);
server.on("/apply", HTTP_POST, handleApply);
server.on("/save_lang", handleSaveLang);
server.begin();
lastActivityTime = millis();
lastHeartbeatTime = millis();
lastWebActivityTime = millis();
xTaskCreatePinnedToCore(MorseCodeLoop, "MorseTask", 4096, NULL, 3, &MorseTask, 1);
xTaskCreatePinnedToCore(KeyerTask, "KeyerTask", 8192, NULL, 4, NULL, 1);
}
void loop() {
if (wifiActive) {
server.handleClient();
if (webTimeoutMs > 0 && (millis() - lastWebActivityTime) > webTimeoutMs && (millis() - lastActivityTime) > webTimeoutMs) {
WiFi.disconnect(true);
WiFi.mode(WIFI_OFF);
wifiActive = false;
}
}
delay(20);
}
