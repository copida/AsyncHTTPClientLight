#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "Vodafone-C01030164";
const char *password = "f4qHtbX2N32qAgad";

WebServer server(80);

String lastMethod = "";
String lastBody = "";
String lastHeaders = "";

String escapeJson(String input) {
  input.replace("\\", "\\\\");
  input.replace("\"", "\\\"");
  input.replace("\n", "\\n");
  input.replace("\r", "\\r");
  input.replace("\t", "\\t");
  return input;
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset="UTF-8"><title>Async Test Server</title></head>
    <body>
      <h2>🧪 Async Test Server</h2>
      <p><b>Ultimo Metodo:</b> <span id="method"></span></p>
      <p><b>Ultimo Body:</b><pre id="body"></pre></p>
      <p><b>Ultimi Headers:</b><pre id="headers"></pre></p>

      <script>
        function update() {
          fetch("/data")
            .then(res => res.json())
            .then(json => {
              document.getElementById("method").innerText = json.method;
              document.getElementById("body").innerText = json.body;
              document.getElementById("headers").innerText = json.headers;
            })
            .catch(err => console.error("Errore AJAX:", err));
        }
        setInterval(update, 2000);
        window.onload = update;
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"method\":\"" + escapeJson(lastMethod) + "\",";
  json += "\"body\":\"" + escapeJson(lastBody) + "\",";
  json += "\"headers\":\"" + escapeJson(lastHeaders) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleAny() {
  lastMethod = server.method() == HTTP_GET ? "GET" : "POST";
  lastBody = server.arg("plain");
  lastHeaders = "";

  for (int i = 0; i < server.headers(); i++) {
    lastHeaders += server.headerName(i) + ": " + server.header(i) + "\n";
  }

  server.send(200, "text/plain", "Ricevuto!");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connessione a WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnesso! IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handleAny);

  server.begin();
  Serial.println("Server avviato");
}

void loop() {
  server.handleClient();
}
