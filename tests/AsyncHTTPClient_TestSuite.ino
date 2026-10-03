/*
🧪 Test consigliati
Tipo	URL	Note
https://httpbin.io/
GET semplice	http://httpbin.org/get
	Test header/response

POST con JSON	http://httpbin.org/post
	Test payload

Redirect	http://httpbin.org/redirect/1
	Verifica 302 + Location

Chunked	http://httpbin.org/stream/3
	Test chunk parsing

Timeout simulato	http://10.255.255.1
	IP finto per timeout

404 Not Found	http://httpbin.org/status/404
	Verifica codice errore

http://jsonplaceholder.typicode.com/posts/1  è un'API REST di test che restituisce un oggetto JSON.
*/

/*
  AsyncHTTPClient_TestSuite
  
  Banco di prova completo per AsyncHTTPClientLight.
  Include server web locale, test interattivi, stress test e logging.
  
  DIPENDENZE:
  - AsyncHTTPClientLight (questa libreria)
  - ESPAsyncWebServer
  - AsyncTCP
  - SD.h (per logging, opzionale)
  - WiFi.h (standard ESP32)
  
  SETUP:
  1. Installa le librerie da Arduino Library Manager
  2. Configura WIFI_SSID e WIFI_PASS
  3. Abilita PSRAM (Sketch → Properties → PSRAM: "OPI PSRAM")
  4. Carica lo sketch
  5. Apri Serial Monitor (115200 baud)
  6. Digita 'H' per il menu
  
  Note:
  - Il test server gira sulla stessa ESP32 su porta 80
  - Il dashboard è disponibile su http://<ESP32_IP>/
  - I log si salvano in /http_log.txt (su SD o LittleFS)
  - Lo stress test fa 100 richieste consecutive per verificare stabilità
*/


#include <esp_task_wdt.h>
#include <WiFi.h>
//#include <LittleFS.h>
//#define MY_FS LittleFS
#include <SD.h>
#define MY_FS SD

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncHTTPClientLight.h>


AsyncWebServer server(80);
AsyncHTTPClientLight client;


#define WIFI_SSID "XXXXXXXXXX"
#define WIFI_PASS "XXXXXXXXXXXX"


// Protezioni per body/header
constexpr size_t MAX_BODY_LEN = 4096;     // limite massimo per body accumulato
constexpr size_t MAX_HEADERS_LEN = 2048;  // limite per headers concatenati

// Buffer per response payload esterno (se usato)
static char jsonBuffer[3000];

// -------------------- GLOBALS --------------------
bool FS_mounted = false;

String localIP = "127.0.0.1";
String serverIP = "127.0.0.1";
String lastMethod = "NONE";
String lastBody = "";
String lastHeaders = "";
bool newdati = false;

uint reqcount = 0;

// Example JSON small payload
String jsonPayload = "{\"title\":\"CopiNet\",\"body\":\"Ciao Davide!\",\"userId\":1}";

// Test URLs
const char *urlgoo = "http://httpbin.org/redirect/1";
const char *urlmeteo = "https://wttr.in/Ferrara?lang=it&format=j2";

unsigned long lastPostTime = 0;
const unsigned long interval = 10000;
int postCount = 0;
const int maxPosts = 4;

bool debugstato = true;

// Variabili globali di sincronizzazione
volatile bool tempoScaduto = false;
AsyncWebServerResponse *rispostaGlobale = NULL;

// Contatore globale dei tentativi ricevuti dal server
int contatoreTentativi = 0;

// 1. Dichiariamo un handle globale per monitorare la task
TaskHandle_t xSyncTaskHandle = NULL;


