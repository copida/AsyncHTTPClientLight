#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

const char* ssid = "TUO_SSID";
const char* password = "TUA_PASSWORD";

AsyncHTTPClientLight client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connessione WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connesso!");
  Serial.println(WiFi.localIP());

  // Configurazione client
  client.setDebug(true);
  client.setLogToFile(true);
  client.setMaxRetries(3);
  client.setTimeout(8000);

  // Callback unificato
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


  // Header personalizzati
  client.addHeader("Authorization", "Bearer xyz123");
  client.addHeader("Custom-Header", "DavidePower");

  // Titolo per logging
  client.addTitle("Richiesta GET iniziale");

  // Avvio richiesta asincrona
  client.beginRequest("http://httpbin.org/get", "GET", "");
}

void loop() {
  client.poll();
  if (client.isFinished()) {
    Serial.println("Risposta ricevuta:");
    Serial.println(client.getLastResponseBody());
  }
}
