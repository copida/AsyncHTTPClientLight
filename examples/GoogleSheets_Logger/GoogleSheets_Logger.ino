/*
  GoogleSheets_Logger
  Invia una riga a un foglio Google tramite Web App di Apps Script.
  
  Caratteristiche:
  - POST JSON ogni N secondi
  - Gestisce redirect 302 (Apps Script ne invia sempre almeno uno)
  - Callback unificata per tutti gli eventi HTTP
  - Logging su Serial
  
  Setup:
  1. Crea uno script Apps Script in Google Sheets:
     function doPost(e) {
       var data = JSON.parse(e.postData.contents);
       var sheet = SpreadsheetApp.getActiveSheet();
       sheet.appendRow([new Date(), data.temp, data.humidity]);
       return ContentService.createTextOutput("OK");
     }
  2. Pubblica come "Web app" (Execute as: tuo account, Anyone can access)
  3. Copia lo script URL
  4. Definisci qui SHEET_URL, WIFI_SSID, WIFI_PASS
  
  Nota: Google Apps Script risponde sempre con un 302 redirect
  verso script.googleusercontent.com — la libreria lo segue automaticamente.
*/

#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

// ===== CONFIGURAZIONE =====
const char* WIFI_SSID = "TUO_SSID";
const char* WIFI_PASS = "TUA_PASSWORD";

// Copia questo URL dalla pubblicazione di Apps Script
const char* SHEET_URL = "https://script.google.com/macros/s/XXXXXXXXXXXXXXX/exec";

// Intervallo di invio (millisecondi)
const unsigned long SEND_INTERVAL = 60000;  // 1 minuto

// ===== GLOBALI =====
AsyncHTTPClientLight http;
unsigned long lastSend = 0;
char responseBuf[512];  // Buffer per risposta

// ============================================
// Callback per eventi HTTP
// ============================================
void onHttpEvent(HTTPEventType type, const HTTPResponse* res) {
  switch (type) {
    case HTTPEventType::Response:
      if (res->statusCode == 200) {
        Serial.println("✅ Riga salvata su Google Sheet");
        // Leggi risposta (valida solo dentro la callback)
        const char* payload = http.getResponsePayload();
        if (payload && strlen(payload) > 0) {
          Serial.printf("   Risposta: %s\n", payload);
        }
      } else {
        Serial.printf("❌ HTTP %d (%lu ms)\n", 
                      res->statusCode, (unsigned long)res->restime);
      }
      break;

    case HTTPEventType::Timeout:
      Serial.printf("⏱️ Timeout: %s (retry automatico)\n", res->msg_error);
      break;

    case HTTPEventType::Error:
      Serial.printf("❌ Errore: %s\n", res->msg_error);
      break;

    case HTTPEventType::Overload:
      Serial.println("⚠️ Richiesta precedente ancora in corso, salto questo giro");
      break;

    default:
      break;
  }
}

// ============================================
// Invia lettura sensore (POST JSON)
// ============================================
void sendReading(float temp, float humidity) {
  // Prepara payload JSON
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%.1f,\"humidity\":%.0f,\"timestamp\":%lu}",
           temp, humidity, millis());

  // Header
  http.addHeader("Content-Type", "application/json");
  http.addTitle("📊 Telemetry to Sheet");

  // Invia asincrono (non blocca loop)
  http.beginRequest(SHEET_URL, "POST", payload);
  Serial.println("📤 Richiesta inviata...");
}

// ============================================
// Leggi sensore (placeholder)
// ============================================
void readSensors(float& outTemp, float& outHumidity) {
  // SOSTITUISCI CON I TUOI SENSORI
  // Questo è solo un esempio con valori casuali
  outTemp = 20.0 + (rand() % 10);  // 20-30°C
  outHumidity = 40 + (rand() % 40); // 40-80%
}

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== AsyncHTTPClientLight + Google Sheets ===\n");

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
    Serial.println("\n❌ WiFi fallito");
    return;
  }

  // Configura HTTP client
  http.setResponsePayload(responseBuf, sizeof(responseBuf));
  http.setTimeout(10000);      // Google puo' essere lento
  http.setMaxRetries(2);
  http.setmaxRedirects(3);     // Apps Script = almeno 1 redirect
  http.setDebug(true);         // Mostra log dettagliato
  http.onEvent(onHttpEvent);

  Serial.println("\n🟢 Sistema pronto. Invio in corso...\n");
  lastSend = 0;  // Forza il primo invio subito
}

// ============================================
// Loop
// ============================================
void loop() {
  // Chiama poll() SEMPRE, a ogni giro
  http.poll();

  // Controlla WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi perso, riconnetto...");
    WiFi.reconnect();
    delay(1000);
    return;
  }

  // Ogni N secondi, invia lettura
  if (http.isFinished() && millis() - lastSend >= SEND_INTERVAL) {
    float temp, humidity;
    readSensors(temp, humidity);
    
    Serial.printf("\n[%lu] Lettura: %.1f°C, %.0f%%\n", 
                  millis(), temp, humidity);
    
    sendReading(temp, humidity);
    lastSend = millis();
  }

  delay(100);  // Non consumare troppo CPU
}