static const char readhelp[] PROGMEM = R"rawliteral(
==============================================================
📡 ISTRUZIONI DI UTILIZZO - AsyncHTTPClient Test Suite
==============================================================
Comandi Seriali:
  :<ip>    -> imposta localIP (es. :192.168.1.10)
  1        -> GET locale (async)
  2        -> POST locale (async)
  3        -> TIMEOUT test (async)
  4        -> OVERLOAD (invii multipli async)
  5        -> GET SINCRONO (eseguito in task separato)
  6        -> GET ASINCRONA REDIRECT
  7        -> ASINCRONA + subito SINCRONA
  8        -> METEO (usa buffer esterno)
  9        -> CHUNKED GET locale
  B        -> LARGE STREAM GET local
  S        -> STRESS TEST (sync task loop)
  F        -> FORMAT LittleFS (MANUALE)*
  V        -> Visualizza /http_log.txt
  D        -> Delete log file
  H        -> Mostra questo help
  J        -> DELETE request a jsonplaceholder (async)
  M        -> Stampa memoria / heap
  <url>    -> qualsiasi URL che inizia con "http" invia GET
* Formattare LittleFS è distruttivo: usa solo se necessario.
==============================================================
)rawliteral";

static const char *streamContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin euismod, purus a euismod
    rhoncus, urna ipsum cursus massa, eu dictum tellus justo ac justo. Quisque ullamcorper
    arcu nec tortor ullamcorper, vel fermentum justo fermentum. Vivamus sed velit ut elit
    accumsan congue ut ut enim. Ut eu justo eu lacus varius gravida ut a tellus. Nulla facilisi.
    Integer auctor consectetur ultricies. Fusce feugiat, mi sit amet bibendum viverra, orci leo
    dapibus elit, id varius sem dui id lacus.</p>
</body>
</html>
)";

static const size_t streamContentLength = strlen_P(streamContent);


String battute[] = {
  "Se sei di buon umore, non ti preoccupare. Ti passerà.",
  "Ogni soluzione genera nuovi problemi.",
  "Se non t'importa dove sei, non ti sei perso.",
  "È incredibile quanto ci vuole a fare una cosa che non stai facendo tu.",
  "Quando c'è bisogno di toccar ferro o legno, ci si accorge che il mondo è fatto di alluminio e plastica",
  "Sorridi... Domani sarà peggio."
};
size_t num_str = sizeof(battute) / sizeof(battute[0]);

static const char htmlContent[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Sample HTML</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>Contenuto di esempio per il test di chunking/streaming...</p>
</body>
</html>
)";
static const size_t htmlContentLength = sizeof(htmlContent) - 1;  // exclude trailing NUL


const char *htmlChunks[] = {
  "<html><body>\n",
  "<h1>Benvenuto Davide!</h1>\n",
  "<p>Questa è una risposta chunked.</p>\n",
  "<p>Ogni blocco è inviato separatamente.</p>\n",
  "</body></html>\n"
};
const size_t numChunks = sizeof(htmlChunks) / sizeof(htmlChunks[0]);


// -------------------- FORWARD DECLARATIONS --------------------
void handleRoot(AsyncWebServerRequest *request);
void handleData(AsyncWebServerRequest *request);
void handleAny(AsyncWebServerRequest *request);
void handleDivertente(AsyncWebServerRequest *request);
void handleChunked(AsyncWebServerRequest *request);
void handleSlow(AsyncWebServerRequest *request);
void handleIP(AsyncWebServerRequest *request);
void handleHTTPEvent(HTTPEventType type, const HTTPResponse *res);

void dumpHeapInfo();
void startWIFI();
void filedebug();
void deletelog();
void readFileLog(const char *nomefile);

void handleSerialCommand(const String &line);
void startSyncRequestTask(const String &url, const String &method = "GET", const String &body = "");
void provastress(const String &urlfinal);


size_t sendChunkCallback(uint8_t *buffer, size_t maxLen, size_t index) {
  size_t pos = 0;
  for (size_t i = 0; i < numChunks; ++i) {
    size_t chunkLen = strlen(htmlChunks[i]);
    if (index < pos + chunkLen) {
      // Siamo dentro htmlChunks[i]
      size_t offsetInChunk = index - pos;
      size_t avail = chunkLen - offsetInChunk;
      // Se possibile ritorniamo l'intero rimanente del chunk (per chunk "atomico");
      // altrimenti ritorniamo al massimo maxLen (la libreria potrebbe chiamare ancora)
      size_t toCopy = (avail > maxLen) ? maxLen : avail;
      memcpy(buffer, htmlChunks[i] + offsetInChunk, toCopy);
      return toCopy;
    }
    pos += chunkLen;
  }
  // oltre la fine -> segnale di fine
  return 0;
}

