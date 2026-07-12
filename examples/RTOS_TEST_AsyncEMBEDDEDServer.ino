/*
🧪 Test consigliati
Tipo	URL	Note
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
//#include <SPIFFS.h>
//#define MY_FS SPIFFS

//#include <SD.h>
#include <LittleFS.h>
#define MY_FS LittleFS

bool FS_mounted = false;


char buffer_temp[512];

#include <WiFi.h>
#include <esp_task_wdt.h>


#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);  // Il tuo server HTTP

#include <AsyncHTTPClientLight.h>
AsyncHTTPClientLight client;

const char *ssid = "Vodafone-C01030164";
const char *password = "f4qHtbX2N32qAgad";

String localIP = "127.0.0.1";
String serverIP = "127.0.0.1";
String lastMethod = "NONE";
String lastBody = "";
String lastHeaders = "";
bool newdati = false;

//{"title":"CopiNet","body":"Ciao Davide!","userId":1}
//==============
uint reqcount = 0;
char jsonBuffer[4500];

String json = "{\"title\":\"CopiNet\",\"body\":\"Ciao Davide!\",\"userId\":1}";

const char *urlgoo = "http://httpbin.org/redirect/1";
//https://github.com/chubin/wttr.in
const char *urlmeteo = "https://wttr.in/Ferrara?format=j2";

// per test timeout
//https://tools-httpstatus.pickup-services.com/200?sleep=10000


unsigned long lastPostTime = 0;
const unsigned long interval = 10000;  // 10 sec.
int postCount = 0;
const int maxPosts = 4;
String ipserver = "127.0.0.1";
bool debugstato = true;
bool sincasinc = false;
//-------- x CHUNK
// Contenuto da inviare a chunk
const char *htmlChunks[] = {
  "<html><body>\n",
  "<h1>Benvenuto Davide!</h1>\n",
  "<p>Questa è una risposta chunked.</p>\n",
  "<p>Ogni blocco è inviato separatamente.</p>\n",
  "</body></html>\n"
};

const size_t numChunks = sizeof(htmlChunks) / sizeof(htmlChunks[0]);
//-------------------------

const char readhelp[] PROGMEM = R"rawliteral(
==============================================================
📡 ISTRUZIONI DI UTILIZZO - AsyncHTTPClient Test Suite
==============================================================

👉 Per impostare un nuovo IP SERVER digita: ":192.xxx.xxx.xxx"

🔧 Test disponibili:
   1 - TEST GET
   2 - TEST POST
   3 - TEST TIMEOUT (non in modo EMBEDDED)
   4 - TEST OVERLOAD
   5 - GET SINCRONO
   6 - GET SINCRONA ESTERNA REDIRECT
   7 - ASINCRONA SINCRONA con HEADERS (contemporanea)
   8 - ESTERNA con BUFFER EXT (setResponsePayload) stream
   9 - RISPOSTA CHUNK
   F - ATTIVAZIONE/DISATTIVAZIONE log su file SPIFFS
   V - VISUALIZZAZIONE file di log
   D - DELETE file di log su SPIFFS

🌐 Per inviare una richiesta GET esterna:
   Digita direttamente l'URL (es: https://script.google.com/macros...)
   H - questo help 
==============================================================
)rawliteral";
//==============
String battute[] = {
  "Se sei di buon umore, non ti preoccupare. Ti passerà.",
  "Ogni soluzione genera nuovi problemi.",
  "Se non t'importa dove sei, non ti sei perso.",
  "È incredibile quanto ci vuole a fare una cosa che non stai facendo tu.'",
  "Quando c'è bisogno di toccar ferro o legno, ci si accorge che il mondo è fatto di alluminio e plastica",
  "Sorridi... Domani sarà peggio."
};
int num_str = sizeof(battute) / sizeof(battute[0]) - 1;

static const char *htmlContent PROGMEM = R"(
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

static const size_t htmlContentLength = strlen_P(htmlContent);


//===================================================
// Funzione di invio chunk
size_t sendChunk(uint8_t *buffer, size_t maxLen, size_t index) {
  static size_t currentChunk = 0;

  //Serial.println(index);
  //Serial.println(currentChunk);
  //Serial.println(numChunks);
  if (index == 0) {
    currentChunk = 0;
  }

  if (currentChunk >= numChunks) {
    currentChunk = 0;  // reset per la prossima richiesta
    return 0;
  }

  const char *chunk = htmlChunks[currentChunk];
  //Serial.print('*');
  //Serial.print(chunk);
  size_t len = strlen(chunk);
  //Serial.println(len);
  len = (len > maxLen) ? maxLen : len;

  memcpy(buffer, chunk, len);
  //Serial.println(chunk); //
  currentChunk++;
  return len;
}


void handleChunked(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", sendChunk);
  request->send(response);
}
//===================================================

String escapeJson(String input) {
  input.replace("\\", "\\\\");
  input.replace("\"", "\\\"");
  input.replace("\n", "\\n");
  input.replace("\r", "\\r");
  input.replace("\t", "\\t");
  return input;
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset="UTF-8"><title>Async Test Server</title></head>
    <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
    h2 { color: #333; }
    #console {
      width: 95%;
      height: 400px;
      background-color: #000;
      color: #0F0;
      border: 1px solid #555;
      padding: 10px;
      overflow-y: scroll;
      font-family: 'Courier New', Courier, monospace;
      font-size: 0.9em;
      white-space: pre-wrap;
      word-wrap: break-word;
      margin-bottom: 15px;
    }
 .button {
  display: inline-block;
  padding: 10px 20px;
  background-color: #007BFF;
  color: white;
  text-decoration: none;
  border-radius: 5px;
  font-weight: bold;
  border: none;
  cursor: pointer;
}

.button:hover {
  background-color: #0056b3;
}
  
  </style>
    <body>
      <h2>🧪 Async Test Server</h2>
      <p><b>IP del server:</b> <span id="ipDisplay"></span></p>
      <p><b>Ultimo Metodo:</b> <span id="method"></span></p>
      <p><b>Ultimo Body:</b><pre id="body"></pre></p>
      <p><b>Ultimi Headers:</b><pre id="headers"></pre></p>
      <div id="console"></div>
      <button onclick="clearconsole()" class="button">CLEAR Console</button>
      <label>
      <input type="checkbox" name="Timestamp" id="Timestamp"  checked>
      Timestamp Enable
      </label>
      
      <script>
      var consoleDiv = document.getElementById('console');
      autoScrollEnabled = true; // Reset allo stato enabled alla riconnessione
      document.getElementById("ipDisplay").innerText = location.hostname;

      function clearconsole() {
        consoleDiv.innerHTML = "";
        return;
      }
  
  function update() {
  fetch("/data")
    .then(res => res.json())
    .then(json => {
      if (!json.method && !json.body && !json.headers) return; // Ignora se vuoto
      var checkBox = document.getElementById("Timestamp");
      
      if (checkBox.checked == true) {
        //console.log("timestamp enable.");

      const dt = new Date();
      const timestamp = dt.getFullYear() + "-" +
                    String(dt.getMonth() + 1).padStart(2, '0') + "-" +
                    String(dt.getDate()).padStart(2, '0') + " " +
                    String(dt.getHours()).padStart(2, '0') + ":" +
                    String(dt.getMinutes()).padStart(2, '0') + ":" +
                    String(dt.getSeconds()).padStart(2, '0');
      consoleDiv.innerHTML += "🕒 " + timestamp + "\n";
      }
      document.getElementById("method").innerText = json.method;
      document.getElementById("body").innerText = json.body;
      document.getElementById("headers").innerText = json.headers;
      consoleDiv.innerHTML += "method: " + json.method + "\n";
      consoleDiv.innerHTML += "body: " + json.body + "\n";
      consoleDiv.innerHTML += "headers: " + json.headers + "\n";
      if (autoScrollEnabled) {
        consoleDiv.scrollTop = consoleDiv.scrollHeight;
      }
    })
    .catch(err => console.warn("Fetch fallita o JSON non valido:", err));
 }
        setInterval(update, 1000);
        window.onload = update;
      </script>
    </body>
    </html>
  )rawliteral";

  //server.send(200, "text/html", html);
  request->send(200, "text/html", html);
}

void serverTask(void *parameter) {
  for (;;) {
    //server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(5));  // Cedi il controllo all'RTOS
  }
}

void handleDivertente(AsyncWebServerRequest *request) {
  //int i;


  request->send(200, "text/plain", "");
  //Serial.printf("stringa n: %d\r\n", i);
  String line = battute[random(0, num_str)];
  
  request->send(200, "text/plain", line);
}


void handleRedirect(AsyncWebServerRequest *request) {

  String host = request->host();  // es. "192.168.1.42" o "test.local"
  String location = "http://" + host + "/divertente";
  Serial.println(location);
  //http://127.0.0.1/redirect
}

void handleData(AsyncWebServerRequest *request) {
  String json = "{";
  if (newdati) {
    json += "\"method\":\"" + escapeJson(lastMethod) + "\",";
    json += "\"body\":\"" + escapeJson(lastBody) + "\",";
    json += "\"headers\":\"" + escapeJson(lastHeaders) + "\"";

    newdati = false;
    lastMethod = "";
    lastBody = "";
    lastHeaders = "";

  } else {
    json += "\"method\":\"\", \"body\":\"\", \"headers\":\"\"";
  }

  json += "}";
  request->send(200, "application/json", json);
}

void handleAny(AsyncWebServerRequest *request) {
  String path = request->url();

  if (path == "/favicon.ico" || path == "/data") {
    request->send(404, "text/plain", "Not found");
    return;
  }

  if (request->method() == HTTP_GET) {
    lastMethod = "GET";
  }

  if (request->method() == HTTP_POST) {
    lastMethod = "POST";
  }
  if (request->method() == HTTP_DELETE) {
    Serial.println("DELETE");
    lastMethod = "DELETE";
  }
  if (request->method() == HTTP_PUT) {
    lastMethod = "PUT";
  }

  //lastBody = "";
  lastHeaders = "";

  for (int i = 0; i < request->headers(); i++) {
    // Ottieni l'oggetto header
    const AsyncWebHeader *h = request->getHeader(i);

    // Aggiungi il nome e il valore dell'header alla stringa
    lastHeaders += h->name();
    lastHeaders += ": ";
    lastHeaders += h->value();
    lastHeaders += "\n";
  }

  newdati = true;
  Serial.println("SERVER TEST ===========");
  Serial.printf("TESTServer Metodo: %s\n", lastMethod);
  Serial.printf("TESTServer Body: %s\n", lastBody.c_str());
  Serial.printf("TESTServer Headers: %s", lastHeaders.c_str());
  Serial.println("SERVER TEST END ===========\n");

  request->send(200, "text/plain", "");
  //Serial.printf("stringa n: %d\r\n", i);
  String line = battute[random(0, num_str)];
  //Serial.println(line);

  request->send(200, "text/plain", line);

  //request->send(200, "text/plain", "OK Ricevuto!");
}

void handleSlow(AsyncWebServerRequest *request) {
  Serial.println("SLOW");
  //vTaskDelay(pdMS_TO_TICKS(15000));  // Ritardo di 10ms
  //
  //#include <esp_task_wdt.h>
  //esp_task_wdt_reset();  // Reset the watchdog timer to prevent it from triggering
  for (int i = 0; i < 4; i++) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 secondo
    //esp_task_wdt_reset();             // facoltativo, se il watchdog è attivo
  }
  //Serial.println("SLOW");

  //esp_task_wdt_reset();  // Reset the watchdog timer to prevent it from triggering
  //delay(15000);  // Simula lentezza di 10 secondi
  request->send(200, "text/plain", "Risposta lenta");
}

void handleIP(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", localIP);
}

void handleHTTPEvent(HTTPEventType type, const HTTPResponse *res) {
  switch (type) {
    case HTTPEventType::Response:
      if (res->statusCode == 200 || res->statusCode == 302) {
        Serial.print("\n✅");
      } else {
        Serial.print("\n❌");
      }
      Serial.printf("==== Risposta ricevuta:\nTitol: %s\n", res->inprogressTitle);
      Serial.printf("Titol: %s\n", res->inprogressTitle);
      Serial.printf("Codice HTTP: %d\n", res->statusCode);
      Serial.printf("Lenght: %d   Type: %s\n", res->contentLength, res->contentType);
      if (strlen(client.getResponsePayload()) > 0) Serial.println(client.getResponsePayload());
      if (strlen(res->msg_error) > 0 || res->statusCode != 200) {
        Serial.print("❌====ERRORE: ");
        Serial.println(res->msg_error);
        //.....registro
      }
      break;
    case HTTPEventType::Error:
      Serial.println("❌==== ERRORE:");
      Serial.print("Errore: ");
      Serial.println(res->msg_error);
      break;
    case HTTPEventType::Timeout:
      Serial.println("⏱ ==== TIMEOUT:");
      Serial.printf("Titol: %s\n", res->inprogressTitle);
      break;
    case HTTPEventType::Overload:
      Serial.println("⚠️ ===== Overload ignorata: ");
      Serial.println(res->msg_error);
      break;
    case HTTPEventType::Chunk:
      Serial.println("\n======== CHUNK:");
      Serial.printf("Chunk Lung: %d\nval: %s", res->expectedLength, res->ptr_workbuffer);
      break;
  }
  Serial.println("====== END =====");
}

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println("Connessione al WiFi...");
  delay(1000);
  startWIFI();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleRoot(request);  // chiami la funzione
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleData(request);  // chiami la funzione
  });

  server.on("/slow", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleSlow(request);  // chiami la funzione
  });

  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleIP(request);  // chiami la funzione
  });

  server.on("/divertente", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleDivertente(request);  // chiami la funzione
  });

  server.on(
    "/post", HTTP_POST, [](AsyncWebServerRequest *request) {
      //Serial.println("POST");
      handleAny(request);  // chiami la funzione
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // This is the onBody callback.
      // It is triggered for each chunk of data received.
      if (index == 0) {
        // This is the first chunk of the body.
        // We'll assume the entire body fits in one chunk for simplicity.
        String payload((char *)data, len);
        lastBody = payload;
        //Serial.print("Ricevuto Payload: ");
        //Serial.println(payload);
      }
    });


  server.on("/chunked", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleChunked(request);  // chiami la funzione
  });


  server.on("/chunked2", HTTP_GET, [](AsyncWebServerRequest *request) {
    String etag = String(htmlContentLength);

    if (request->header(asyncsrv::T_INM) == etag) {
      request->send(304);
      return;
    }

    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      Serial.printf("%u / %u\n", index, htmlContentLength);

      // finished ?
      if (htmlContentLength <= index) {
        Serial.println("finished");
        return 0;
      }

      // serve a maximum of 256 or maxLen bytes of the remaining content
      // this small number is specifically chosen to demonstrate the chunking
      // DO NOT USE SUCH SMALL NUMBER IN PRODUCTION
      // Reducing the chunk size will increase the response time, thus reducing the server's capacity in processing concurrent requests
      const int chunkSize = min((size_t)256, min(maxLen, htmlContentLength - index));
      Serial.printf("sending: %u\n", chunkSize);

      memcpy(buffer, htmlContent + index, chunkSize);

      return chunkSize;
    });

    response->addHeader(asyncsrv::T_Cache_Control, "public,max-age=60");
    response->addHeader(asyncsrv::T_ETag, etag);

    request->send(response);
  });

  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(302, "text/plain", "Redirecting...");
    String host = request->host();
    String location = "http://" + host + "/divertente";
    request->redirect(location);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    handleAny(request);
  });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("plain/text", 40 * 1024);
    for (int i = 0; i < 32 * 1024; i++) {
      response->write('a');
    }
    request->send(response);
  });

  server.on("/stream2", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("plain/text", 40 * 1024);
    for (int i = 0; i < htmlContentLength; i++) {
      response->write(htmlContent[i]);
    }
    request->send(response);
  });

  // server.on("/", handleRoot);
  // server.on("/data", handleData);
  // server.onNotFound(handleAny);
  // server.on("/slow", handleSlow);
  // server.on("/ip", handleIP);
  // server.on("/chunked", handleChunked);
  // server.on("/divertente", handleDivertente);
  // server.on("/redirect", handleRedirect);


  server.begin();
  Serial.println("Server WEB avviato");
  serverIP = WiFi.localIP().toString().c_str();
  //serverIP = String(WiFi.localIP());
  Serial.printf("Server IP: %s\n", serverIP);
  delay(2000);
  /// end webserver

  Serial.println(readhelp);
  Serial.flush();


if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    //print_terminal("LittleFS FAIL");
    LittleFS.format();
    if (LittleFS.begin()) FS_mounted = true;
  } else {
    FS_mounted = true;
    Serial.println("LittleFS Mounted OK");
    //print_terminal("LittleFS OK");
  }


  // Configurazione client
  //AsyncHTTPClientLight client;

  client.onEvent(handleHTTPEvent);  // onevent NO LAMBDA

  client.setDebug(true);
  //client.setLogToFile(true);  // Attiva se vuoi log su SPIFFS
  filedebug();
  client.setMaxRetries(2);
  client.setTimeout(10000);

  Serial.println(ipserver);
  // Creazione della task del server web
  xTaskCreate(
    serverTask,       // Funzione da eseguire come task
    "WebServerTask",  // Nome della task (per il debug)
    10000,            // Dimensione dello stack in byte
    NULL,             // Parametro da passare alla task
    1,                // Priorità della task (1 è bassa, 2 è alta)
    NULL);            // Handle della task

  // char prova[] = "http://127.0.0.1/chunked";
  // client.addTitle("PROVA");
  // client.beginRequest(prova, "GET");
  Serial.println(num_str);
}

void loop() {
  //server.handleClient();

  client.poll();
  delay(10);

  if (Serial.available() > 0) {
    String buffer_ser = Serial.readStringUntil('\n');
    docall(buffer_ser);
  }
}

void docall(String urlpath) {
  //https://tools-httpstatus.pickup-services.com/200?sleep=10000
  //http://192.168.1.21/slow

  Serial.printf("ESEGUO...%s\n", urlpath.c_str());
  free();
  String titolo;
  //String json = "{\"title\":\"CopiNet\",\"body\":\"Ciao Davide!\",\"userId\":1}";

  int inttipo = urlpath[0];
  if (urlpath[0] >= 48 && urlpath[0] <= 57) inttipo = atoi(urlpath.c_str());
  Serial.println(inttipo);

  reqcount++;
  String urlfinal = "http://" + localIP;

  switch (inttipo) {
    case ':':  // GET locale
      if (urlpath.length() < 8) break;
      localIP = &urlpath[1];
      Serial.print("Impostato nuovo Server:");
      Serial.println(localIP);
      break;
    case 1:  // GET locale
      titolo = "Richiesta GET locale n: " + String(reqcount);
      client.addTitle(titolo);
      urlfinal += "/events";
      client.beginRequest(urlfinal, "GET");
      break;
    case 2:  // POST locale
      titolo = "Richiesta POST locale n: " + String(reqcount);
      client.addTitle(titolo);
      urlfinal += "/post";
      client.addHeader("Content-Type", "application/json");
      client.beginRequest(urlfinal, "POST", json);
      break;
    case 3:  // timeout locale
      titolo = "Richiesta TIMEOUT locale n: " + String(reqcount);
      client.addTitle(titolo);
      urlfinal += "/slow";
      client.setTimeout(3000);
      client.beginRequest(urlfinal, "GET");
      //client.setTimeout(10000);
      break;
    case 4:  // overload locale
      titolo = "Richiesta GET locale n: " + String(reqcount);
      urlfinal += "/events";
      client.addTitle(titolo);
      client.beginRequest(urlfinal, "GET");
      reqcount++;
      titolo = "Richiesta OVERLOAD locale n: " + String(reqcount);
      client.addTitle(titolo);
      // non attendo la fine
      client.beginRequest(urlfinal, "GET");
      //client.runSync(urlfinal, "GET");
      break;
    case 5:  // SINCRONO locale
      urlfinal += "/events";
      dosincrona(urlfinal);
      break;
    case 6:  // SINCRONO locale
      client.setTimeout(10000);
      urlfinal = urlgoo;
      dosincrona(urlfinal);
      break;
    case 7:  // TEST per richiesta asincrona e subito sincrona
      urlfinal += "/events";
      client.addHeader("Custom-Header", "DavidePower");
      client.addHeader("Authorization", "Bearer xyz123");

      titolo = "ASINCRONA GET locale n: " + String(reqcount);
      client.addTitle(titolo);
      client.beginRequest(urlfinal, "GET");

      reqcount++;
      client.addHeader("Authorization", "ESP32S3 xyz123");
      client.addHeader("Custom-Header", "DavidePower");
      titolo = "SINCRONA GET locale n: " + String(reqcount);
      client.addTitle(titolo);
      dosincrona(urlfinal);
      break;
    case 8:  // sito meteo e risposta JSON
      client.addTitle("Risposta JSON SYNC STREAM");
      Serial.println(sizeof(jsonBuffer));
      client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
      //client.runSync(urlmeteo, "GET", "");
      client.beginRequest(urlmeteo, "GET", " ");
      Serial.println(jsonBuffer);
      break;
    case 9:  // TEST per richiesta asincrona e subito sincrona
      urlfinal += "/chunked";
      //String newheader = "DavidePower " + String(reqcount);
      titolo = "CHUNKED GET locale n: " + String(reqcount);
      client.addTitle(titolo);
      client.beginRequest(urlfinal, "GET");
      break;
    case 'J':  // TEST per DELETErichiesta asincrona e subito sincrona
      urlfinal = "https://jsonplaceholder.typicode.com/posts/1";
      //String newheader = "DavidePower " + String(reqcount);
      titolo = "DELETE jsonplaceholder n: " + String(reqcount);
      client.addTitle(titolo);
      client.beginRequest(urlfinal, "DELETE");
      break;
//https:         //jsonplaceholder.typicode.com/comments?postId=1
    case 'C':  // TEST per richiesta asincrona e subito sincrona
      urlfinal += "/chunked2";
      //String newheader = "DavidePower " + String(reqcount);
      client.setResponsePayload(jsonBuffer, sizeof(jsonBuffer));
      titolo = "CHUNKED LONG GET locale n: " + String(reqcount);
      client.addTitle(titolo);

      client.beginRequest(urlfinal, "GET");
      break;
    case 'M':  // print memoria libera
      free();
      break;
    case 'N':
      break;

    case 'S':  // Prova stress
      urlfinal += "/events";
      provastress(urlfinal);
      break;
    case 'F':  // debug su file
      filedebug();
      break;
    case 'V':  // view log
      readFileLog("/http_log.txt");
      break;
    case 'D':  // debug su file
      deletelog();
      break;
    case 'H':  // debug su file
      Serial.println(readhelp);
      break;
    default:  // prova esterna libera
      if (urlpath[0] != 'h') break;
      titolo = "Richiesta MANUALE n: " + String(reqcount);
      client.addTitle(titolo);
      client.beginRequest(urlpath, "GET", "");
      break;
  }
}

void free() {
  Serial.printf("Memoria libera:%d (%d float)\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) / sizeof(float));

  Serial.print("Ram size: ");
  Serial.println(formatBytes(ESP.getHeapSize()));
  Serial.print("Free ram: ");
  Serial.println(formatBytes(ESP.getFreeHeap()));
  Serial.print("Max alloc ram: ");
  Serial.println(formatBytes(ESP.getMaxAllocHeap()));
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
  }
}


void provastress(String urlfinal) {
  client.setDebug(false);
  Serial.println("PROVA STRESS");
  free();
  delay(3000);
  reqcount = 0;

  while (reqcount < 100) {

    while (!client.isFinished()) {
      vTaskDelay(pdMS_TO_TICKS(20));
      client.poll();
    }

    free();
    reqcount++;
    String titolo = "Richiesta GET locale n: " + String(reqcount);
    Serial.println(titolo);
    client.addTitle(titolo);
    Serial.printf("STRESS n: %d\n", reqcount);
    //client.beginRequest(urlfinal, "GET");
    client.runSync(urlfinal);
  }
  client.setDebug(true);
  Serial.println("END STRESS");
}
void dosincrona(String urlfinal) {
  int codres = 0;
  String titolo = "Richiesta SINCRONA locale n: " + String(reqcount);
  client.addTitle(titolo);
  codres = client.runSync(urlfinal, "GET", "");
  //codres = client.runSync( "http://127.0.0.1/events", "GET", "");

  //Serial.printf("END SINCRONA = %d\n", codres);

  //const char *payload = client.getResponsePayload();  // restituisce responsePayloadBuffer
  //Serial.println(payload);
}

void dosasynsync(String urlfinal) {
  int codres = 0;
  String titolo = "Ric. ASINCRONA locale n: " + String(reqcount);
  client.addTitle(titolo);
  //Serial.println(urlfinal);
  client.beginRequest(urlfinal, "GET", "");
  reqcount++;
  //faccio sincrona
  titolo = "Ric. SINCRONA locale n: " + String(reqcount);
  client.addTitle(titolo);
  codres = client.runSync(urlfinal, "GET", "");

  Serial.printf("END ASINCRONA e SINCRONA = %d\n", codres);
}

void filedebug() {
#ifdef MY_FS
  debugstato = !debugstato;
  client.setLogToFile(debugstato);
  Serial.printf("DEBUG su file %s\n", (debugstato) ? "ATTIVATO" : "DISATTIVATO");
#endif
}
void deletelog() {
#ifdef MY_FS
  MY_FS.remove("/http_log.txt");
  Serial.println("File di LOG deleted..");
#endif
}

//-----------------------------------
void startWIFI() {
  Serial.println("\nSetting Station configuration ... ");
  WiFi.begin(ssid, password);
  Serial.println(String("Connecting to ") + ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup End");
}
//----------------------------
void readFileLog(char *nomefile) {
#ifndef MY_FS
  Serial.println("MY_FS non abilitato....");
#endif
#ifdef MY_FS
  Serial.println("Leggo File LOG:");
  //"/http_log.txt"
  File file = MY_FS.open(nomefile, "r");
  if (!file) {
    Serial.println("Errore nell'apertura del file");
    return;
  }

  Serial.println("============= START  file:");
  while (file.available()) {
    Serial.println(file.readStringUntil('\n'));
  }

  Serial.println("============= END  file:");
  file.close();
#endif
}
//-----------------------------

