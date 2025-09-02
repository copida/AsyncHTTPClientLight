#include "AsyncHTTPClientLight.h"
#define ASYNC_HTTP_DEBUG
#define ASYNC_HTTP_LOG_SPIFFS
//#define ASYNC_HTTP_LOG_SD


#ifdef ASYNC_HTTP_DEBUG
#pragma message "### AsyncHTTPClientLight: Attivato DEBUG ###"
#endif

#ifdef ASYNC_HTTP_LOG_SPIFFS
#include <SPIFFS.h>
#pragma message "### AsyncHTTPClientLight: Funzionalità 'SPIFFS' inclusa nella compilazione. ###"
#endif

#ifdef ASYNC_HTTP_LOG_SD
#include <SD.h>
#pragma message "### AsyncHTTPClientLight: Funzionalità 'SD' inclusa nella compilazione. ###"
#endif

AsyncHTTPClientLight::AsyncHTTPClientLight() {
  reset();
  timeoutMs = 10000;
  debugEnabled = false;
  requestCounter = 0;
  maxRetries = 1;
  retryCount = 1;
}

void AsyncHTTPClientLight::triggerEvent(HTTPEventType type, const String& message) {
  if (unifiedCallback) unifiedCallback(type, message);
}

void AsyncHTTPClientLight::setMaxRetries(int retries) {
  if (retries >= 1) maxRetries = retries;
}

void AsyncHTTPClientLight::onRetry(std::function<void(int)> cb) {
  retryCallback = cb;
}

void AsyncHTTPClientLight::setDebug(bool enabled) {
  debugEnabled = enabled;
}

void AsyncHTTPClientLight::setLogToFile(bool enabled) {
  logToFile = enabled;

#if defined(ASYNC_HTTP_LOG_SPIFFS)
  if (enabled && !SPIFFS.begin(true)) {
    log("SPIFFS non inizializzato");
  }
#elif defined(ASYNC_HTTP_LOG_SD)
  if (enabled && !SD.begin()) {
    log("SD non inizializzata");
  }
#endif
}

int AsyncHTTPClientLight::getLastHTTPcode() const {
  return statusCode;
}

String AsyncHTTPClientLight::getLastResponseBody() const {
  return responseBody;
}

String AsyncHTTPClientLight::getLastTitle() const {
  return inprogressTitle;
}

void AsyncHTTPClientLight::log(const String& msg) {
#ifdef ASYNC_HTTP_DEBUG
  String fullMsg = logPrefix + msg;

  if (debugEnabled) Serial.println(fullMsg);

  if (logToFile) {
#if defined(ASYNC_HTTP_LOG_SPIFFS)
    File f = SPIFFS.open("/http_log.txt", FILE_APPEND);
#elif defined(ASYNC_HTTP_LOG_SD)
    File f = SD.open("/http_log.txt", FILE_APPEND);
#endif

    if (f) {
      f.println(fullMsg);
      f.close();
    }
  }
#endif
}

void AsyncHTTPClientLight::reset() {
  client = nullptr;
  host = "";
  path = "/";
  port = 80;
  useSSL = false;
  method = "GET";
  payload = "";
  //headers.clear();
  finished = true;
  chunkedEncoding = false;
  headersParsed = false;
  statusCode = -1;
  responseBody = "";
  state = IDLE;
  redirectCount = 0;
}

bool AsyncHTTPClientLight::parseURL(const String& url) {
  useSSL = url.startsWith("https://");
  int index = url.indexOf("://");
  if (index == -1) return false;

  int start = index + 3;
  int pathIndex = url.indexOf("/", start);
  if (pathIndex == -1) {
    host = url.substring(start);
    path = "/";
  } else {
    host = url.substring(start, pathIndex);
    path = url.substring(pathIndex);
  }

  port = useSSL ? 443 : 80;

  log("Protocollo: " + String(useSSL ? "HTTPS" : "HTTP"));
  log("Host: " + host);
  log("Porta: " + String(port));
  log("Path: " + path);

  return true;
}