// Handler che usa beginChunkedResponse
void handleChunked(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", sendChunkCallback);
  request->send(response);
}

String escapeJson(const String &input) {
  String s = input;
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  s.replace("\t", "\\t");
  return s;
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset="UTF-8"><title>Async Test Server</title></head>
    <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
    h2 { color: #333; }
    #console { width:95%; height:400px; background:#000; color:#0F0; padding:10px; overflow-y:scroll; font-family:monospace; white-space:pre-wrap;}
    .button { padding:10px 20px; background:#007BFF; color:#fff; border:none; border-radius:5px; cursor:pointer; }
    </style>
    <body>
      <h2>🧪 Async Test Server</h2>
      <p><b>IP del server:</b> <span id="ipDisplay"></span></p>
      <p><b>Ultimo Metodo:</b> <span id="method"></span></p>
      <p><b>Ultimo Body:</b><pre id="body"></pre></p>
      <p><b>Ultimi Headers:</b><pre id="headers"></pre></p>
      <div id="console"></div>
      <button onclick="clearconsole()" class="button">CLEAR Console</button>
      <label><input type="checkbox" id="Timestamp" checked> Timestamp Enable</label>
      
      <script>
      var consoleDiv = document.getElementById('console');
      document.getElementById("ipDisplay").innerText = location.hostname;
      function clearconsole(){ consoleDiv.innerHTML = ""; }
      function update(){
        fetch("/data").then(r=>r.json()).then(json=>{
          if (!json.method && !json.body && !json.headers) return;
          var checkBox = document.getElementById("Timestamp");
          if (checkBox.checked) {
            const dt = new Date();
            const ts = dt.toISOString().replace('T',' ').split('.')[0];
            consoleDiv.innerHTML += "🕒 " + ts + "\n";
          }
          document.getElementById("method").innerText = json.method;
          document.getElementById("body").innerText = json.body;
          document.getElementById("headers").innerText = json.headers;
          consoleDiv.innerHTML += "method: " + json.method + "\n";
          consoleDiv.innerHTML += "body: " + json.body + "\n";
          consoleDiv.innerHTML += "headers: " + json.headers + "\n";
          consoleDiv.scrollTop = consoleDiv.scrollHeight;
        }).catch(e=>console.warn("Fetch failed:", e));
      }
      setInterval(update, 1000);
      window.onload = update;
      </script>
    </body>
    </html>
  )rawliteral";
  request->send(200, "text/html", html);
}

void handleDivertente(AsyncWebServerRequest *request) {
  String line = battute[random(0, num_str)];
  request->send(200, "text/plain", line);
}

void handleData(AsyncWebServerRequest *request) {
  String j = "{";
  if (newdati) {
    j += "\"method\":\"" + escapeJson(lastMethod) + "\",";
    j += "\"body\":\"" + escapeJson(lastBody) + "\",";
    j += "\"headers\":\"" + escapeJson(lastHeaders) + "\"";
    // reset
    newdati = false;
    lastMethod = "";
    lastBody = "";
    lastHeaders = "";
  } else {
    j += "\"method\":\"\", \"body\":\"\", \"headers\":\"\"";
  }
  j += "}";
  request->send(200, "application/json", j);
}

void handleAny(AsyncWebServerRequest *request) {
  String path = request->url();
  if (path == "/favicon.ico" || path == "/data") {
    request->send(404, "text/plain", "Not found");
    return;
  }

  // Metodo
  if (request->method() == HTTP_GET) lastMethod = "GET";
  else if (request->method() == HTTP_POST) lastMethod = "POST";
  else if (request->method() == HTTP_DELETE) lastMethod = "DELETE";
  else if (request->method() == HTTP_PUT) lastMethod = "PUT";
  else lastMethod = "OTHER";

  // Headers - limite lunghezza accodata
  lastHeaders = "";
  for (int i = 0; i < request->headers(); i++) {
    const AsyncWebHeader *h = request->getHeader(i);
    if (lastHeaders.length() < MAX_HEADERS_LEN - 64) {
      lastHeaders += h->name();
      lastHeaders += ": ";
      lastHeaders += h->value();
      lastHeaders += "\n";
    }
  }

  newdati = true;

  // risposta singola
  String line = battute[random(0, num_str)];
  request->send(200, "text/plain", line);
}

void analyzeurl(AsyncWebServerRequest *request) {
  String path = request->url();
  if (path == "/favicon.ico" || path == "/data") {
    request->send(404, "text/plain", "Not found");
    return;
  }

  // Metodo
  if (request->method() == HTTP_GET) lastMethod = "GET";
  else if (request->method() == HTTP_POST) lastMethod = "POST";
  else if (request->method() == HTTP_DELETE) lastMethod = "DELETE";
  else if (request->method() == HTTP_PUT) lastMethod = "PUT";
  else lastMethod = "OTHER";

  // Headers - limite lunghezza accodata
  lastHeaders = "";
  for (int i = 0; i < request->headers(); i++) {
    const AsyncWebHeader *h = request->getHeader(i);
    if (lastHeaders.length() < MAX_HEADERS_LEN - 64) {
      lastHeaders += h->name();
      lastHeaders += ": ";
      lastHeaders += h->value();
      lastHeaders += "\n";
    }
  }

  newdati = true;
}

// Task in background che conta gli 11 secondi
void TaskRitardo(void *pvParameters) {
  // 1. Aspetta 11 secondi
  vTaskDelay(11000 / portTICK_PERIOD_MS);

  // 2. Imposta il flag per sbloccare il contenuto del pacchetto
  tempoScaduto = true;


  if (rispostaGlobale != NULL) {
    rispostaGlobale->_respond((AsyncWebServerRequest *)rispostaGlobale);
  }

  vTaskDelete(NULL);  // Elimina la task
}

void handleInstabile(AsyncWebServerRequest *request) {
  contatoreTentativi++;

  Serial.printf("[TESTER] Connessione ricevuta. Tentativo numero: %d\n", contatoreTentativi);

  // SCENARIO 1: Primo tentativo del client -> Il server è estremamente lento
  if (contatoreTentativi == 1) {
    Serial.println("[TESTER] Comportamento: LENTO (12 secondi). Forzo il timeout del client...");

    for (int ix = 0; ix < 12; ix++) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      esp_task_wdt_reset();
    }

    request->send(200, "text/plain", "Risposta lenta (arriverà tardi)");
  }

  // SCENARIO 2: Il client ha subìto il timeout e fa il secondo tentativo (Retry)
  // Il server risponde subito, ma con un errore interno (500 Internal Server Error)
  else if (contatoreTentativi == 2) {
    Serial.println("[TESTER] Comportamento: ERRORE 500. Rispondo immediatamente con un fallimento...");

    // Resettiamo il contatore per il prossimo ciclo di test completo
    contatoreTentativi = 0;

    request->send(500, "text/plain", "Errore Interno del Server (Simulato)");
  }
}

