#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

const char* ssid = "TUO_SSID";
const char* password = "TUA_PASSWORD";

AsyncHTTPClientLight client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connesso");

  client.setDebug(true); // Attiva il debug (solo se ASYNC_HTTP_DEBUG è definito)

  client.onRequestComplete([](int code, String body) {
    Serial.println("Risposta POST ricevuta:");
    Serial.println("Codice: " + String(code));
    Serial.println("Body:\n" + body);
  });

  client.onError([](String err) {
    Serial.println("Errore: " + err);
  });

  String json = "{\"title\":\"CopiNet\",\"body\":\"Ciao Davide!\",\"userId\":1}";

  client.beginRequest("https://jsonplaceholder.typicode.com/posts", "POST", json);
  client.addHeader("Content-Type", "application/json");
}

void loop() {
  client.poll();
}