void AsyncHTTPClientLight::addTitle(const String& title) {
  pendingTitle = title;
}

int AsyncHTTPClientLight::runSync(const String& url, const String& method, const String& payload) {

// aspetto se eventualmente c'è una richiesta asincrona in corso
// la porto a termine
if(!finished){
	Serial.printf("Wait end Asincrona : %s\n", inprogressTitle.c_str());
	Serial.printf("Pending Sincrona : %s\n", pendingTitle.c_str());
}


while(!finished){
   vTaskDelay(pdMS_TO_TICKS(10));
   poll();
  }

  _isSyncMode = true;
  unsigned long startTime;
  startTime = millis();

  beginRequest(url, method, payload);

  while (!isFinished()) {
	vTaskDelay(pdMS_TO_TICKS(10));
    //delay(100);
    poll();
  }

  // clean
  client->stop();
  _isSyncMode = false;
  finished = true;
  log("SYNC CLOSED");
  return statusCode;
}


void AsyncHTTPClientLight::beginRequest(const String& url, const String& method_, const String& payload_) {
  if (!isFinished() && redirectCount == 0 && retryCount == 1) {
    log("Overload: richiesta già in corso: " + inprogressTitle);
    triggerEvent(HTTPEventType::Overload, pendingTitle);
    return;
  }

  reset();
  requestCounter++;
  logPrefix = "[REQ " + String(requestCounter) + "] ";


  if (finished && retryCount == 1) {	//nuova richiesta
    inprogressTitle = pendingTitle.length() ? pendingTitle : "(nessun titolo)";
    log(" === " + inprogressTitle + " ===");
    pendingTitle = "(nessun titolo)";
	inprogressHeaders = pendingHeaders;
    pendingHeaders.clear();
  }

  log("Tentativo n:" + String(retryCount));

  if (!parseURL(url)) {
    finished = true;
    log(logPrefix + "ERRORE URL non valido");
    triggerEvent(HTTPEventType::Error, logPrefix + "URL non valido");
    return;
  }

  method = method_;
  payload = payload_;

  state = CONNECTING;

  lastActivity = millis();
  finished = false;
}


void AsyncHTTPClientLight::addHeader(const String& key, const String& value) {
  pendingHeaders.push_back({ key, value });;
}

void AsyncHTTPClientLight::setTimeout(unsigned long ms) {
  timeoutMs = ms;
}

void AsyncHTTPClientLight::onEvent(UnifiedCallback cb) {
  unifiedCallback = cb;
}

bool AsyncHTTPClientLight::isFinished() {
  return finished;
}

void AsyncHTTPClientLight::poll() {
  unsigned long startTime;
  //  if (finished || !client) return;
  if (finished) return;

  //if (!_isSyncMode) checktimeout();
  checktimeout();
  switch (state) {
    case CONNECTING:
      connecting();
      break;
    case SENDING:
      sending();
      break;
    case RECEIVING:
      //startTime = millis();
      //while (!client->available() && millis() - startTime < timeoutMs) {
        //vTaskDelay(pdMS_TO_TICKS(10));  // Ritardo di 10ms
		//delay(10);
      //}
	  if (!_isSyncMode) vTaskDelay(pdMS_TO_TICKS(10)); 
      if (client->available()) receiving();
      break;
    default:
      break;
  }
}

void AsyncHTTPClientLight::checktimeout() {
  if (millis() - lastActivity > timeoutMs) {
    retryCount++;
    if (retryCount > maxRetries) {
      state = IDLE;
      client->stop();
      finished = true;
      retryCount = 1;
      log("Esauriti tentativi (Timeout)");
      triggerEvent(HTTPEventType::Timeout, logPrefix + "Timeout");
      //      finished = true;
      return;
    }
    log("Timeout Retry#" + String(retryCount) + " in corso...");
    if (retryCallback) retryCallback(retryCount);
    beginRequest((useSSL ? "https://" : "http://") + host + path, method, payload);
    return;
  }
}