void handleSlow(AsyncWebServerRequest *request) {
  Serial.println("SLOW");
  static int i = 0;

  if (i == 0) {
    for (int ix = 0; ix < 10; ix++) {
      vTaskDelay(pdMS_TO_TICKS(1000));  // 1 secondo
      esp_task_wdt_reset();             // facoltativo, se il watchdog è attivo
    }
  }

  (i != 0) ? i = 0 : i++;

  request->send(200, "text/plain", "Risposta lenta");
}


void handleIP(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", serverIP);
}

void handleHTTPEvent(HTTPEventType type, const HTTPResponse *res) {
  switch (type) {
    case HTTPEventType::Response:
      if (res->statusCode == 200 || res->statusCode == 302) {
        Serial.print("\n✅");
      } else {
        Serial.print("\n❌");
      }
      Serial.printf("==== Risposta ricevuta: titolo: %s\n", res->inprogressTitle);
      Serial.printf("Codice HTTP: %d\n", res->statusCode);
      Serial.printf("Lunghezza: %d   Type: %s\n", res->contentLength, res->contentType);

      if (strlen(res->msg_error) > 0 || res->statusCode != 200) {
        Serial.print("❌====ERRORE: ");
        Serial.println(res->msg_error);
      }
      if (strlen(client.getResponsePayload()) > 0) Serial.println(client.getResponsePayload());
      
      break;
    case HTTPEventType::Error:
      Serial.println("❌==== ERRORE:");
      Serial.print("Errore: ");
      Serial.println(res->msg_error);
      break;
    case HTTPEventType::Timeout:
      Serial.println("⏱ ==== TIMEOUT:");
      Serial.printf("Titolo: %s\n", res->inprogressTitle);
      break;
    case HTTPEventType::Overload:
      Serial.println("⚠️ ===== Overload ignorata: ");
      Serial.println(res->msg_error);
      break;
    case HTTPEventType::Chunk:
      //Serial.println("\n======== CHUNK:");
      //Serial.printf("Chunk Lung: %d\nval: %s\n", res->expectedLength, res->ptr_workbuffer ? res->ptr_workbuffer : "");
      Serial.printf("===== CHUNK: Lung: %d\n", res->expectedLength);
      return;
      break;
  }
  Serial.println("====== END =====");
}

