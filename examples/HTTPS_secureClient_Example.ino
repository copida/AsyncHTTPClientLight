#include <ESP8266WiFi.h>           // o <WiFi.h> per ESP32
#include <WiFiClientSecure.h>
#include "AsyncHttpClient.h"       // tua libreria custom

const char* ssid = "TUO_SSID";
const char* password = "TUA_PASSWORD";

WiFiClientSecure secureClient;
AsyncHttpClient client(secureClient);

int currentRequest = 0;
bool waitingForFinish = false;
unsigned long waitStart = 0;
const unsigned long MAX_WAIT = 5000;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  secureClient.setInsecure(); // disabilita verifica certificato

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connesso a WiFi");
}

void loop() {
  client.poll();

  if (waitingForFinish) {
    if (client.isFinished()) {
      Serial.println("✅ Richiesta #" + String(currentRequest) + " completata");
      waitingForFinish = false;
      currentRequest++;
    } else if (millis() - waitStart > MAX_WAIT) {
      Serial.println("⚠️ Timeout su richiesta #" + String(currentRequest));
      waitingForFinish = false;
      currentRequest++;
    }
  }

  if (!waitingForFinish && currentRequest < 10) {
    String method = (currentRequest % 2 == 0) ? "GET" : "POST";
    String url;
    String payload = "";

    if (method == "GET") {
      url = "https://jsonplaceholder.typicode.com/posts/" + String(currentRequest + 1);
    } else {
      url = "https://jsonplaceholder.typicode.com/posts";
      payload = "{\"title\":\"test\",\"body\":\"ciao\",\"userId\":1}";
    }

    client.addTitle("🔁 Richiesta #" + String(currentRequest) + " - " + method);
    client.beginRequest(url, method, payload);

    waitingForFinish = true;
    waitStart = millis();
  }
}