void AsyncHTTPClientLight::connecting() {
  client = useSSL ? &secureClient : &plainClient;
  if (useSSL) secureClient.setInsecure();
//  log("Client creato puntatore: " + String((uintptr_t)client));

  if (client->connect(host.c_str(), port)) {
    state = SENDING;
    lastActivity = millis();
    log("Connessione riuscita");
    return;
  } else {
    log("Connessione fallita");
    retryCount++;
    if (retryCount > maxRetries) {
      retryCount = 1;
      log("Esauriti tentativi RiConnessione");
      triggerEvent(HTTPEventType::Error, logPrefix + "Connessione fallita");
      finished = true;
      return;
    }

    log("Retry connecting#" + String(retryCount));
    if (retryCallback) retryCallback(retryCount);
    beginRequest((useSSL ? "https://" : "http://") + host + path, method, payload);
    return;
  }
}

void AsyncHTTPClientLight::sending() {
	//Serial.println("=== HEADER LIST ===");
//for (auto& h : inprogressHeaders) {
  //  Serial.println(h.first + ": " + h.second);
//}
//Serial.println("===================");

  String request = method + " " + path + " HTTP/1.1\r\n";
  request += "Host: " + host + "\r\n";
  for (auto& h : inprogressHeaders) {
    request += h.first + ": " + h.second + "\r\n";
  }
  
  if (payload.length()) {
    request += "Content-Length: " + String(payload.length()) + "\r\n";
    request += "Content-Type: application/json\r\n";
  }
   
  request += "Connection: close\r\n\r\n";
  if (payload.length()) request += payload;
  client->print(request);
  state = RECEIVING;
  lastActivity = millis();
  //client->flush();
}

void AsyncHTTPClientLight::receiving() {
  while (client->available()) {
    String line = client->readStringUntil('\n');
    line.trim();
    lastActivity = millis();

    if (!headersParsed) {
      log("Header: " + line);
      if (line.startsWith("HTTP/")) {
        int space = line.indexOf(' ');
        statusCode = line.substring(space + 1).toInt();
        log("Status code: " + String(statusCode));
      } else if (line.startsWith("Transfer-Encoding:") && line.indexOf("chunked") != -1) {
        chunkedEncoding = true;
        log("Chunked encoding rilevato");
      } else if (line.startsWith("Location:") && (statusCode == 301 || statusCode == 302 || statusCode == 307)) {
        if (redirectCount < maxRedirects) {
          String newURL = line.substring(9);
          newURL.trim();
          redirectCount++;
          client->stop();
          log("Redirect verso: " + newURL);
          beginRequest(newURL, method, payload);
          return;
        }
      } else if (line.length() == 0) {
        headersParsed = true;
        log("Fine header");
      }
    } else {
      if (chunkedEncoding) {
        String chunkSizeStr = line;
        int chunkSize = (int)strtol(chunkSizeStr.c_str(), nullptr, 16);
        if (chunkSize == 0) {
          finished = true;
          if (unifiedCallback) unifiedCallback(HTTPEventType::Response, responseBody);
          client->stop();
          log("Fine chunk\n");
          return;
        }
        String chunk = "";
        while (chunk.length() < chunkSize) {
          if (client->available()) {
            chunk += (char)client->read();
          }
        }
        client->readStringUntil('\n');
        if (unifiedCallback) unifiedCallback(HTTPEventType::Chunk, chunk);
        responseBody += chunk;
        log("Chunk ricevuto: " + chunk);
      } else {
        responseBody += line + "\n";
      }
    }
  }

  if (!client->connected()) {
    log("Connessione chiusa");
    finished = true;
    if (unifiedCallback) unifiedCallback(HTTPEventType::Response, responseBody);
    client->stop();
  }
}