// -------------------- UTILITIES --------------------
void dumpHeapInfo() {
  Serial.printf("Memoria libera:%d (%d float)\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) / sizeof(float));
  Serial.print("Ram size: ");
  Serial.println(formatBytes(ESP.getHeapSize()));
  Serial.print("Free ram: ");
  Serial.println(formatBytes(ESP.getFreeHeap()));
  Serial.print("Max alloc ram: ");
  Serial.println(formatBytes(ESP.getMaxAllocHeap()));
}

// helper format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0, 2) + " KB";
  } else if (bytes < (1024ULL * 1024ULL * 1024ULL)) {
    return String(bytes / 1024.0 / 1024.0, 2) + " MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0, 2) + " GB";
  }
}

// -------------------- SERIAL COMMANDS & SYNC TASK --------------------

// Task per eseguire richieste sincrone evitando blocco del main loop
struct SyncTaskParams {
  String url;
  String method;
  String body;
};


void syncRequestTask(void *pv) {
  SyncTaskParams *p = static_cast<SyncTaskParams *>(pv);
  Serial.printf("SyncTask: eseguo %s %s\n", p->method.c_str(), p->url.c_str());
  
  int code = client.runSync(p->url, p->method.c_str(), p->body.c_str());
  Serial.printf("SyncTask: result %d\n", code);
  
  delete p;
  
  // PRIMA DI MORIRE: Azzeriamo l'handle globale per dire al tester che abbiamo finito!
  xSyncTaskHandle = NULL; 
  vTaskDelete(NULL);
}

void startSyncRequestTask(const String &url, const String &method, const String &body) {
  SyncTaskParams *p = new SyncTaskParams{ url, method, body };
  
  // Passiamo l'indirizzo di xSyncTaskHandle così FreeRTOS lo popola
  xTaskCreate(syncRequestTask, "SyncReq", 16000, p, 1, &xSyncTaskHandle);
}

