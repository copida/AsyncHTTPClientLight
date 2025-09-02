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

## FUNZIONALITA'
Caratteristiche principali:
- Richieste Asincrone e Sincrone
- Gestione non bloccante tramite poll()
- Supporto per redirect HTTP (301, 302, 307) redirect HTTP (max 1)
- Compatibilità con chunked encoding
- Header personalizzati
- Logging avanzato con titoli personalizzati
- Logging su seriale e SPIFFS (attivabile via `#define ASYNC_HTTP_DEBUG`)

Timeout configurabile
Protezione da overload
Compatibile con WiFiClientSecure per HTTPS
Esempi inclusi: GET, POST, HTTPS, AsyncTestServer


 progettata per richieste GET/POST non bloccanti,
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

## Setup rapido
```cpp
#include "AsyncHTTPClientLight.h"

AsyncHTTPClientLight client;
client.beginRequest("http://example.com/data", "POST", jsonPayload);

void loop();
client.poll();

if (client.isFinished()) {
  Serial.println(client.getResponse());
}
```

## Modalità sincrona
```cpp
//NO: client.poll();

int codhttp = client.runSync("http://example.com/data", "GET", "");

if (codhttp == 200) {

  Serial.println(client.getResponse());
  
}
```

## Modalità Mista
```cpp
void loop(){
client.poll();

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
| getResponse()                     | Restituisce il corpo della risposta   |
| getStatusCode()                   | Restituisce il codice HTTP            |
| onEvent(callback)                 | Callback per eventi HTTP              |
| addTitle("Titolo")                | Etichetta per logging                 |
| setTimeout(millis)                | imposta Timeout richieste             |
| setDebug(true)                    | Abilita debug log                     |
| setLogToFile(true)                | Attiva log su SPIFFS o SD             |
| setMaxRetries(3)                  | Num. tentativi x timeout (default 1)  |


##  CALLBACK UNIFICATA ###############
es: tipica callback...
```cpp
    client.onEvent([](HTTPEventType type, const String& msg) {
    switch (type) {
    case HTTPEventType::Response:
      Serial.println("Risposta ricevuta: " + msg);
      Serial.println("Codice HTTP: " + String(client.getLastHTTPcode()));
      break;
    case HTTPEventType::Timeout:
      Serial.println("Timeout: " + msg);
      break;
    case HTTPEventType::Error:
      Serial.println("Errore: " + msg);
      break;
    case HTTPEventType::Chunk:
      Serial.println("Chunk ricevuto: " + msg);
      break;
    case HTTPEventType::Overload:
      Serial.println("Richiesta ignorata: " + msg);
      break;
    }
    });
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

Per abilitare il debug:
```cpp
#define ASYNC_HTTP_DEBUG
#define ASYNC_HTTP_LOG_SPIFFS // oppure ASYNC_HTTP_LOG_SD
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


