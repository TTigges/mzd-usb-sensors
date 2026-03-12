# MZD USB Sensors — Modernisierung & Optimierung

## 🎯 Projektziele
- **Flash-Wear eliminieren:** Weg von ständigen Datei-Schreibvorgängen auf CMU
- **Latenz reduzieren:** Von Polling auf kontinuierlichen 4Hz-Stream umstellen
- **Format modernisieren:** Positionsbasierte `#`-Delimiter durch JSON ersetzen
- **Architektur vereinfachen:** Weniger bewegliche Teile, bessere Wartbarkeit

---

## ✅ Abgeschlossen

### 🧹 Code-Cleanup (usbunit Arduino Firmware)
- **4 ungenutzte Module entfernt:** Display, TPMS_BLE, RGB_Analog, WS2801
  - 8 Dateien gelöscht (.h/.ino Paare)
  - `config.h` auf 3 aktive Module reduziert: CC1101, TPMS_433, Oil
  - `MAX_ACTIONS` von 6 auf 2 verringert
- **Resultate:** ~2KB Flash gespart, weniger Verwirrung im Code

### ⚡ Phase 1: Protokoll-Optimierung (Sender)
- **Problem:** `Serial.flush()` blockierte Arduino-Loop, viele kleine `Serial.print()` erzeugten Jitter
- **Lösung:** 
  - Neue `sendProtocolLine()` Funktion mit `Serial.write()` statt `Serial.print()`
  - `sendEOT()` ohne `flush()` — Serial TX-Buffer garantiert richtige Reihenfolge
  - NULL-Pointer-Bug in `sendCommand()`/`sendError()` behoben
- **Resultate:** Weniger Blocking, stabileres Timing

### ⚡ Phase 1b: Frame-Buffering
- **TPMS sendData():** 4 Sensoren mit je ~10 `Serial.print()` → 1 `snprintf()` + 1 `Serial.write()`
- **Oil sendData():** 4 separate prints → 1 buffered line
- **Frame-Größen:** TPMS ~160 bytes, Oil ~48 bytes (beide unter 256-byte Empfangspuffer)
- **Resultate:** Weniger USB-Interrupts, gleichmäßigerer Datenfluss

### ✓ Kompatibilitätsprüfung
- Bestätigt: **usbget Receiver unverändert kompatibel** mit optimiertem Sender
- Gleiches Protokollformat, Zeilenlängen im Spezifikationsbereich
- `receiveLine()` buffert korrekt, kein Datenverlag

---

## 🚀 Neues Ziel: SSE-basierte Architektur

### 🏗️ Vision: HTTP + SSE statt websocketd + Files

#### Alt (aktuell)
```
Arduino →USB→ usbget →.out files→ Bash →websocketd →WS→ Browser
Browser →WS→ websocketd →stdin→ Bash →usbget →USB→ Arduino (Config)
```

#### Neu (geplant)
```
Arduino →USB→ usbget-daemon →SSE(JSON)→ Browser (Daten 4Hz)
Browser →HTTP POST→ usbget-daemon →USB→ Arduino (Config 2x/Jahr)
```

### 📊 Datenformat: JSON statt positionsbasierte Delimiter

#### Problem mit `#`-Format
```javascript
res = event.data.split("#");  // envData##0#0#18.5######0#0#25.1#2.05#...
updateOutsideTemp(res[4]);    // Index-Magie, fragil, nicht erweiterbar
```

#### Lösung: JSON
```javascript
var d = JSON.parse(event.data);  // {"tpms":{"fl":{"t":25.1,"p":2.05},...}}
updateTireTemp("Fl", d.tpms.fl.t);  // benannt, selbstdokumentierend, typsicher
```

**Vorteile:**
- ✅ Rückwärtskompatibel: neue Felder brechen bestehende Apps nicht
- ✅ Multi-Consumer: jede App nimmt nur was sie braucht
- ✅ Debuggbar: Menschen können JSON lesen
- ✅ Native Browser-Unterstützung: `JSON.parse()` ist optimiert

### 🔧 Architektur-Details

#### Server-Side Events (SSE) für Streaming
- **Einfacher als WebSocket:** Standard HTTP, automatisches Reconnect
- **Debuggbar:** `curl http://127.0.0.1:9969/stream` zeigt Live-Daten
- **Optimal für Read-mostly:** 99% der Zeit nur Daten senden

```c
// usbget-daemon SSE-Output
printf("HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\n\r\n");
printf("data: {\"tpms\":{\"fl\":{\"t\":%.1f,\"p\":%.2f},...}}\n\n", ...);
```