// Gestione seriale -
void handleSerialCommand(const String &lineRaw) {

  String line = lineRaw;
  line.trim();
  if (line.length() == 0) return;

  Serial.printf("ESEGUO... %s\n", line.c_str());

  // set IP :192.168.x.y
  if (line.startsWith(":")) {
    String ip = line.substring(1);
    if (ip.length() >= 7) {
      localIP = ip;
      Serial.printf("Impostato nuovo Server: %s\n", localIP.c_str());
    }
    return;
  }

  // single-char commands
  char c = line.charAt(0);
  if (isDigit(c)) {
    int cmd = atoi(line.c_str());
    reqcount++;
    String urlfinal = "http://" + localIP;
    switch (cmd) {
      case 1:
        client.addTitle("Richiesta GET locale n: " + String(reqcount));
        client.addHeader("Content-Type", "application/json");
        client.addHeader("Custom-Header", "DavidePower");
        client.addHeader("Authorization", "Bearer xyz123");
        client.beginRequest(urlfinal + "/events", "GET");
        break;
      case 2:
        client.addTitle("Richiesta POST locale n: " + String(reqcount));
        client.addHeader("Content-Type", "application/json");
        client.beginRequest(urlfinal + "/post", "POST", jsonPayload);
        break;
      case 3:
        client.addTitle("Richiesta TIMEOUT locale n: " + String(reqcount));
        client.setTimeout(3000);
        client.beginRequest(urlfinal + "/slow", "GET");
        client.setTimeout(10000);  // ripristino default
        break;
      case 4:
        client.addTitle("Richiesta OVERLOAD");
        client.beginRequest(urlfinal + "/events", "GET");
        client.addTitle("Richiesta OVERLOAD 2");
        client.beginRequest(urlfinal + "/events", "GET");
        break;
      case 5:
        // run sync in task
        startSyncRequestTask(urlfinal + "/events", "GET", "");
        break;
      case 6:
        //startSyncRequestTask(urlgoo, "GET", "");
        client.addTitle("ASINCRONA REDIRECT GET  locale n: " + String(reqcount));
        client.beginRequest(urlfinal + "/redirect", "GET", "");
        break;
      case 7:
        client.addHeader("Content-Type", "application/json");
        client.addHeader("Custom-Header", "DavidePower");
        client.addHeader("Authorization", "Bearer xyz123");

        client.addTitle("ASINCRONA GET locale n: " + String(reqcount));
        client.beginRequest(urlfinal + "/events", "GET");
        client.addTitle("SINCRONA dopo ASINCRONA locale n: " + String(reqcount));
        client.runSync(urlfinal + "/events", "GET", "");
        // poi una sync subito (in task)
        //startSyncRequestTask(urlfinal + "/events", "GET", "");
        break;
      case 8:
        client.addTitle("Risposta JSON SYNC STREAM");
        client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
        client.beginRequest(urlmeteo, "GET", " ");
        // voglio vedere subito risultato
        while(!client.isFinished()){
          vTaskDelay(pdMS_TO_TICKS(10)); // Aspetta 10ms e ricontrolla
          client.poll();
        }
        Serial.println("=== OUTPUT ====");
        Serial.println(jsonBuffer);
        break;
      case 9:
        client.addTitle("CHUNKED GET locale n: " + String(reqcount));
        client.beginRequest(urlfinal + "/chunked", "GET");
        break;
      default:
        Serial.println("Comando numerico non gestito.");
        break;
    }
    return;
  }

  // letter commands
  switch (c) {
    case 'B':
      client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
      client.addTitle("LARGE STREAM buffer local n: " + String(++reqcount));
      client.beginRequest(String("http://") + localIP + "/stream2", "GET");
      break;
    case 'J':
      client.addTitle("DELETE jsonplaceholder n: " + String(++reqcount));
      client.beginRequest("https://jsonplaceholder.typicode.com/posts/1", "DELETE");
      break;
    case 'C':
      client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
      client.addTitle("CHUNKED LONG GET locale n: " + String(++reqcount));
      client.beginRequest(String("http://") + localIP + "/chunked2", "GET");
      break;
    case 'E':
      //client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
      client.addTitle("CHUNKED LONG GET locale n: " + String(++reqcount));
      client.beginRequest(String("http://") + localIP + "/chunked2", "GET");
      break;
    case 'M':
      dumpHeapInfo();
      break;
    case 'S':
      provastress(String("http://") + localIP + "/events");
      break;
    case 'F':
    #if defined(_LITTLEFS_H_) || defined(__LITTLEFS_H)
      // FORMAT LittleFS (manual): richiedi conferma
      Serial.println("FORMATTARE LittleFS? Digitare 'Y' per confermare (seleziona entro 10s)...");
      {
        unsigned long start = millis();
        while (millis() - start < 10000 && !Serial.available()) {
          delay(100);
        }
        if (Serial.available()) {
          String conf = Serial.readStringUntil('\n');
          conf.trim();
          if (conf == "Y" || conf == "y") {
            if (LittleFS.format()) {
              Serial.println("LittleFS formattato. Prova a montare di nuovo.");
              if (LittleFS.begin()) {
                FS_mounted = true;
                Serial.println("LittleFS mounted OK");
              } else {
                FS_mounted = false;
                Serial.println("LittleFS begin failed after format");
              }
            } else {
              Serial.println("Formato fallito.");
            }
          } else {
            Serial.println("Format annullato.");
          }
        } else {
          Serial.println("Conferma non ricevuta. Format annullato.");
        }
      }
      #endif
      break;
    case 'V':
      readFileLog("/http_log.txt");
      break;
    case 'D':
      deletelog();
      break;
    case 'H':
      Serial.println(readhelp);
      break;
    default:
      // Se inizia con http invia GET
      if (line.startsWith("http")) {
        client.addTitle("Richiesta MANUALE n: " + String(++reqcount));
        client.beginRequest(line, "GET", "");
      } else {
        Serial.println("Comando non riconosciuto. H per help.");
      }
      break;
  }
}


