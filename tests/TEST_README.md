# 🧪 Complete Test Suite per AsyncHTTPClientLight

Questo sketch è un **banco di prova completo e professionale** per la libreria AsyncHTTPClientLight.
Include un server web locale, test interattivi, dashboard real-time e stress test.

---

## 📋 Cosa contiene

✅ **Server web locale** — Gira sulla stessa ESP32 per test senza dipendenze esterne  
✅ **13+ test interattivi** — Da comandi seriali, uno per ogni scenario  
✅ **Dashboard HTML** — Real-time sul browser, mostra le richieste  
✅ **Stress test** — 100 richieste consecutive per verificare stabilità memoria  
✅ **Logging completo** — Su Serial e su file SD/LittleFS  
✅ **Task FreeRTOS** — Mostra come usare `runSync()` senza bloccare il main  
✅ **Gestione errori** — Timeout, retry, overload, redirect, chunked  

---

## 🚀 Quick Start

### 1. Setup Hardware
- ESP32 con WiFi attivo
- SD card (opzionale, per log file)
- PSRAM abilitato (Sketch → Properties → PSRAM: "OPI PSRAM")

### 2. Setup Software
```
Arduino IDE → Sketch → Include Library → Manage Libraries:
  ✅ AsyncHTTPClientLight (questa libreria)
  ✅ ESPAsyncWebServer
  ✅ AsyncTCP
```

### 3. Configura Credenziali
Nel file, modifica:
```cpp
#define WIFI_SSID "TUO_SSID"
#define WIFI_PASS "TUA_PASSWORD"
```

### 4. Carica e Apri Serial Monitor
```
Baud Rate: 115200
Digita: H
```

---

## 📡 Comandi Disponibili

### Richieste di Base

| Cmd | Descrizione | Cosa testa |
|-----|-------------|-----------|
| `1` | GET asincrono | Richiesta base GET, header custom |
| `2` | POST asincrono | Payload JSON, modalità asincrona |
| `3` | GET con timeout | Timeout 3s + retry automatico |
| `4` | Overload | Invii multipli, gestione overload |
| `5` | GET sincrono | runSync() in task separato (non blocca) |
| `6` | Redirect 302 | Segue redirect automaticamente |
| `7` | Async + Sync mista | Usa entrambe contemporaneamente |
| `8` | Meteo (PSRAM) | Buffer esterno 50KB, parse JSON |
| `9` | Chunked GET | Transfer-encoding: chunked |

### Test Avanzati

| Cmd | Descrizione | Cosa testa |
|-----|-------------|-----------|
| `B` | Large stream | 40KB di dati in streaming |
| `J` | DELETE | Metodo HTTP DELETE (jsonplaceholder) |
| `S` | Stress test | 100 richieste consecutive, memoria stabile? |
| `C` | Chunked long | Chunked transfer lungo |

### Utilities

| Cmd | Descrizione |
|-----|-------------|
| `:192.168.1.100` | Imposta server locale (es. se cambi IP) |
| `http://example.com/path` | GET custom a qualsiasi URL |
| `M` | Stampa info memoria (heap, PSRAM, max alloc) |
| `V` | Visualizza `/http_log.txt` (log file) |
| `D` | Cancella `/http_log.txt` |
| `H` | Mostra questo help |

---

## 🎯 Scenari d'uso

### Scenario 1: Test GET semplice
```
Digita: 1
Output:
  ✅ Risposta ricevuta: titolo: Richiesta GET locale n: 1
  Codice HTTP: 200
  Lunghezza: 45   Type: text/plain
  [Payload ricevuto]
```

### Scenario 2: Test Timeout + Retry
```
Digita: 3
Output:
  → GET con TIMEOUT (3s)
  ⏱ ===== TIMEOUT:
  Titolo: Richiesta TIMEOUT locale
  [Retry automatico]
  ✅ Risposta ricevuta dopo retry
```

### Scenario 3: Test Overload
```
Digita: 4
Output:
  → OVERLOAD (invii multipli)
  ⚠️ ===== Overload ignorata: 
  [Seconda richiesta viene rifiutata]
```

### Scenario 4: Test Redirect 302
```
Digita: 6
Output:
  → GET con REDIRECT 302
  [La libreria segue il redirect automaticamente]
  ✅ Risposta finale ricevuta
```

### Scenario 5: Modalità Mista (Async + Sync)
```
Digita: 7
Output:
  → Async + Sync mista (contemporanea)
  [Avvia GET asincrono]
  [Subito dopo, avvia GET sincrono]
  [runSync() completa la richiesta asincrona, poi avvia la sua]
  ✅ Due risposte ricevute
```

### Scenario 6: Stress Test (100 richieste)
```
Digita: S
Output:
  🔥 STRESS TEST CON 100 RICHIESTE
  [Stampa memoria prima]
  ................................................................
  [Stampa memoria dopo ogni 10 richieste]
  ✅ STRESS TEST COMPLETATO
  [Memoria è stabile? ✅ No memory leak? ✅]
```

### Scenario 7: Visualizza Log File
```
Digita: V
Output:
  ========== START LOG FILE ==========
  [REQ 1] === Richiesta GET locale n: 1 ===
  [REQ 1] Protocollo: HTTP
  [REQ 1] Host: 192.168.x.x
  [REQ 1] Porta: 80
  [REQ 1] Tentativo 1
  [REQ 1] Connessione riuscita
  [REQ 1] Status code: 200
  ========== END LOG FILE ==========
```

---

## 🌐 Dashboard Web

Mentre lo sketch è in esecuzione, apri il browser:

```
http://<ESP32_IP>/
```

