# 🔌 AsyncHTTPClientLight  
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)


Libreria leggera e modulare per gestire richieste HTTP per ESP32, 
pensata per offrire massima flessibilità, controllo e affidabilità in modalità asincrona.

---

## 🚀 Caratteristiche principali

- Gestione non bloccante tramite `poll()`
- Supporto per redirect HTTP (301, 302, 307)
- Compatibilità con chunked encoding
- Logging avanzato con titoli personalizzati
- Timeout configurabile
- Protezione da overload con callback dedicato
- Compatibile con `WiFiClientSecure` per HTTPS
- Logging su seriale, SPIFFS o SD (attivabile via `#define ASYNC_HTTP_DEBUG`)
- Esempi inclusi: GET, POST, HTTPS, AsyncTestServer

---

## 📚 Esempi inclusi

- `GET_Example.ino`
- `POST_Example.ino`
- `HTTPS_secureClient_Example.ino`
- `AsyncTestServer.ino` → server web per testare richieste in tempo reale

---

## 🧰 Setup rapido

```cpp
#include "AsyncHTTPClientLight.h"
Per abilitare il debug:

cpp
#define ASYNC_HTTP_DEBUG
#define ASYNC_HTTP_LOG_SPIFFS // oppure ASYNC_HTTP_LOG_SD
🧪 Esempio asincrono
cpp
client.addTitle("📦 Invio dati sensore");
client.beginRequest("https://api.example.com/data", "POST", jsonPayload);
Nel loop():

cpp
client.poll();
if (client.isFinished()) {
  // Richiesta completata
}
⚠️ Overload intelligente
cpp
client.onOverload([](const String& lostTitle) {
  Serial.println("❌ Overload: richiesta ignorata -> " + lostTitle);
});
💾 Logging su file
cpp
client.setLogToFile(true); // Salva su SPIFFS o SD
Il file http_log.txt viene creato automaticamente nella root e contiene:

Code
[REQ 1] === Invio dati sensore ===
[REQ 1] Inizio richiesta POST a https://api.example.com/data
[REQ 1] Richiesta completata
🔐 HTTPS supportato
cpp
secureClient.setInsecure(); // oppure setCACert(...) per certificati validi
🧠 Note finali
Questa libreria è pensata per essere leggera, affidabile e facilmente integrabile in progetti embedded.
 Ogni funzione è progettata per offrire controllo senza complicazioni,
 e ogni log è pensato per aiutarti a capire cosa succede sotto il cofano.

## 📜 Licenza

Questo progetto è distribuito sotto licenza MIT.  
Puoi usarlo, modificarlo e condividerlo liberamente.  
Vedi il file `LICENSE` per i dettagli.
