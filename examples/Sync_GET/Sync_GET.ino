/*
  Sync_GET
  Dimostra la modalità sincrona (runSync).
  
  runSync() BLOCCA il loop finché la richiesta non finisce e ritorna
  il codice HTTP (200, 404, ecc).
  Se c'è una richiesta asincrona in corso, la porta a termine prima.
  
  Usi:
  - Sketch semplici dove non serve asincronia
  - Setup iniziale (configurazione, download parametri)
  - Parti critiche dove hai bisogno della risposta prima di continuare
  
  Svantaggio: il loop() è bloccato durante la richiesta,
  quindi non puoi controllare bottoni, sensori, ecc.
  
  Per applicazioni real-time, usa beginRequest() + poll() + callback (vedi altri esempi).
*/

#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

// ===== CONFIGURAZIONE =====
const char* WIFI_SSID = "TUO_SSID";
const char* WIFI_PASS = "TUA_PASSWORD";

// ===== GLOBALI =====
AsyncHTTPClientLight http;
char responseBuf[2048];  // Buffer per risposta

// ============================================
// Funzione helper: GET sincrono
// ============================================
bool getSync(const char* url, String& outResponse) {
  Serial.printf("\n📡 GET %s\n", url);

  http.setResponsePayload(responseBuf, sizeof(responseBuf));
  http.setTimeout(8000);
  http.setDebug(false);  // Disabilita per meno rumore
  http.addTitle("Sync GET");

  int httpCode = http.runSync(url, "GET");

  if (httpCode == 200) {
    outResponse = String(http.getResponsePayload());
    Serial.printf("✅ HTTP %d | %d bytes\n\n", httpCode, outResponse.length());
    return true;
  } else {
    Serial.printf("❌ HTTP %d\n\n", httpCode);
    return false;
  }
}

// ============================================
// Funzione helper: POST sincrono
// ============================================
bool postSync(const char* url, const char* payload, String& outResponse) {
  Serial.printf("\n📤 POST %s\n", url);
  Serial.printf("   Payload: %s\n", payload);

  http.setResponsePayload(responseBuf, sizeof(responseBuf));
  http.setTimeout(10000);
  http.setDebug(false);
  http.addHeader("Content-Type", "application/json");
  http.addTitle("Sync POST");

  int httpCode = http.runSync(url, "POST", payload);

  if (httpCode == 200 || httpCode == 201) {
    outResponse = String(http.getResponsePayload());
    Serial.printf("✅ HTTP %d | %d bytes\n\n", httpCode, outResponse.length());
    return true;
  } else {
    Serial.printf("❌ HTTP %d\n\n", httpCode);
    return false;
  }
}

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== AsyncHTTPClientLight - Sync Mode ===\n");

  // Connetti WiFi
  Serial.print("Connessione WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ WiFi fallito");
    return;
  }

  Serial.println("\n✅ WiFi OK\n");

  // ===== ESEMPIO 1: GET semplice =====
  Serial.println("--- ESEMPIO 1: GET semplice ---");
  String response1;
  if (getSync("http://httpbin.org/get", response1)) {
    Serial.println(response1);
  }

  // ===== ESEMPIO 2: POST JSON =====
  Serial.println("\n--- ESEMPIO 2: POST JSON ---");
  String response2;
  const char* jsonData = "{\"name\":\"ESP32\",\"action\":\"test\"}";
  if (postSync("http://httpbin.org/post", jsonData, response2)) {
    Serial.println(response2);
  }

  // ===== ESEMPIO 3: HTTPS GET =====
  Serial.println("\n--- ESEMPIO 3: HTTPS GET ---");
  String response3;
  if (getSync("https://httpbin.org/json", response3)) {
    // Puoi fare parsing JSON qui se serve
    Serial.println(response3.substring(0, 200));  // Mostra primi 200 char
    Serial.println("...");
  }

  Serial.println("\n=== Setup completato ===\n");
}

// ============================================
// Loop
// ============================================
void loop() {
  // Rimani qui: aggiorna ogni 30 secondi se vuoi
  delay(30000);

  if (WiFi.status() == WL_CONNECTED) {
    // Esempio: GET periodico
    // String resp;
    // getSync("http://httpbin.org/get", resp);
  }
}