#### HTTP POST für seltene Konfiguration
- **TPMS-ID Updates:** 2-3x/Jahr bei Ventiltausch
- **RESTful:** `POST /config` mit JSON-Body
- **Einfach:** Standard `fetch()` API

```javascript
// Config nur bei Bedarf
fetch("/usbget/config", {
    method: "POST",
    body: JSON.stringify({tpms: {fl: "A1B2C3D4", fr: "E5F6G7H8", ...}})
});
```

### 📈 Erwartete Verbesserungen

| Aspekt | Vorher | Nachher | Gewinn |
|---|---|---|---|
| **Flash-Writes** | Jede Query → `.out` | Keine | Problem eliminiert |
| **Latenz** | Polling ~1s | Streaming 250ms | 4x schneller |
| **Format** | Index-basiert | JSON | Wartbar, erweiterbar |
| **Debugging** | `.out` Dateien | HTTP/SSE | Standard-Tools |
| **Architecture** | 4 Komponenten | 2 Komponenten | Weniger Fehlerquellen |

---

## ❓ Ursprüngliche Problemstellung

### Flash-Wear Problem
- **CMU schreibt bei jedem Query .out-Dateien** nach `/tmp/mnt/data_persist/dev/bin`
- Bei 4Hz continuous streaming → 14.400 Writes/Stunde auf eMMC/NAND
- **Flash-Speicher verschleißt:** Limited write cycles, besonders bei ständigen kleinen Files
- Lösung: Eliminiere File-I/O komplett durch Streaming

### Latenz & Polling-Ineffizienz
- **Aktuell:** Browser → WS → Bash → usbget → USB → Arduino → USB → usbget → .out → Bash → WS → Browser
- **Roundtrip ~1 Sekunde** für jeden Wert
- Arduino könnte kontinuierlich senden, aber System pollt nur on-demand
- **Verschwendung:** Arduino sammelt ständig Daten, wird aber nur gelegentlich abgefragt

### Fragiles Datenformat
```javascript
// Aktuelles Format: Index-Horror
var res = event.data.split("#");  // "envData##0#0#18.5######0#0#25.1#2.05#24.8#2.10#26.0#2.00#25.5#2.08"
updateTireTemp("Fl", res[12]);    // Warum Index 12? Niemand weiß es ohne Doku
```
- **20 Felder, nur 9 genutzt im gezeigten Code:** Andere Apps nutzen andere Indizes
- **Erweitern unmöglich:** Neuer Sensor verschiebt alle Indizes → alle Apps brechen
- **Multi-Consumer-Alptraum:** Apps müssen Index-Katalog koordinieren

---

## 📋 Implementierungs-Roadmap

### Phase 2a: Arduino Broadcast Mode 🔧
**Ziel:** Arduino sendet kontinuierlich statt on-demand

#### Neue Protokoll-Kommandos
```c
// protocol.h: Erweitere Command-Enum
#define CMD_BROADCAST_START 'B'
#define CMD_BROADCAST_STOP  'X'
```

#### Multi-Rate-System für optimale Performance
**Problem:** Verschiedene Sensoren brauchen verschiedene Update-Raten
- TPMS-Druck ändert sich über Stunden → 1Hz ausreichend
- Öldruck bei Sportfahrten → 4Hz für Responsivität  
- Ladedruck folgt Gaspedal → 8Hz für Echtzeit-Feeling

#### Individual Sensor Timer Implementation
```c
// usbunit.ino: Per-Sensor Timing statt global
typedef struct {
    unsigned long lastUpdate;
    unsigned int intervalMs;
    bool forceNext;  // Für sofortigen Update nach Config
} sensor_timing_t;

// Default-Intervalle für verschiedene Sensor-Kategorien
sensor_timing_t tpms_timing = {0, 1000, false};    // 1Hz - träge
sensor_timing_t oil_timing = {0, 250, false};      // 4Hz - responsive
sensor_timing_t boost_timing = {0, 125, false};    // 8Hz - Echtzeit (geplant)

bool broadcastEnabled = false;
unsigned long lastBroadcastCheck = 0;
#define BROADCAST_CHECK_INTERVAL_MS 50  // Prüfe alle 50ms ob was zu senden ist

bool shouldUpdate(sensor_timing_t* timing, unsigned long now) {
    if (timing->forceNext) {
        timing->forceNext = false;
        timing->lastUpdate = now;
        return true;
    }
    
    if (now - timing->lastUpdate >= timing->intervalMs) {
        timing->lastUpdate = now;
        return true;
    }
    return false;
}

void loop() {
    if (Serial.available() > 0) {
        handleCommand();  // Commands haben Priorität
    }
    
    runTimeout();  // Bestehende Sensor-Updates
    
    // Broadcast-Check: Häufiger prüfen, aber nur senden wenn nötig
    if (broadcastEnabled && (millis() - lastBroadcastCheck >= BROADCAST_CHECK_INTERVAL_MS)) {
        sendBroadcastData();
        lastBroadcastCheck = millis();
    }
}

void sendBroadcastData() {
    unsigned long now = millis();
    bool sentSomething = false;
    
    // Jeder Sensor prüft seinen eigenen Timer
    if (shouldUpdate(&tpms_timing, now)) {
        actions[TPMS_INDEX]->sendData();
        sentSomething = true;
    }
    
    if (shouldUpdate(&oil_timing, now)) {
        actions[OIL_INDEX]->sendData();  
        sentSomething = true;
    }
    
    // Zukünftiger Ladedruck-Sensor
    // if (shouldUpdate(&boost_timing, now)) {
    //     actions[BOOST_INDEX]->sendData();
    //     sentSomething = true;
    // }
    
    // EOT nur senden wenn tatsächlich Daten kamen
    if (sentSomething) {
        sendEOT();
    }
}
```

