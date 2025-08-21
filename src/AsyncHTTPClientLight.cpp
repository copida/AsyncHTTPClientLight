#include "AsyncHTTPClientLight.h"
#define ASYNC_HTTP_DEBUG
//#define ASYNC_HTTP_LOG_SPIFFS
#define ASYNC_HTTP_LOG_SD

#ifdef ASYNC_HTTP_DEBUG
#ifdef ASYNC_HTTP_LOG_SPIFFS
#include <SPIFFS.h>
#endif
#ifdef ASYNC_HTTP_LOG_SD
#include <SD.h>
#endif
#endif


bool logToFile = false;


AsyncHTTPClientLight::AsyncHTTPClientLight() {
    reset();
    timeoutMs = 10000;
    debugEnabled = false;
    requestCounter = 0;
}

void AsyncHTTPClientLight::setDebug(bool enabled) {
    debugEnabled = enabled;
}
// per SPIFFS
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

void AsyncHTTPClientLight::log(const String& msg) {
#ifdef ASYNC_HTTP_DEBUG
//    String prefix = "[REQ " + String(requestCounter) + "] ";
//    String fullMsg = prefix + msg;
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
///end SPIFFS

void AsyncHTTPClientLight::reset() {
    client = nullptr;
    host = "";
    path = "/";
    port = 80;
    useSSL = false;
    method = "GET";
    payload = "";
    headers.clear();
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

void AsyncHTTPClientLight::beginRequest(const String& url, const String& method_, const String& payload_) {
    if (!isFinished() && redirectCount == 0) {
        lastOverloadTitle = pendingTitle.length() ? pendingTitle : "(nessun titolo)";
        log("⚠️ Overload: richiesta già in corso. Ignorata: " + lastOverloadTitle);
        if (overloadCallback) overloadCallback(lastOverloadTitle);
        return;
    }

    reset();
    requestCounter++;
    logPrefix = "[REQ " + String(requestCounter) + "] ";

    if (pendingTitle.length()) {
        log( " === " + pendingTitle + " ===");
        pendingTitle = "";
    }

    if (!parseURL(url)) {
        finished = true;
	log(logPrefix + "ERRORE URL non valido");
        if (errorCallback) errorCallback(logPrefix + "URL non valido");
        return;
    }

    method = method_;
    payload = payload_;
    client = useSSL ? &secureClient : &plainClient;
    if (useSSL) secureClient.setInsecure();

    state = CONNECTING;
    lastActivity = millis();
    finished = false;

    log("Inizio richiesta " + method + " a " + url);
}

void AsyncHTTPClientLight::addHeader(const String& key, const String& value) {
    headers.push_back({key, value});
}

void AsyncHTTPClientLight::setTimeout(unsigned long ms) {
    timeoutMs = ms;
}

void AsyncHTTPClientLight::onChunkReceived(ChunkCallback cb) {
    chunkCallback = cb;
}

void AsyncHTTPClientLight::onRequestComplete(ResponseCallback cb) {
    responseCallback = cb;
}

void AsyncHTTPClientLight::onError(ErrorCallback cb) {
    errorCallback = cb;
}

void AsyncHTTPClientLight::onOverload(std::function<void(const String&)> cb) {
    overloadCallback = cb;
}

bool AsyncHTTPClientLight::isFinished() {
    return finished;
}

void AsyncHTTPClientLight::poll() {
    if (finished || !client) return;

    if (millis() - lastActivity > timeoutMs) {
        finished = true;
        if (errorCallback) errorCallback(logPrefix + "Timeout");
        log("Timeout");
        return;
    }

    switch (state) {
        case CONNECTING:
            if (client->connect(host.c_str(), port)) {
                state = SENDING;
                lastActivity = millis();
                log("Connessione riuscita");
            } else {
                finished = true;
                if (errorCallback) errorCallback(logPrefix + "Connessione fallita");
                log("Connessione fallita");
            }
            break;

        case SENDING: {
            String request = method + " " + path + " HTTP/1.1\r\n";
            request += "Host: " + host + "\r\n";
            for (auto& h : headers) {
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
            log("Richiesta inviata");
            break;
        }

        case RECEIVING:
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
                        int chunkSize = (int) strtol(chunkSizeStr.c_str(), nullptr, 16);
                        if (chunkSize == 0) {
                            finished = true;
                            if (responseCallback) responseCallback(statusCode, responseBody);
                            client->stop();
                            log("Fine chunk");
                            return;
                        }
                        String chunk = "";
                        while (chunk.length() < chunkSize) {
                            if (client->available()) {
                                chunk += (char)client->read();
                            }
                        }
                        client->readStringUntil('\n');
                        if (chunkCallback) chunkCallback(chunk);
                        responseBody += chunk;
                        log("Chunk ricevuto: " + chunk);
                    } else {
                        responseBody += line + "\n";
                    }
                }
            }

            if (!client->connected()) {
                finished = true;
                if (responseCallback) responseCallback(statusCode, responseBody);
                client->stop();
                log("Connessione chiusa");
            }
            break;

        default:
            break;
    }
}
