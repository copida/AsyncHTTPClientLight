# AsyncHTTPClientLight

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)
![Version](https://img.shields.io/badge/version-2.0.0-lightgrey.svg)

> A lightweight asynchronous and synchronous HTTP client for ESP32 with automatic redirects, retries, HTTPS support, chunked transfers, and detailed debugging.

**Author:** Davide  
**License:** MIT  
**Version:** 2.0 (AsyncClient Evolution)

---

## 📋 Table of Contents

- #-what-is-it
- #-key-features
- #-why-asynchttpclientlight
- #-installation
- #-quick-start
- #-usage-modes
- #-main-api
- #-callback-events
- #-https
- #-included-examples
- #-logging
- #-important-notes
- #-troubleshooting
- #-data-structures
- #-quick-reference
- #-suggested-tests
- #-contributing
- #-license

---

## 📡 What Is It?

**AsyncHTTPClientLight** is a lightweight HTTP client library for ESP32 that provides both:

- **Asynchronous (non-blocking) requests**
- **Synchronous (blocking) requests**

Designed as a stable alternative to Arduino's `HTTPClient`, it includes:

- Automatic redirect handling
- Smart retry mechanism
- HTTPS support
- Chunked transfer support
- Advanced logging
- Configurable response buffers
- PSRAM support for large payloads

Ideal for:

- IoT telemetry
- Cloud data logging
- REST APIs
- Google Sheets integrations
- Adafruit IO
- ESP32 FreeRTOS applications

---

## 🎯 Key Features

| Feature | Description |
|----------|-------------|
| ✅ Async Requests | Non-blocking state-machine architecture |
| ✅ Sync Requests | Traditional blocking API |
| ✅ Mixed Mode | Use sync and async together |
| ✅ Automatic HTTPS | Enables `WiFiClientSecure` automatically |
| ✅ Redirects | Supports 301, 302, 303, 307, 308 |
| ✅ Smart Retries | Configurable retry mechanism |
| ✅ Chunked Transfer | Full support |
| ✅ Custom Headers | Authorization, API keys, custom tokens |
| ✅ Unified Callbacks | Single event handler |
| ✅ Logging | Serial, SD, SPIFFS, LittleFS |
| ✅ PSRAM Support | Large payload downloads |
| ✅ FreeRTOS Friendly | ESP32 multitasking ready |

---

## ⭐ Why AsyncHTTPClientLight?

| Feature | HTTPClient | AsyncHTTPClientLight |
|----------|------------|---------------------|
| Non-blocking requests | ❌ | ✅ |
| Unified callback system | ❌ | ✅ |
| Automatic redirects | ⚠️ Limited | ✅ |
| Retry handling | ❌ | ✅ |
| Chunked transfer support | ⚠️ Partial | ✅ |
| Detailed logging | ❌ | ✅ |
| PSRAM payload buffers | ❌ | ✅ |
| ESP32 multitasking support | ⚠️ | ✅ |

---

## 📦 Installation

### Manual Installation

1. Clone or download this repository.

```bash
git clone https://github.com/copida/AsyncHTTPClientLight.git
```

2. Copy the folder into your Arduino libraries directory:

```text
~/Arduino/libraries/AsyncHTTPClientLight
```

3. Restart Arduino IDE.

### Arduino Library Manager

Coming soon.

### Dependencies

- Arduino.h
- WiFiClient.h
- WiFiClientSecure.h
- STL `vector`
- STL `functional`

Optional:

- SD.h
- SPIFFS.h
- LittleFS.h

---

## 🚀 Quick Start

### Asynchronous Mode (Recommended)

```cpp
#include <WiFi.h>
#include "AsyncHTTPClientLight.h"

AsyncHTTPClientLight http;

void handleEvent(HTTPEventType type,
                 const HTTPResponse* res) {

  if (type == HTTPEventType::Response) {
    Serial.printf("HTTP %d\n", res->statusCode);
    Serial.println(http.getResponsePayload());
  }
}

void setup() {

  Serial.begin(115200);

  WiFi.begin("SSID", "PASSWORD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  http.setDebug(true);
  http.onEvent(handleEvent);
  http.addTitle("My Request");

  http.beginRequest(
    "https://api.example.com/data",
    "GET"
  );
}

void loop() {

  http.poll();      // REQUIRED

  delay(100);
}
```

### Synchronous Mode

```cpp
void setup() {

  WiFi.begin("SSID", "PASSWORD");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  int httpCode = http.runSync(
    "https://api.example.com/data",
    "GET"
  );

  Serial.printf(
    "HTTP %d\n",
    httpCode
  );

  Serial.println(
    http.getResponsePayload()
  );
}

void loop() {}
```

---

## 💡 Usage Modes

### 1️⃣ Asynchronous (Non-Blocking)

```cpp
http.beginRequest(
  url,
  "POST",
  jsonPayload
);

// loop()
http.poll();

if (http.isFinished()) {
  Serial.println(
    http.getResponsePayload()
  );
}
```

**Advantages**

- Main loop remains responsive
- Sensors continue working
- Displays update correctly
- Event-based notifications

**Disadvantages**

- Slightly more complex
- One request at a time

---

### 2️⃣ Synchronous (Blocking)

```cpp
int code =
  http.runSync(url, "GET");

Serial.println(
  http.getResponsePayload()
);
```

**Advantages**

- Simple API
- Immediate response

**Disadvantages**

- Blocks execution
- No multitasking while waiting

---

### 3️⃣ Mixed Mode

```cpp
void loop() {

  http.poll();

  if (someEvent) {

    int code =
      http.runSync(
        url,
        "GET"
      );

    // Completes any async
    // request currently running
  }
}
```

---

## 🔁 Main API

| Function | Description |
|-----------|-------------|
| `beginRequest()` | Start async request |
| `runSync()` | Perform synchronous request |
| `poll()` | Drive async state machine |
| `isFinished()` | Request completed? |
| `getLastHTTPcode()` | Last HTTP code |
| `getResponsePayload()` | Response buffer |
| `setResponsePayload()` | Custom response buffer |
| `addHeader()` | Add HTTP header |
| `addTitle()` | Request logging label |
| `setTimeout()` | Request timeout |
| `setMaxRetries()` | Retry count |
| `setmaxRedirects()` | Maximum redirects |
| `setDebug()` | Enable Serial logging |
| `setLogToFile()` | File logging |
| `onEvent()` | Event callback |

---

## 🔄 Callback Events

```cpp
void handleEvent(
  HTTPEventType type,
  const HTTPResponse* res
) {

  switch (type) {

    case HTTPEventType::Response:

      Serial.printf(
        "✅ HTTP %d\n",
        res->statusCode
      );

      Serial.println(
        http.getResponsePayload()
      );
      break;

    case HTTPEventType::Timeout:

      Serial.printf(
        "⏱️ Timeout: %s\n",
        res->msg_error
      );
      break;

    case HTTPEventType::Error:

      Serial.printf(
        "❌ Error: %s\n",
        res->msg_error
      );
      break;

    case HTTPEventType::Overload:

      Serial.println(
        "⚠️ Request already running"
      );
      break;

    case HTTPEventType::Chunk:

      Serial.printf(
        "📦 Chunk: %d bytes\n",
        res->contentLength
      );
      break;

    default:
      break;
  }
}
```

> ⚠️ `getResponsePayload()` is valid only inside the callback.

---

## 🔐 HTTPS

HTTPS is enabled automatically whenever the URL starts with:

```text
https://
```

Example:

```cpp
http.beginRequest(
  "https://api.example.com/data",
  "GET"
);
```

### Security Notice

By default the library uses:

```cpp
setInsecure();
```

This means:

✅ Traffic is encrypted  
❌ Server identity is NOT verified

Suitable for:

- Telemetry
- Sensors
- Public APIs
- Logging

Not recommended for:

- Passwords
- Authentication tokens
- Sensitive user data

For production-grade security, certificate validation with `setCACert()` should be implemented.

---

## 📝 Included Examples

### Sync_GET.ino

Simple blocking GET and POST requests.

### GoogleSheets_Logger.ino

Features:

- Periodic JSON upload
- Google Apps Script redirects
- Unified callback handling
- WiFi auto-reconnect

### Adafruit_IO_History.ino

Features:

- 50KB PSRAM buffer
- ArduinoJson v7 support
- API authentication headers
- Historical data download

---

## 💾 Logging

### Serial Logging

```cpp
http.setDebug(true);
```

Example output:

```text
[REQ 1] === My Request ===
[REQ 1] Protocol: HTTPS
[REQ 1] Host: api.example.com
[REQ 1] Port: 443
[REQ 1] Attempt 1
[REQ 1] Connection successful
[REQ 1] Sending...
[REQ 1] Status code: 200
[REQ 1] Content-Length: 512
```

### File Logging

Inside:

```cpp
src/AsyncHTTPClientLight.cpp
```

Configure:

```cpp
#define ASYNC_HTTP_DEBUG 1

#if ASYNC_HTTP_DEBUG
  #define ASYNC_HTTP_LOG_SD
  #define MAXSIZEFILE_LOG 512000
#endif
```

Enable:

```cpp
http.setLogToFile(true);
```

Generated files:

```text
/http_log.txt
/old_Log.txt
```

---

## 🧠 Important Notes

### Payload Lifetime

✅ Correct:

```cpp
void handleEvent(
  HTTPEventType type,
  const HTTPResponse* res
) {

  if (type == HTTPEventType::Response) {
    Serial.println(
      http.getResponsePayload()
    );
  }
}
```

❌ Wrong:

```cpp
Serial.println(
  http.getResponsePayload()
);
```

The buffer may already have been released.

---

### Always Call poll()

```cpp
void loop() {

  http.poll();

  // Your application code
}
```

---

### One Request at a Time

```cpp
if (http.isFinished()) {

  http.beginRequest(
    url,
    "GET"
  );

} else {

  Serial.println(
    "Previous request still running"
  );
}
```

---

### Use PSRAM for Large Payloads

```cpp
char* bigBuffer =
  (char*)ps_malloc(50000);

http.setResponsePayload(
  bigBuffer,
  50000
);

http.beginRequest(
  url,
  "GET"
);

free(bigBuffer);
```

---

## 🚨 Troubleshooting

### Request fails for no apparent reason

```cpp
http.setDebug(true);
```

Review log messages in Serial Monitor.

---

### Random timeout

```cpp
http.setTimeout(15000);
```

---

### Redirect not followed

```cpp
http.setmaxRedirects(5);
```

---

### Corrupted response

```cpp
http.setResponsePayload(
  largeBuffer,
  bufferSize
);
```

---

### Memory leak or crash

Verify:

```cpp
http.poll();
```

is called every loop iteration.

---

### WiFi disconnect

```cpp
if (WiFi.status() != WL_CONNECTED) {

  Serial.println(
    "WiFi lost, reconnecting..."
  );

  WiFi.reconnect();

  return;
}
```

---

## 📊 Data Structures

### HTTPResponse

```cpp
struct HTTPResponse {

  int statusCode;
  uint32_t restime;

  char inprogressTitle[64];
  char contentType[45];

  int contentLength;

  bool isStream;
  bool isChunked;

  int expectedLength;

  char* ptr_workbuffer;

  char msg_error[50];
};
```

### HTTPEventType

```cpp
enum class HTTPEventType {

  Response,
  Error,
  Timeout,
  Overload,
  Chunk,
  Receiving,
  Line
};
```

---

## 📖 Quick Reference

| Use Case | API |
|-----------|------|
| Setup / Configuration | `runSync()` |
| Periodic Telemetry | `beginRequest()` + `poll()` |
| Large Downloads | `setResponsePayload()` |
| Bearer Tokens | `addHeader()` |
| Google Sheets | `setmaxRedirects()` |
| Adafruit IO | `setmaxRedirects()` |
| Troubleshooting | `setDebug()` |

---

## 🧪 Suggested Tests

### Delayed Response

```text
https://httpbin.org/delay/5
```

### HTTP 404

```text
https://httpbin.org/status/404
```

### JSON Response

```text
https://httpbin.org/json
```

### POST Echo

```text
https://httpbin.org/post
```

---

## 🤝 Contributing

Contributions are welcome.

Feel free to:

- Open Issues
- Submit Pull Requests
- Report bugs
- Suggest new features

---

## 🌟 Support the Project

If this library helped you:

- ⭐ Star the repository
- 🐛 Report bugs
- 💡 Suggest improvements
- 📢 Share it with other ESP32 developers

---

## 📜 License

Released under the MIT License.

See the `LICENSE` file for details.

---

**Maintainer:** Davide (copida.soft@gmail.com)

**Repository:**  
https://github.com/copida/AsyncHTTPClientLight