#### Konfigurierbare Update-Raten
```c
// config.h: Timing-Parameter
#define TPMS_DEFAULT_INTERVAL_MS    1000   // 1Hz - Reifendruck träge
#define OIL_DEFAULT_INTERVAL_MS     250    // 4Hz - Balance Responsivität/Effizienz
#define BOOST_DEFAULT_INTERVAL_MS   125    // 8Hz - Throttle-Response sichtbar
#define ENV_DEFAULT_INTERVAL_MS     2000   // 0.5Hz - Umgebung sehr träge

typedef struct timing_config_t {
    uint16_t tpms_interval_ms;    
    uint16_t oil_interval_ms;
    uint16_t boost_interval_ms;
    checksum_t checksum;
} timing_config_t;

// Via Stiming Command änderbar für Race/Eco-Modi
```

#### Bandbreiten-Effizienz
```
19200 baud = ~1920 bytes/sec praktisch

Sensor      Rate    Bytes/Frame    Bytes/sec    % Budget
TPMS        1Hz     120            120          6.25%
Oil         4Hz     35             140          7.3% 
Boost       8Hz     25             200          10.4%
TOTAL                              460          24%

→ 75% Reserve für Config, USB-Overhead, Erweiterungen
```

#### Command-Handler erweitern
```c
// usbunit.ino: In handleQCommand()
case CMD_BROADCAST_START:
    broadcastEnabled = true;
    lastBroadcast = 0;  // Sofort senden
    sendEOT();
    break;
    
case CMD_BROADCAST_STOP:
    broadcastEnabled = false;
    sendEOT();
    break;
```

#### Wichtige Details
- **Config während Broadcast:** Commands unterbrechen Broadcast-Zyklus
- **Memory:** Keine neuen Arrays, nutzt bestehende sensor[]-Strukturen
- **Timing:** `millis()` overflow nach 49 Tagen ist unkritisch (Automotive-Zyklen)

### Phase 2b: usbget HTTP-Daemon 🌐
**Ziel:** Eigener HTTP-Server statt CLI-Tool + websocketd

#### Dependency entscheiden
```bash
# Option A: microhttpd (leichtgewichtig, C)
sudo apt-get install libmicrohttpd-dev

# Option B: Raw sockets (mehr Code, keine Dependencies)
# Option C: Ein anderes leightweight HTTP-Framework
```

#### HTTP-Server Grundstruktur
```c
// usbget.c: Neue main() für Daemon-Mode
int main(int argc, char *argv[]) {
    if (has_flag(argc, argv, "--daemon")) {
        return run_http_daemon();
    }
    // Bestehende CLI-Mode bleibt für backward-compatibility
    return run_cli_mode();
}

int run_http_daemon() {
    // USB-Verbindung öffnen und halten
    if (usbConnect(device) != 0) {
        fprintf(stderr, "Failed to connect to USB device\n");
        return 1;
    }
    
    // Arduino Broadcast starten
    usbSendCommand("B\n");
    
    // HTTP-Server starten
    start_http_server(9969);
    
    // Main loop: USB → JSON buffern, HTTP clients bedienen
    while (running) {
        read_arduino_data();
        handle_http_requests();
    }
}
```

