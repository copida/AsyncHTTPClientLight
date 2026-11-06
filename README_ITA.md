AsyncHTTPClientLight — Libreria HTTP asincrona (e sincrona!) per ESP32

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)
![Version](https://img.shields.io/badge/version-2.0.0-lightgrey.svg)

Autore: Davide  
Licenza: MIT  
Versione: 2.0 (AsyncClient Evolution)

## 📡 Cos'è

**AsyncHTTPClientLight** è una libreria HTTP leggera per ESP32 che offre richieste non bloccanti (asincrone) e bloccanti (sincrone), con gestione automatica di redirect, retry, timeout e logging dettagliato.

Nata per sostituire **HTTPClient** nei progetti dove la stabilità conta:
- Niente memory leak da connessioni non chiuse bene
- Redirect 302 gestiti correttamente (es. Google Apps Script, Adafruit IO)
- Log leggibile di ogni fase, per capire **perché** una richiesta fallisce
- Memoria prevedibile (buffer di risposta scelto da te)

---

## 🎯 Caratteristiche Principali

✅ **Modalità Asincrona** — Richieste non bloccanti con state machine  
✅ **Modalità Sincrona** — Richieste bloccanti (per setup o test)  
✅ **Modalità Mista** — Usa entrambe nello stesso sketch  
✅ **HTTPS Automatico** — Abilita WiFiClientSecure se l'URL contiene `https://`  
✅ **Redirect Automatico** — Segue 301, 302, 303, 307, 308  
✅ **Retry Intelligenti** — Riconnessioni automatiche configurabili  
✅ **Chunked Transfer** — Supporto `transfer-encoding: chunked`  
✅ **Callback Unificata** — Un'unica callback per tutti gli eventi  
✅ **Header Personalizzati** — Aggiungi header HTTP custom (es. Authorization)  
✅ **Logging Avanzato** — Debug su Serial e file (SD/SPIFFS/LittleFS)  
✅ **Buffer Esterno** — Usa PSRAM per payload grandi (es. 50KB)  
✅ **FreeRTOS Compatible** — Pensata per ambienti multi-task ESP32

---

## 📦 Installazione

### Manuale
1. Clona o scarica il repository
2. Copia la cartella nella tua libreria Arduino: `~/Arduino/libraries/AsyncHTTPClientLight`
3. Riavvia Arduino IDE

### Arduino Library Manager (prossimamente)
`Sketch → Include Library → Manage Libraries → AsyncHTTPClientLight`

### Dipendenze
- **Arduino.h** (standard ESP32)
- **WiFiClient.h** (standard ESP32)
- **WiFiClientSecure.h** (standard ESP32, per HTTPS)
- **vector, functional** (STL standard)
- **SD.h** (opzionale, per logging su SD card)
- **SPIFFS.h** (opzionale, per logging su SPIFFS)
- **LittleFS.h** (opzionale, per logging su LittleFS)

---

## 🚀 Quick Start

### Asincrono (consigliato)
```cpp
#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

AsyncHTTPClientLight http;

void handleEvent(HTTPEventType type, const HTTPResponse* res) {
  if (type == HTTPEventType::Response) {
    Serial.printf("HTTP %d\n", res->statusCode);
    Serial.println(http.getResponsePayload());
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  http.setDebug(true);
  http.onEvent(handleEvent);
  http.addTitle("Mia richiesta");
  http.beginRequest("https://api.example.com/data", "GET");
}

void loop() {
  http.poll();  // ← IMPORTANTE: chiamare a ogni loop()
  delay(100);
}
```

### Sincrono (semplice, ma blocca il loop)
```cpp
void setup() {
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  int httpCode = http.runSync("https://api.example.com/data", "GET");
  Serial.printf("HTTP %d\n", httpCode);
  Serial.println(http.getResponsePayload());
}

void loop() {}
```

---

## 💡 Modalità di utilizzo

### 1️⃣ Asincrona (Non-bloccante)
Ideale per applicazioni real-time dove il loop deve continuare a girare.

```cpp
http.beginRequest(url, "POST", jsonPayload);  // Avvia, non aspetta

// Nel loop():
http.poll();  // Gestisci state machine
if (http.isFinished()) {
  Serial.println(http.getResponsePayload());
}
```

**Vantaggi:**
- Loop non si ferma
- Puoi gestire sensori, pulsanti, display mentre la richiesta è in corso
- Callback notifica quando la risposta arriva

**Svantaggi:**
- Un po' più complessa da usare
- Una sola richiesta alla volta (le altre arrivano con `Overload`)

---

### 2️⃣ Sincrona (Bloccante)
Ideale per setup iniziale, configurazione, oppure sketch semplici.

```cpp
int httpCode = http.runSync(url, "GET");  // Aspetta risposta
Serial.println(http.getResponsePayload());
```

**Vantaggi:**
- Semplice da usare
- API diretta: chiami e hai subito la risposta

**Svantaggi:**
- Loop è bloccato durante la richiesta
- Non puoi controllare sensori/pulsanti/display durante

---

### 3️⃣ Mista
Usa modalità asincrona nel loop, e sincrona dove serve una risposta immediata.

```cpp
void loop() {
  http.poll();  // Asincrona
  if (someEvent) {
    int code = http.runSync(url, "GET");  // Sincrona
    // runSync() completa la richiesta asincrona precedente, se in corso
  }
}
```

---

## 🔁 API Principali

| Funzione | Descrizione |
|----------|-------------|
| `beginRequest(url, method, payload)` | Avvia richiesta asincrona |
| `runSync(url, method, payload)` | Richiesta sincrona, ritorna HTTP code |
| `poll()` | **Chiama a ogni loop()**: gestisce state machine |
| `isFinished()` | Verifica se richiesta completata |
| `getLastHTTPcode()` | Codice HTTP (200, 404, ecc.) |
| `getResponsePayload()` | Buffer della risposta |
| `setResponsePayload(buffer, size)` | Buffer esterno (PSRAM) |
| `addHeader(key, value)` | Aggiungi header HTTP |
| `addTitle(label)` | Etichetta per logging |
| `setTimeout(ms)` | Timeout richiesta (default 10000) |
| `setMaxRetries(n)` | Tentativi su timeout (default 1) |
| `setmaxRedirects(n)` | Max redirect da seguire (default 1) |
| `setDebug(true)` | Abilita log su Serial |
| `setLogToFile(true)` | Salva log su SD/SPIFFS/LittleFS |
| `onEvent(callback)` | Callback per eventi HTTP |

---

## 🔄 Eventi Callback

```cpp
void handleEvent(HTTPEventType type, const HTTPResponse* res) {
  switch (type) {
    case HTTPEventType::Response:
      Serial.printf("✅ HTTP %d\n", res->statusCode);
      Serial.println(http.getResponsePayload());  // ← valido solo qui!
      break;
    
    case HTTPEventType::Timeout:
      Serial.printf("⏱️ Timeout: %s (retry...)\n", res->msg_error);
      break;
    
    case HTTPEventType::Error:
      Serial.printf("❌ Errore: %s\n", res->msg_error);
      break;
    
    case HTTPEventType::Overload:
      Serial.println("⚠️ Richiesta precedente ancora in corso");
      break;
    
    case HTTPEventType::Chunk:
      Serial.printf("📦 Chunk: %d bytes\n", res->contentLength);
      break;
    
    default:
      break;
  }
}

http.onEvent(handleEvent);
```

**Nota importante:** Il payload `http.getResponsePayload()` è valido **solo dentro la callback**. Dopo che la callback torna, il buffer viene liberato per la prossima richiesta.

---

## 🔐 HTTPS

**Automatico.** Se l'URL contiene `https://`, la libreria abilita da sola `WiFiClientSecure` e usa `setInsecure()`.

```cpp
// Questo basta:
http.beginRequest("https://api.example.com/data", "GET");
```

⚠️ **ATTENZIONE:** Il certificato NON viene verificato (`setInsecure()`). La connessione è cifrata ma non autenticata — chi sniffa la rete potrebbe fingersi il server. Va bene per dati non sensibili (meteo, log, telemetria). Per token o credenziali importanti, non è adatto (serve un'upgrade con `setCACert()`).

---

## 📝 Esempi Reali Inclusi

### 1. `Sync_GET.ino` — Modalità sincrona semplice
Mostra GET e POST bloccanti, utile per setup.

### 2. `GoogleSheets_Logger.ino` — Log periodico su Google Sheets
- Invia JSON ogni 60s
- Gestisce il 302 redirect di Apps Script
- Callback unificata
- Riconnessione WiFi automatica

### 3. `Adafruit_IO_History.ino` — Download storico da Adafruit IO
- Buffer PSRAM 50KB
- Parse JSON con ArduinoJson v7
- API key negli header (X-AIO-Key)
- Caso d'uso reale: scarica 1000 record e li processa

Copia uno di questi file dal folder `examples/` e adattalo ai tuoi parametri.

---

## 💾 Logging

### Console (Serial)
```cpp
http.setDebug(true);
```
Output di esempio:
```
[REQ 1] === Mia richiesta ===
[REQ 1] Protocollo: HTTPS
[REQ 1] Host: api.example.com
[REQ 1] Porta: 443
[REQ 1] Tentativo 1
[REQ 1] Connessione riuscita
[REQ 1] Sending...
[REQ 1] Status code: 200
[REQ 1] Content-Length: 512
```

### File (SD/SPIFFS/LittleFS)
1. Apri `src/AsyncHTTPClientLight.cpp`
2. Cambia le linee iniziali:
```cpp
#define ASYNC_HTTP_DEBUG 1  // 0 per disabilitare

#if ASYNC_HTTP_DEBUG
  #define ASYNC_HTTP_LOG_SD        // oppure _SPIFFS o _LittleFS
  #define MAXSIZEFILE_LOG 512000   // Max 512 KB prima rotazione
#endif
```
3. Nel tuo sketch:
```cpp
http.setLogToFile(true);
```
4. I log vanno in:
   - `/http_log.txt` (attivo)
   - `/old_Log.txt` (rotazione precedente)

---

## 🧠 Cose importanti

### ✅ Il payload rimane valido solo nella callback
```cpp
// ✅ GIUSTO: leggi dentro la callback
void handleEvent(HTTPEventType type, const HTTPResponse* res) {
  if (type == HTTPEventType::Response) {
    Serial.println(http.getResponsePayload());
  }
}

// ❌ SBAGLIATO: il buffer è stato già liberato
Serial.println(http.getResponsePayload());  // → garbage
```

### ✅ Chiama sempre `poll()` nel loop()
```cpp
void loop() {
  http.poll();  // ← OBBLIGATORIO per modalità asincrona
  // ... resto del codice
}
```

### ✅ Una sola richiesta alla volta
Se avvi una nuova richiesta mentre ce n'è una in corso, ricevi `Overload` nella callback.

```cpp
if (http.isFinished()) {
  http.beginRequest(url, "GET");  // OK
} else {
  Serial.println("Richiesta precedente ancora in corso");
}
```

### ✅ Buffer grande per payload largi
Il buffer interno è 768 byte. Per download di decine di KB, usa PSRAM:

```cpp
char* bigBuffer = (char*)ps_malloc(50000);  // 50 KB
http.setResponsePayload(bigBuffer, 50000);
http.beginRequest(url, "GET");
// Dopo la callback:
free(bigBuffer);
```

---

## 🚨 Troubleshooting

### "Richiesta fallita senza motivo"
→ Abilita il log: `http.setDebug(true)`. Leggi il prefisso `[REQ n]` nel Serial Monitor per capire dove blocca (connessione, timeout, DNS, ecc).

### "Timeout casuale su un server veloce"
→ Aumenta il timeout: `http.setTimeout(15000)` (15 secondi). Alcuni server rispondono lentamente al primo hit.

### "HTTP 302 non viene seguito"
→ Aumenta i redirect: `http.setmaxRedirects(5)`. Default è 1.

### "Risposta parziale o corrotta"
→ Usa un buffer esterno più grande: `http.setResponsePayload(bigBuf, bigSize)`. Il buffer interno è solo 768 byte.

### "Memory leak / crash dopo tante richieste"
→ Controlla che chiami `poll()` **a ogni loop()**. Se la salti, le risorse non si liberano.

### "WiFi cade durante richiesta"
→ Aggiungi controllo WiFi nel loop:
```cpp
if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi perso, riconnetto...");
  WiFi.reconnect();
  return;
}
```

---

## 📊 Strutture Dati

### HTTPResponse
```cpp
struct HTTPResponse {
  int statusCode;            // 200, 404, 500, -1 se errore
  uint32_t restime;          // Tempo di risposta (ms)
  char inprogressTitle[64];  // Etichetta della richiesta
  char contentType[45];      // "application/json", ecc.
  int contentLength;         // Lunghezza payload (bytes)
  bool isStream;             // Trasferimento a lunghezza fissa
  bool isChunked;            // Trasferimento chunked
  int expectedLength;        // Lunghezza attesa chunk
  char* ptr_workbuffer;      // Buffer interno
  char msg_error[50];        // Messaggio errore
};
```

### HTTPEventType
```cpp
enum class HTTPEventType {
  Response,   // Risposta ricevuta (status 200, 404, ecc)
  Error,      // Errore generico (connessione, parsing)
  Timeout,    // Timeout raggiunto (retry automatico)
  Overload,   // Richiesta già in corso
  Chunk,      // Dati chunked ricevuti (parziale)
  Receiving,  // Ricezione in corso
  Line        // Linea ricevuta (non chunked)
};
```

---

## 📖 Ricorda: quando usi cos'è

| Caso d'uso | Usa | Perché |
|-----------|-----|-------|
| Setup WiFi, carica config | `runSync()` | Semplice, una volta sola |
| Sensore periodico ogni 1 min | `beginRequest()` + `poll()` | Loop non si ferma |
| Download file grande | `setResponsePayload()` + `beginRequest()` | Buffer PSRAM |
| API con auth Bearer | `addHeader("Authorization", "Bearer...")` | Header custom |
| Google Sheets o Adafruit IO | `setmaxRedirects(3)` | Gestiscono redirect |
| Debug: perché fallisce? | `setDebug(true)` + Serial | Log dettagliato |

---

## 🧪 Test consigliati

- **Richiesta lenta:** `https://httpbin.org/delay/5` (aspetta 5s)
- **Errore 404:** `https://httpbin.org/status/404`
- **JSON valido:** `https://httpbin.org/json`
- **POST echo:** `https://httpbin.org/post` (ritorna quello che invii)

---

## 📝 Note Finali

Questa libreria è progettata per chi fa IoT, data logging e telemetria su ESP32 e vuole **stabilità** e **trasparenza**. Ogni funzione è stata scelta per offrire controllo senza complicazioni.

Se HTTPClient ti ha dato problemi, AsyncHTTPClientLight risolve quelli più comuni (memory leak, redirect, timeout misterioso).

---

## 📜 Licenza

MIT License — usala, modificala, condividila liberamente.

---

**Maintainer:** Davide (copida.soft@gmail.com)  
**Repository:** https://github.com/copida/AsyncHTTPClientLight
