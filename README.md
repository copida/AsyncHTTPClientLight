AsyncHTTPClientLight — Libreria HTTP asincrona (e sincrona!) per ESP32

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)
![Version](https://img.shields.io/badge/version-2.0.0-lightgrey.svg)

Autore: Davide
Licenza: MIT
Versione: 2.0 (AsyncClient Evolution)

## Introduzione
AsyncHTTPClientLight è una libreria leggera e modulare per gestire richieste HTTP su ESP32
Pensata per ambienti embedded, offre un'interfaccia asincrona non bloccante,
ma include anche una modalità sincrona intelligente per chi desidera semplicità e immediatezza.

# AsyncHTTPClientLight

📡 **Libreria HTTP leggera per ESP32 con supporto asincrono, sincrono, HTTPS, redirect, logging e callback.**

Una libreria C++ ottimizzata per microcontrollori ESP32 che offre connessioni HTTP/HTTPS non bloccanti (asincrone) e bloccanti (sincrone), con gestione automatica di redirect, retry, timeout e trasferimento chunked.

---

## 🎯 Caratteristiche Principali

✅ **Modalità Asincrona** - Richieste non bloccanti con state machine  
✅ **Modalità Sincrona** - Richieste bloccanti (per compatibility)  
✅ **HTTPS/SSL** - Supporto completo per connessioni crittografate  
✅ **Redirect Automatico** - Gestione 301, 302, 303, 307, 308  
✅ **Retry Intelligenti** - Riconnessioni automatiche con configurazione  
✅ **Chunked Transfer** - Supporto transfer-encoding: chunked  
✅ **Stream di Dati** - Lettura payload a lunghezza fissa  
✅ **Logging Avanzato** - Debug su Serial e file (SD/SPIFFS/LittleFS)(attivabile via `#define ASYNC_HTTP_DEBUG`)
✅ **Callback Unificati** - Un'unica callback per tutti gli eventi  
✅ **Header Personalizzati** - Aggiungi qualsiasi header HTTP  
✅ **FreeRTOS Compatible** - Integrato con task ESP32  

---

Timeout configurabile
Protezione da overload
Compatibile con WiFiClientSecure per HTTPS
Esempi inclusi: GET, POST, HTTPS, AsyncTestServer


 progettata per richieste GET/POST PUT DELETE non bloccanti,
 con supporto a redirect, chunked transfer, header personalizzati e logging modulare.


✅ Asincrona per prestazioni
✅ Sincrona per praticità
✅ Unica API, doppia anima


## Filosofia
- Leggerezza: nessun overhead inutile
- Controllo: ogni fase della richiesta è gestibile
- Flessibilità: compatibile con loop, RTOS, e modalità sincrona
- Trasparenza: logging dettagliato su seriale, SPIFFS o SD

## Installazione
- Clona o scarica il repository
- Copia la cartella AsyncHTTPClientLight nella tua cartella libraries
- Assicurati di avere WiFiClientSecure per HTTPS

### Dipendenze
- **Arduino.h** (standard ESP32)
- **WiFiClient.h** (standard ESP32)
- **WiFiClientSecure.h** (standard ESP32)
- **vector** (STL standard)
- **functional** (STL standard)
- **SD.h** (opzionale, per logging)
- **SPIFFS.h** (opzionale, per logging)
- **LittleFS.h** (opzionale, per logging)


## Setup rapido
```cpp
#include "AsyncHTTPClientLight.h"

AsyncHTTPClientLight client;
client.onEvent(handleHTTPEvent);

client.beginRequest("http://example.com/data", "POST", jsonPayload);

void loop();
client.poll();

if (client.isFinished()) {
  Serial.println(client.getLastHTTPcode());
}
```

## Modalità sincrona
```cpp
//NO: client.poll();

//HTTPS POST sincrona
void testSync() {
  http.setDebug(true);
  http.addTitle("Test POST Sync");

  const char* payload = "{\"name\":\"ESP32\"}";
  int status = http.runSync("https://httpbin.org/post", "POST", payload);

  Serial.printf("Codice HTTP: %d\n", http.getLastHTTPcode());
  Serial.println(client.getResponse());
}

```

## Modalità Mista
```cpp
void loop(){
client.poll();
//.....
client.beginRequest("http://example.com/data", "POST", jsonPayload);
int codhttp = client.runSync("http://example.com/data", "GET");
if(codhttp != 200)....

//La funzione runSync() rileva se è in corso una richiesta asincrona
// e la porta a termine attivamente prima di avviare la propria.
```

--------------------------------------------------------------------

🔁 API principali

| Funzione                          | Descrizione                           |
|-----------------------------------|---------------------------------------|
| beginRequest(url,method, payload) | Avvia una richiesta asincrona         |
| runSync(url, method, payload)     | Avvia una richiesta asincrona         |
| poll()                            | Gestisce lo stato interno             |
| isFinished()                      | Verifica se la richiesta è completata |
| setResponsePayload(char* buffer, size_t maxLen)| Set Buffer esterno per payload|
| getLastHTTPcode()                 | Restituisce il codice HTTP            |
| onEvent(callback)                 | Callback per eventi HTTP              |
| addTitle("Titolo")                | Etichetta per logging                 |
| setTimeout(millis)                | imposta Timeout richieste             |
| setDebug(true)                    | Abilita debug log                     |
| setLogToFile(true)                | Attiva log su SPIFFS LittleFS o SD    |
| setMaxRetries(3)                  | Num. tentativi x timeout (default 1)  |
| setmaxRedirects(2)                |Imposta max redirect da seguire (default: 1

Struttura response:

| Nome                              | Descrizione                           |
|-----------------------------------|---------------------------------------|
| int statusCode                    | risposta codice HTTP, -1 se errore    |
| char* inprogressTitle             | Titolo Richiesta                      |
| char* contentType                 | Type content                          |
| int contentLength                 | Lunghezza payload                     |
| char* ptr_workbuffer              | Puntatore risposta                    |
| char msg_error                    | Messaggi Errori                       |


##  CALLBACK UNIFICATA ###############
es: tipica callback...
🔄 Eventi supportati (callback)
##Evento	Descrizione:.

HTTPEventType::Response	Risposta HTTP ricevuta.

HTTPEventType::Error	Timeout, connessione fallita.

HTTPEventType::Chunk	Dati chunk ricevuti (parziale).

HTTPEventType::Overload	Richiesta già in corso.

```cpp
    http.onEvent(HTTPEventType type, const HTTPResponse *res) {
    if (type == HTTPEventType::Response) {
      Serial.printf("Status: %d\n", res->statusCode);
      Serial.println("Payload:");
      Serial.println(res->ptr_outbuffer); // Contenuto ricevuto
    }
    if (type == HTTPEventType::Error) {
      Serial.printf("Errore: %s\n", res->msg_error);
    }
  });
```
es: callback completa...
```cpp

void handleHTTPEvent(HTTPEventType type, const HTTPResponse* response) {
  switch (type) {
    case HTTPEventType::Response:
      Serial.println("✅ Risposta ricevuta!");
      Serial.printf("Codice HTTP: %d\n", response->statusCode);
      Serial.printf("Tipo: %s\n", response->contentType);
      Serial.printf("Dimensione: %d bytes\n", response->contentLength);
      Serial.printf("Payload: %s\n", http.getResponsePayload());
      break;
      
    case HTTPEventType::Error:
      Serial.printf("❌ Errore: %s\n", response->msg_error);
      break;
      
    case HTTPEventType::Timeout:
      Serial.printf("⏱️ Timeout: %s\n", response->msg_error);
      break;
      
    case HTTPEventType::Overload:
      Serial.println("⚠️ Richiesta già in corso!");
      break;
      
    case HTTPEventType::Chunk:
      Serial.printf("📦 Chunk ricevuto: %d bytes\n", response->contentLength);
      break;
      
    default:
      break;
  }
}

```
---------------------------------------------------
🔐 HTTPS
```cpp
WiFiClientSecure secureClient;

secureClient.setInsecure(); // oppure setCACert(...)

client.setClient(&secureClient);
```
--------------------------------------------------


🧰 Esempi inclusi
- GET_Example.ino
- POST_Example.ino
- HTTPS_secureClient_Example.ino
- AsyncTestServer.ino → server web per testare richieste in tempo reale

🌐 Server di test
Il file AsyncTestServer.ino ospita un server web su ESP32 per simulare risposte HTTP.
Puoi usarlo per testare:
- Ricezione di richieste GET/POST
- Logging degli header ricevuti
- Risposte personalizzate con server.sendHeader(...)
- In test EMBEDDED ( server + client) utilizzare RTOS...

🧠 Modalità mista (asincrona + sincrona)
La libreria è progettata per funzionare in ambienti misti:
- Nel loop() puoi usare poll() per gestire richieste asincrone
- Quando serve una risposta immediata, usa runSync(...)
- La libreria gestisce internamente eventuali conflitti

### Setup rapido

#include "AsyncHTTPClientLight.h"

Per abilitare il debug (AsyncHTTPClientLight.cpp):
```cpp
#define ASYNC_HTTP_DEBUG 1
#define ASYNC_HTTP_LOG_SPIFFS // oppure ASYNC_HTTP_LOG_SD
```
Disabilitazione debug dalla compilazione (less -3kb):
```cpp
#define ASYNC_HTTP_DEBUG 0
```


🧪 Esempio asincrono
```cpp
client.addTitle("📦 Invio dati sensore");
client.beginRequest("https://api.example.com/data", "POST", jsonPayload);
Nel loop():

client.poll();
if (client.isFinished()) {
  // Richiesta completata
}
```

💾 Logging su file
```cpp
client.setLogToFile(true); // Salva su SPIFFS o SD
```
Il file http_log.txt viene creato automaticamente nella root e contiene:
```txt
Code
[REQ 1] === Invio dati sensore ===
[REQ 1] Inizio richiesta POST a https://api.example.com/data
[REQ 1] Richiesta completata
```

## HTTPS supportato
```cpp
secureClient.setInsecure(); // oppure setCACert(...) per certificati validi
```

Buffer Personalizzato
void setResponsePayload(char* buffer, size_t maxLen)
Usa un buffer esterno anziché quello interno (768 byte).
```cpp
char myBuffer[2048];
http.setResponsePayload(myBuffer, sizeof(myBuffer));
http.beginRequest("https://api.example.com/large-data");
// La risposta verrà memorizzata in myBuffer
```
const char* getResponsePayload()
Ottiene il buffer della risposta.
```cpp
const char* payload = http.getResponsePayload();
```

🔍 Strutture Dati
HTTPResponse
Contiene i dati della risposta ricevuta.
```cpp
struct HTTPResponse {
    int statusCode;           // 200, 404, 500, ecc.
    uint32_t restime;         // Tempo di risposta (ms)
    char inprogressTitle[64]; // Titolo della richiesta
    char contentType[45];     // "application/json", ecc.
    int contentLength;        // Lunghezza payload (byte)
    bool isStream;            // Trasferimento a lunghezza fissa
    bool isChunked;           // Trasferimento chunked
    int expectedLength;       // Lunghezza attesa chunk
    char* ptr_workbuffer;     // Buffer di lavoro interno
    char msg_error[40];       // Messaggio errore
};
```
HTTPEventType
Tipi di evento HTTP.
```cpp
enum class HTTPEventType {
  Response,   // Risposta ricevuta
  Error,      // Errore generico
  Timeout,    // Timeout raggiunto
  Overload,   // Richiesta già in corso
  Chunk,      // Dati chunked ricevuti
  Receiving,  // Ricezione in corso
  Line        // Linea ricevuta
};
```
💡 Esempi Avanzati
Esempio 1: GET con Header Personalizzato
```cpp
#include "AsyncHTTPClientLight.h"

AsyncHTTPClientLight http;

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  
  http.setDebug(true);
  http.onEvent(handleEvent);
  
  // Aggiungi header custom
  http.addHeader("Authorization", "Bearer eyJhbGc...");
  http.addHeader("Accept", "application/json");
  
  // Avvia richiesta
  http.addTitle("API Call");
  http.beginRequest("https://api.example.com/user/profile");
}

void loop() {
  http.poll();
  delay(10);
}

void handleEvent(HTTPEventType type, const HTTPResponse* resp) {
  if (type == HTTPEventType::Response && resp->statusCode == 200) {
    Serial.println("Profilo ricevuto:");
    Serial.println(http.getResponsePayload());
  }
}
```
Esempio 2: POST JSON
```cpp
void sendData() {
  String json = R"({
    "temperature": 25.5,
    "humidity": 60,
    "device_id": "ESP32-001"
  })";
  
  http.addHeader("Content-Type", "application/json");
  http.addTitle("Send Telemetry");
  http.beginRequest("https://api.example.com/telemetry", "POST", json);
}

void handleEvent(HTTPEventType type, const HTTPResponse* resp) {
  if (type == HTTPEventType::Response) {
    if (resp->statusCode == 201) {
      Serial.println("✅ Dati inviati con successo!");
    } else {
      Serial.printf("❌ Errore: %d\n", resp->statusCode);
    }
  }
}
```
Esempio 3: Gestione File Chunked
```cpp
// Ricezione dati chunked automaticamente gestita
http.addTitle("Large Download");
http.beginRequest("https://api.example.com/large-file");

void handleEvent(HTTPEventType type, const HTTPResponse* resp) {
  if (type == HTTPEventType::Chunk) {
    // Ogni chunk viene elaborato
    Serial.printf("📦 Chunk: %d bytes ricevuti (totale: %d)\n", 
                  resp->contentLength, resp->expectedLength);
  }
  
  if (type == HTTPEventType::Response) {
    Serial.printf("✅ Download completato: %d bytes\n", 
                  resp->contentLength);
    // Processa i dati in http.getResponsePayload()
  }
}
```
Esempio 4: Retry e Timeout
```cpp
void setup() {
  http.setTimeout(3000);      // 3 secondi timeout
  http.setMaxRetries(5);      // Riprova 5 volte
  http.setmaxRedirects(3);    // Segui max 3 redirect
  
  http.setDebug(true);        // Vedi i tentativi
  http.onEvent(handleEvent);
}

void handleEvent(HTTPEventType type, const HTTPResponse* resp) {
  if (type == HTTPEventType::Timeout) {
    Serial.printf("⏱️ Timeout al tentativo %s\n", resp->msg_error);
    // Riprova automatica se retryCount < maxRetries
  }
  
  if (type == HTTPEventType::Response) {
    if (resp->statusCode == 200) {
      Serial.println("✅ Successo dopo retry!");
    } else {
      Serial.printf("❌ Fallito: HTTP %d\n", resp->statusCode);
    }
  }
}
```
Esempio 5: Logging su File

```cpp
void setup() {
  // Inizializza SD card
  if (!SD.begin(SS_PIN)) {
    Serial.println("SD init failed!");
    return;
  }
  
  http.setDebug(true);
  http.setLogToFile(true);  // Salva su /http_log.txt
  
  // I log appariranno su Serial E su SD card
  http.beginRequest("https://api.example.com/data");
}

// Il file /http_log.txt conterrà:
// [REQ 1] === API Call ===
// [REQ 1] Protocollo: HTTPS
// [REQ 1] Porta: 443
// [REQ 1] Host: api.example.com
// [REQ 1] Path: /data
// [REQ 1] Tentativo n:1
// [REQ 1] Connessione riuscita
// [REQ 1] Sending..
// [REQ 1] Status code: 200
// ...
```

⚙️ Configurazione Logging (avanzato)
Nel file AsyncHTTPClientLight.cpp, modifica le linee iniziali:
```cpp
#define ASYNC_HTTP_DEBUG 1  // 0 = disabilita log

#if ASYNC_HTTP_DEBUG
  // Seleziona UNO di questi filesystem:
  
  #define ASYNC_HTTP_LOG_SD        // SD card
  //#define ASYNC_HTTP_LOG_SPIFFS    // SPIFFS
  //#define ASYNC_HTTP_LOG_LittleFS // LittleFS
  
  #define MAXSIZEFILE_LOG 512000   // Max 512 KB prima rotazione
#endif
```
Log file:

Attivo: /http_log.txt
Precedente (rotazione): /old_Log.txt
------------------------------------------------------------------------

## Tips & Traps
Timeout non rispettato? Prova con un server locale che simuli lentezza (sleep).

Secondo tentativo troppo veloce? Alcuni server ottimizzano la connessione dopo il primo hit.

Dimenticato client.loop()? Senza di lui, niente magia.

URL errato? Controlla bene: anche un https:// al posto di http:// può mandare tutto in tilt.

Debug? Usa Serial.println(client.getStatusCode()); per vedere cosa succede.

🧠 Test consigliati
🌐 URL lento: https://tools-httpstatus.pickup-services.com/200?sleep=10000

🖥️ Server locale con delay

🔄 Retry su http://example.com/fail

-------------------------------------------------------
## Note finali
Questa libreria è pensata per essere leggera, affidabile e facilmente integrabile in progetti embedded.
 Ogni funzione è progettata per offrire controllo senza complicazioni,
 e ogni log è pensato per aiutarti a capire cosa succede sotto il cofano.


📜 Licenza
Questo progetto è distribuito sotto licenza MIT.
Puoi usarlo, modificarlo e condividerlo liberamente.