#### SSE-Route Implementation
```c
// /stream endpoint
void handle_sse_stream(connection) {
    send_sse_headers(connection);
    
    // Client in Live-Stream einhängen
    add_sse_client(connection);
    
    // Die Daten kommen vom Arduino-Reader-Thread
    // Werden als JSON zu allen SSE-Clients gepusht
}

void arduino_data_received(char* protocol_line) {
    // Arduino Protocol → JSON konvertieren
    if (strncmp(line, "+", 1) == 0) {
        // +TPMS oder +OIL Daten → JSON-Buffer hinzufügen
        parse_arduino_line_to_json(line);
    } else if (strncmp(line, ".", 1) == 0) {
        // EOT → JSON komplett, an alle SSE-Clients senden
        broadcast_json_to_sse_clients();
        reset_json_buffer();
    }
}
```

#### Config-Route Implementation
```c
// /config endpoint - HTTP POST
void handle_config_post(connection, json_body) {
    // JSON parsen (eventuell mit cJSON library)
    cJSON *config = cJSON_Parse(json_body);
    
    if (cJSON_HasObjectItem(config, "tpms")) {
        // TPMS Config an Arduino weiterleiten
        send_tpms_config_to_arduino(config["tpms"]);
    }
    
    // Response
    send_http_response(connection, "200 OK", "application/json", 
                      "{\"status\":\"success\"}");
}
```

#### JSON-Assembly mit Multi-Rate-Support
```c
// Arduino sendet unterschiedliche Sensor-Daten je nach Timing
// Manchmal nur: "+oiltemp: 85.2 oilpress: 3.45"
// Manchmal nur: "+0: A1B2C3D4 25.1 2.05"  
// usbget assembliert sparsames JSON

typedef struct {
    char json_buffer[2048];
    int buffer_pos;
    bool tpms_in_frame;
    bool oil_in_frame;
    bool boost_in_frame;
} json_builder_t;

void parse_arduino_line_to_json(char* line) {
    if (starts_with(line, "+0:") || starts_with(line, "+1:") || 
        starts_with(line, "+2:") || starts_with(line, "+3:")) {
        // TPMS Sensor-Line → JSON
        if (!json_builder.tpms_in_frame) {
            append_to_json("\"tpms\":{");
            json_builder.tpms_in_frame = true;
        }
        parse_tpms_line(line);
    } else if (contains(line, "oiltemp:")) {
        // Oil Sensor-Line → JSON
        if (!json_builder.oil_in_frame) {
            append_to_json("\"oil\":{");
            json_builder.oil_in_frame = true;
        }
        parse_oil_line(line);
    }
}

// Resultierende JSON-Nachrichten sind variabel:
// Nur Oil:  {"oil":{"t":85.2,"p":3.45}}  (~30 bytes)
// Nur TPMS: {"tpms":{"fl":{"t":25.1,"p":2.05},...}}  (~120 bytes)  
// Beides:   {"tpms":{...},"oil":{...}}  (~150 bytes)
```

#### Browser-Side Smart Merging
```javascript
var sensorState = {};  // Persistent state zwischen Updates

eventSource.onmessage = function(event) {
    var update = JSON.parse(event.data);
    
    // Merge nur geänderte Werte in globalen State
    Object.assign(sensorState, update);
    
    // Selective Rendering - nur was sich geändert hat
    if (update.tpms) renderTires(sensorState.tpms);      // 1Hz Updates
    if (update.oil)  renderOil(sensorState.oil);         // 4Hz Updates  
    if (update.boost) renderBoost(sensorState.boost);    // 8Hz Updates
};

// Racing-Mode: Temporär höhere Update-Raten
function setRaceMode() {
    fetch("/config", {
        method: "POST",
        body: JSON.stringify({
            timing: {
                oil: 100,    // 10Hz für Performance-Monitoring
                boost: 62    // 16Hz für Throttle-Response
            }
        })
    });
}
```

### Phase 2c: Integration & Migration 🔄

#### Startup-Script Änderungen
```bash
# Vorher
websocketd --port=9969 /path/to/bash-script.sh

# Nachher  
/path/to/usbget --daemon --device=arduino_nano_clone --port=9969

# Oder mit explizitem Port-Parameter falls implementiert
/path/to/usbget --daemon --device=arduino_nano_clone --http-port=9969
```

#### Browser-App Migration
```javascript
// Vorher: WebSocket
function retrievedata(action) {
    var tpmsWebsocket = new WebSocket("ws://127.0.0.1:9969/");
    tpmsWebsocket.onmessage = function(event) {
        var res = event.data.split("#");
        // ...Index-Gymnastik...
    };
    tpmsWebsocket.send(action);
}

// Nachher: SSE + fetch
function connectSensorStream() {
    var eventSource = new EventSource("http://127.0.0.1:9969/stream");
    eventSource.onmessage = function(event) {
        var data = JSON.parse(event.data);
        if (data.tpms) updateTires(data.tpms);
        if (data.oil) updateOil(data.oil);
    };
    // Automatisches Reconnect eingebaut!
}

function setTpmsConfig(sensors) {
    fetch("http://127.0.0.1:9969/config", {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify({tpms: sensors})
    }).then(response => response.json())
      .then(data => console.log("Config updated:", data));
}
```