Il dashboard mostra:
- **IP del server** — l'indirizzo della ESP32
- **Ultimo Metodo** — GET, POST, DELETE ricevuti
- **Ultimo Body** — payload ricevuto (POST)
- **Ultimi Headers** — header HTTP ricevuti
- **Console live** — log in tempo reale di tutte le richieste

Aggiorna automaticamente ogni secondo. Utile per **debugging visuale**.

---

## 📊 Info Memoria

Comando `M` stampa:

```
Memoria libera: 247436 bytes (61859 floats)
Ram size: 301584 B
Free ram: 247436 B
Max alloc ram: 114376 B
```

Usalo prima e dopo lo stress test per verificare:
- ✅ La memoria libera non scende troppo
- ✅ No memory leak

---

## 🔍 Come Leggere i Log

### Output Serial (in tempo reale)
```
✅==== Risposta ricevuta: titolo: Richiesta GET locale n: 1
Codice HTTP: 200
Lunghezza: 45   Type: text/plain
====== END =====
```

**Significato:**
- ✅ = HTTP 200 (successo)
- ❌ = HTTP errore o timeout
- ⏱ = Timeout (retry automatico)
- ⚠️ = Overload (richiesta rifiutata)

### Log File (/http_log.txt)
Digita `V` per visualizzare. Contiene:
```
[REQ 1] === Mio Test ===
[REQ 1] Protocollo: HTTP
[REQ 1] Host: 192.168.1.100
[REQ 1] Porta: 80
[REQ 1] Path: /events
[REQ 1] Tentativo n:1
[REQ 1] Connessione riuscita
[REQ 1] Sending...
[REQ 1] Status code: 200
[REQ 1] Content-Length: 45
[REQ 1] Content-Type: text/plain
```

---

## ⚙️ Configurazione Avanzata

### Cambia il server locale
Per default il test usa `http://127.0.0.1` (localhost sulla ESP32 stessa).

Se vuoi usare un'altra ESP32:
```
Digita: :192.168.1.50
Output: ✅ Server impostato: 192.168.1.50
```

Ora tutti i comandi useranno `http://192.168.1.50`.

### Abilita/Disabilita log file
Nel codice, modifica:
```cpp
client.setLogToFile(true);   // abilita
client.setLogToFile(false);  // disabilita
```

### Cambia timeout
Nel loop dei comandi, prima di `beginRequest()`:
```cpp
client.setTimeout(5000);      // 5 secondi
client.beginRequest(url, "GET");
client.setTimeout(10000);     // ripristina
```

### Cambia retry
```cpp
client.setMaxRetries(5);      // Riprova 5 volte su timeout
```

### Cambia redirect
```cpp
client.setmaxRedirects(5);    // Segui max 5 redirect
```

---

## 🔧 Troubleshooting

### "❌ WiFi fallito"
→ Controlla SSID e password nel codice  
→ Verifica che la WiFi della rete sia accesa

### "❌ SD card non trovata"
→ Opzionale. Il test funziona anche senza (solo log su Serial)  
→ Se vuoi i log su SD, controlla il pin CS (default 46)

### "❌ Server WEB non risponde"
→ Ricopia l'indirizzo IP da Serial Monitor  
→ Verifica che ESP32 e PC siano sulla stessa rete

### "⚠️ Overload" quando faccio due GET
→ Normale! La libreria accetta una sola richiesta alla volta  
→ Aspetta che la prima finisca (`isFinished()`), poi fai la seconda

### "⏱️ Timeout su comando 3"
→ Perfetto! È il test del timeout. Vedi il retry automatico.

### "Memoria che scende molto nello stress test"
→ Digita `M` prima e dopo lo stress test  
→ Se torna ai livelli iniziali = no memory leak ✅

---

## 📚 Cosa Imparare da Questo Sketch

Leggendo il codice puoi capire:

1. **Come usare `beginRequest()` + `poll()`** (asincrono)
   ```cpp
   client.beginRequest(url, "GET");
   // nel loop:
   client.poll();
   if (client.isFinished()) {
     Serial.println(client.getResponsePayload());
   }
   ```

2. **Come usare `runSync()`** (sincrono)
   ```cpp
   int httpCode = client.runSync(url, "GET");
   Serial.println(client.getResponsePayload());
   ```

3. **Come usare la callback**
   ```cpp
   void handleHTTPEvent(HTTPEventType type, const HTTPResponse* res) {
     switch (type) {
       case HTTPEventType::Response:
         Serial.printf("HTTP %d\n", res->statusCode);
         break;
       // ... altri eventi
     }
   }
   client.onEvent(handleHTTPEvent);
   ```

4. **Come gestire buffer grandi** (PSRAM)
   ```cpp
   char* bigBuffer = (char*)ps_malloc(50000);
   client.setResponsePayload(bigBuffer, 50000);
   // ... richiesta
   free(bigBuffer);
   ```

5. **Come testare stabilità** (stress test)
   ```cpp
   dumpHeapInfo();       // prima
   for (int i = 0; i < 100; i++) {
     client.runSync(url, "GET");
   }
   dumpHeapInfo();       // dopo (dovrebbe essere simile)
   ```

---

## 🎓 Prossimi Passi

Dopo aver familiarizzato con questo test suite:

1. **Guarda gli examples/** — Casi d'uso reali (Google Sheets, Adafruit IO)
2. **Leggi il README.md** — Documentazione completa della libreria
3. **Adatta il codice ai tuoi bisogni** — Usa `beginRequest()` + `poll()` nel tuo loop

---

## 📞 Support

Se qualcosa non funziona:

1. Controlla i log: digita `V`
2. Abilita debug: `client.setDebug(true)`
3. Controlla la memoria: digita `M`
4. Prova con il server locale prima (comandi 1-9)
5. Poi prova con URL esterni

---

**Happy testing! 🚀**
