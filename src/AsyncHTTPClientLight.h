#pragma once

#include <Arduino.h>
#include <vector>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <functional>

enum class HTTPEventType {
  Response,
  Error,
  Timeout,
  Overload,
  Chunk,
  Receiving
};

using UnifiedCallback = std::function<void(HTTPEventType, const String&)>;

class AsyncHTTPClientLight {
public:
  AsyncHTTPClientLight();

  void beginRequest(const String& url, const String& method = "GET", const String& payload = "");
  int runSync(const String& url, const String& method = "GET", const String& payload = "");
  void addHeader(const String& key, const String& value);
  void setTimeout(unsigned long ms);
  void setDebug(bool enabled);
  void setLogToFile(bool enabled);
  void setMaxRetries(int retries);
  void onRetry(std::function<void(int)> cb);
  void onEvent(UnifiedCallback cb);
  void addTitle(const String& title);
  int getLastHTTPcode() const;
  String getLastResponseBody() const;
  String getLastTitle() const;

  void poll();
  bool isFinished();
  bool finished;

private:
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  WiFiClient* client;

  String host;
  String path;
  int port;
  bool useSSL;
  String method;
  String payload;
  
  std::vector<std::pair<String, String>> pendingHeaders;
  std::vector<std::pair<String, String>> inprogressHeaders;

  //    bool finished;
  bool chunkedEncoding;
  bool headersParsed;
  bool _isSyncMode = false;
 
  int statusCode;
  String responseBody;

  enum State { IDLE,
               CONNECTING,
               SENDING,
               RECEIVING };
  State state;

  unsigned long timeoutMs;
  unsigned long lastActivity;
  int requestCounter;
  int maxRetries;
  int retryCount;
  int redirectCount;
  const int maxRedirects = 3;

  bool debugEnabled;
  bool logToFile;
  String logPrefix;
  String pendingTitle;
  String inprogressTitle;

  UnifiedCallback unifiedCallback;
  std::function<void(int)> retryCallback;

  void reset();
  bool parseURL(const String& url);
  void log(const String& msg);
  void checktimeout();
  void connecting();
  void sending();
  void receiving();
  void triggerEvent(HTTPEventType type, const String& message);
};