#### Fallback-Strategie
- **Parallel-Betrieb:** Alte websocketd + neue SSE beide laufen lassen
- **Feature-Flag in Browser:** `if (USE_NEW_SSE) { connectSSE() } else { connectWebSocket() }`
- **Graduelle Migration:** Ein Device/Feature nach dem anderen umstellen
- **Rollback-Plan:** Bei Problemen sofort auf altes System zurück

---

## 🧪 Test-Strategien

### Integration Testing
```bash
# Arduino Broadcast testen
echo "B" | ./usbget --device=arduino_nano_clone
# Sollte kontinuierliche Daten senden bis 'X'

# SSE-Stream testen  
curl -N http://127.0.0.1:9969/stream
# Sollte JSON-Events im SSE-Format ausgeben

# Config testen
curl -X POST http://127.0.0.1:9969/config \
     -H "Content-Type: application/json" \
     -d '{"tpms":{"fl":"A1B2C3D4"}}'
```

### Load Testing  
```bash
# Mehrere SSE-Clients simulieren
for i in {1..10}; do
    curl -N http://127.0.0.1:9969/stream &
done

# Memory-Usage überwachen
watch -n 1 'ps aux | grep usbget'
```

### Compatibility Testing
- **TPMS-Hardware:** Alle 4 Sensoren live empfangen
- **Oil-Hardware:** Analog-Reads funktionieren
- **Browser-App:** Alte Apps mit anderen WebSocket-Quellen unverändert

---

## ⚠️ Bekannte Fallstricke & Lösungen

### Arduino Memory (2KB SRAM)
```c
// PROBLEM: String-Building kann Stack overflowßen
char buffer[200];  // Für komplexe JSON gefährlich

// LÖSUNG: Statische Puffer + snprintf bounds checking
static char line[160];  // Global, nicht auf Stack
int len = snprintf(line, sizeof(line), "data: {...}");
if (len >= sizeof(line)) { /* Handle overflow */ }
```

### USB-Verbindung Stabilität
```c
// PROBLEM: USB-Disconnect bricht Daemon ab
// LÖSUNG: Reconnect-Logic + Heartbeat
void check_usb_health() {
    if (!usbHealthy) {
        log("USB disconnected, attempting reconnect...");
        usbClose();
        sleep(1);
        if (usbConnect(device) == 0) {
            usbSendCommand("B\n");  // Broadcast restart
        }
    }
}
```

### SSE Client Cleanup
```c
// PROBLEM: Clients disconnecten, Server merkt es nicht
// LÖSUNG: Periodic write + error handling
void cleanup_dead_sse_clients() {
    for (int i = 0; i < num_sse_clients; i++) {
        if (write(clients[i].fd, "\n", 1) < 0) {
            // Client ist weg
            close_and_remove_client(i);
        }
    }
}
```

---

## 🔧 Entwicklungs-Environment

### Voraussetzungen 
- **Hardware:** Arduino Nano mit TPMS-Setup, MZD CMU mit USB-Access
- **Cross-Compile:** m3-toolchain für ARM Cortex-A9 (`arm-cortexa9_neon-linux-gnueabi-g++`)
- **Libraries:** libusb-1.0, optional microhttpd oder cJSON der HTTP-Route
- **Testing:** curl, Browser mit Developer Tools, `lsusb` für USB-Debugging

### Build-System
```makefile
# usbget/Makefile: Daemon-Mode hinzufügen
CFLAGS += -DDAEMON_SUPPORT
LIBS += -lmicrohttpd -lcjson

usbget: usbget.o protocol.o usb.o http_server.o json_builder.o
    $(CC) -o $@ $^ $(LIBS)
```

---

## 🞇ℹ Technische Notizen

**Hardware:** Arduino Nano (ATmega328P, 2KB SRAM), MZD CMU (ARM Cortex-A9)  
**Kommunikation:** USB Serial 19200 baud, ~480 bytes/250ms Budget  
**Sensoren:** 4x TPMS 433MHz (CC1101), 2-5x Oil (Analog), ~18 Werte total  
**Ziel-Performance:** 4Hz Stream, ~180 bytes JSON, <250ms Latenz

**Kompatibilität:** Andere Apps mit anderen WebSocket-Quellen bleiben unverändert
