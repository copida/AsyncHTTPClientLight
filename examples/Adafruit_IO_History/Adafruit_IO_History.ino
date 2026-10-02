/*
  Adafruit_IO_History
  Scarica storico di 1000 record da Adafruit IO.
  
  Caratteristiche:
  - Buffer PSRAM da 50KB (setResponsePayload)
  - Header di autenticazione (X-AIO-Key)
  - Parse JSON con ArduinoJson v7
  - runSync() per aspettare la risposta completa
  
  Setup:
  1. Definisci AIO_USERNAME e IO_KEY (o usali da secrets.h)
  2. Installa ArduinoJson v7 da Library Manager
  3. Abilita PSRAM: Sketch → Properties → PSRAM: "OPI PSRAM"
  4. Cambia WIFI_SSID e WIFI_PASS
  
  Nota: questo è un caso di uso reale di AsyncHTTPClientLight
  su API che rispondono con payload di decine di KB.
*/

#include <WiFi.h>
#include "AsyncHTTPClientLight.h"
#include <ArduinoJson.h>

// ===== CONFIGURAZIONE =====
const char* WIFI_SSID = "TUO_SSID";
const char* WIFI_PASS = "TUA_PASSWORD";
const char* AIO_USERNAME = "tuo_username";
const char* IO_KEY = "tua_api_key";
const char* FEED_NAME = "monitor1.temperatura";

// ===== GLOBALI =====
AsyncHTTPClientLight http;
char* psramBuffer = nullptr;
const size_t BUFFER_SIZE = 50000;

// ============================================
// Helper: alloca buffer in PSRAM (ESP32)
// ============================================
char* allocaBufferPSRAM(size_t size) {
  char* buf = (char*)ps_malloc(size);
  if (buf) {
    memset(buf, 0, size);
  }
  return buf;
}

// ============================================
// Callback per eventi HTTP
// ============================================
void onHttpEvent(HTTPEventType type, const HTTPResponse* res) {
  switch (type) {
    case HTTPEventType::Response:
      Serial.printf("✅ HTTP %d | %lu bytes | %lu ms\n",
                    res->statusCode, 
                    (unsigned long)res->contentLength,
                    (unsigned long)res->restime);
      break;
    case HTTPEventType::Error:
    case HTTPEventType::Timeout:
      Serial.printf("❌ %s\n", res->msg_error);
      break;
    default:
      break;
  }
}

// ============================================
// Download storico da Adafruit IO
// ============================================
void scaricaStoricoAdafruitIO() {
  Serial.println("\n====== ADAFRUIT IO HISTORY DOWNLOAD ======\n");

  // Controlla WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi non connesso");
    return;
  }

  // Alloca buffer PSRAM
  psramBuffer = allocaBufferPSRAM(BUFFER_SIZE);
  if (!psramBuffer) {
    Serial.println("❌ Errore allocazione PSRAM");
    return;
  }
  Serial.printf("✅ Buffer PSRAM: %u bytes\n", BUFFER_SIZE);

  // Costruisci URL (Adafruit API v2)
  char url[256];
  snprintf(url, sizeof(url),
           "https://io.adafruit.com/api/v2/%s/feeds/%s/data?limit=1000",
           AIO_USERNAME, FEED_NAME);
  Serial.printf("URL: %s\n\n", url);

  // Configura HTTP client
  http.setResponsePayload(psramBuffer, BUFFER_SIZE);
  http.setTimeout(15000);           // Adafruit puo' essere lento
  http.setMaxRetries(2);
  http.setDebug(true);
  http.onEvent(onHttpEvent);

  // Header di autenticazione
  http.addHeader("X-AIO-Key", IO_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addTitle("📥 Adafruit IO History");

  // Richiesta sincrona (aspetta risposta)
  int httpCode = http.runSync(url, "GET");

  if (httpCode != 200) {
    Serial.printf("❌ HTTP error: %d\n", httpCode);
    free(psramBuffer);
    psramBuffer = nullptr;
    return;
  }

  Serial.println("\n📄 Parsing JSON...\n");

  // ===== Parse JSON (ArduinoJson v7) =====
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, psramBuffer);

  if (error) {
    Serial.printf("❌ JSON parse error: %s\n", error.c_str());
    free(psramBuffer);
    psramBuffer = nullptr;
    return;
  }

  // Adafruit IO ritorna un array di record
  JsonArray array = doc.as<JsonArray>();
  if (!array) {
    Serial.println("❌ JSON non è un array valido");
    free(psramBuffer);
    psramBuffer = nullptr;
    return;
  }

  // ===== Elabora record =====
  Serial.println("ID | Valore | Created_At");
  Serial.println("----------------------------------------");

  int countRecord = 0;
  for (JsonObject record : array) {
    const char* id = record["id"] | "?";
    const char* value = record["value"] | "?";
    const char* created_at = record["created_at"] | "?";

    // Esempio: filtra i record che contengono "Online"
    if (strstr(value, "Online") == NULL) {
      countRecord++;
      Serial.printf("%s | %s | %s\n", id, value, created_at);
    }
  }

  Serial.println("----------------------------------------");
  Serial.printf("\n✅ Record processati: %d\n", countRecord);
  Serial.printf("📊 PSRAM libera: %u bytes\n", ESP.getFreePsram());

  // Libera memoria
  free(psramBuffer);
  psramBuffer = nullptr;
}

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== AsyncHTTPClientLight + Adafruit IO ===\n");

  // Connetti WiFi
  Serial.print("Connessione WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connesso!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Connessione WiFi fallita");
    return;
  }

  // Scarica storico
  scaricaStoricoAdafruitIO();
}

// ============================================
// Loop
// ============================================
void loop() {
  delay(30000);  // Aspetta 30s tra i refresh
  
  // Decommenta per download periodico:
  // scaricaStoricoAdafruitIO();
}