void provastress(const String &urlfinal) {
  Serial.println("PROVA STRESS CON TASK");
  dumpHeapInfo();
  delay(3000);
  reqcount = 0;

  while (reqcount < 100) {
    

    while (xSyncTaskHandle != NULL) {
      vTaskDelay(pdMS_TO_TICKS(10)); // Aspetta 10ms e ricontrolla
      client.poll(); // Continua a fare il polling se necessario alla tua libreria
    }
    
    dumpHeapInfo();
    reqcount++;
    String titolo = "Richiesta GET locale n: " + String(reqcount);
    Serial.println(titolo);
    client.addTitle(titolo);
    
    
    startSyncRequestTask(urlfinal, "GET", "");
    
    
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  dumpHeapInfo();
  Serial.println("END STRESS");
}

// -------------------- FILESYSTEM LOGGING --------------------
void filedebug() {
#ifdef MY_FS
  debugstato = !debugstato;
  client.setLogToFile(debugstato);
  Serial.printf("DEBUG su file %s\n", (debugstato) ? "ATTIVATO" : "DISATTIVATO");
#else
  Serial.println("MY_FS non abilitato: definire MY_FS per abilitare file debug.");
#endif
}

void deletelog() {
#ifdef MY_FS
  if (MY_FS.remove("/http_log.txt")) {
    Serial.println("File di LOG deleted..");
  } else {
    Serial.println("Errore deleting log o file non esiste.");
  }
#else
  Serial.println("MY_FS non abilitato.");
#endif
}

void readFileLog(const char *nomefile) {
#ifdef MY_FS
  if (!FS_mounted) {
    Serial.println("LittleFS non montato.");
    return;
  }
  File file = MY_FS.open(nomefile, "r");
  if (!file) {
    Serial.println("Errore nell'apertura del file o file non esiste.");
    return;
  }
  Serial.println("============= START file:");
  while (file.available()) {
    Serial.println(file.readStringUntil('\n'));
  }
  Serial.println("============= END file:");
  file.close();
#else
  Serial.println("MY_FS non abilitato.");
#endif
}

// -------------------- WIFI --------------------
void startWIFI() {
  Serial.println("\nSetting Station configuration ... ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println(String("Connecting to ") + WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 20000) {
      Serial.println("\nTimeout connecting to WiFi (20s). Will retry in background.");
      // Non bloccare per sempre: ritorna e lascia che l'app tenti di connettersi in background
      return;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup End");
}

// -------------------- SETUP / LOOP --------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // Seed per random
  randomSeed(esp_random());

  Serial.println("Connessione al WiFi...");
  startWIFI();

  // Configuro server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/slow", HTTP_GET, handleSlow);
  
  server.on("/ip", HTTP_GET, handleIP);
  server.on("/divertente", HTTP_GET, handleDivertente);

  
  server.on(
    "/post", HTTP_POST,
    [](AsyncWebServerRequest *request) {
    
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      
      if (index == 0) {
        lastBody = "";
        lastBody.reserve(min((size_t)1024, MAX_BODY_LEN));
      }
      
      if (lastBody.length() + len <= MAX_BODY_LEN) {
        lastBody += String((char *)data, len);
      } else {
        size_t canCopy = MAX_BODY_LEN - lastBody.length();
        if (canCopy > 0) lastBody += String((char *)data, canCopy);
        
      }
      if (index + len == total) {
        
        newdati = true;
        lastMethod = "POST";
        
        request->send(200, "text/plain", "OK");
      }
    });

  server.on("/chunked", HTTP_GET, handleChunked);

  server.on("/chunked2", HTTP_GET, [](AsyncWebServerRequest *request) {
    String etag = String(streamContentLength);

    if (request->header(asyncsrv::T_INM) == etag) {
      request->send(304);
      return;
    }

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      Serial.printf("%u / %u\n", index, streamContentLength);

      // finished ?
      if (streamContentLength <= index) {
        Serial.println("finished");
        return 0;
      }

     
      const int chunkSize = min((size_t)256, min(maxLen, streamContentLength - index));
      Serial.printf("sending: %u\n", chunkSize);

      memcpy(buffer, streamContent + index, chunkSize);

      return chunkSize;
    });
    response->addHeader(asyncsrv::T_Cache_Control, "public,max-age=60");
    response->addHeader(asyncsrv::T_ETag, etag);

    request->send(response);
  });


  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    analyzeurl(request);
    String host = request->host();
    String location = "http://" + host + "/divertente";
    request->redirect(location);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    handleAny(request);
  });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("plain/text", 40 * 1024);
    for (int i = 0; i < 32 * 1024; i++) response->write('a');
    request->send(response);
  });

  server.on("/stream2", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("plain/text", 40 * 1024);
    for (size_t i = 0; i < streamContentLength; i++) response->write(streamContent[i]);
    request->send(response);
  });

  server.begin();
  Serial.println("Server WEB avviato");
  serverIP = WiFi.localIP().toString().c_str();
  Serial.printf("Server IP: %s\n", serverIP.c_str());
  delay(200);

  Serial.println(readhelp);
#if defined(_LITTLEFS_H_) || defined(__LITTLEFS_H)
  LittleFS mount (no format automatico)
  if (!LittleFS.begin()) {
    FS_mounted = false;
    Serial.println("LittleFS Mount Failed. Use Serial 'F' to format if necessary.");
  } else {
    FS_mounted = true;
    Serial.println("LittleFS Mounted OK");
  }
#elif defined(_SD_H_) || defined(__SD_H)

   if (!SD.begin(46)) {
    Serial.println("SD...FAIL");
  } else {
    Serial.println("SD...OK");
    FS_mounted = true;
  }
  #endif

  // Client config
  client.onEvent(handleHTTPEvent);
  client.setDebug(true);
  client.setLogToFile(true);  // abilita tramite filedebug() se MY_FS è definito
  //filedebug();
  client.setMaxRetries(2);
  client.setTimeout(10000);

}

void loop() {
  // Poll del client (necessario)
  client.poll();

  // Lettura input seriale
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleSerialCommand(line);
  }

  vTaskDelay(pdMS_TO_TICKS(10));
}
