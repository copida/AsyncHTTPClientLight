#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <vector>

typedef std::function<void(String chunk)> ChunkCallback;
typedef std::function<void(int statusCode, String body)> ResponseCallback;
typedef std::function<void(String error)> ErrorCallback;

class AsyncHTTPClientLight {
public:
    AsyncHTTPClientLight();

    void beginRequest(const String& url, const String& method = "GET", const String& payload = "");
    void addTitle(const String& title);
    void addHeader(const String& key, const String& value);
    void setTimeout(unsigned long ms);
    void setDebug(bool enabled);
    void onChunkReceived(ChunkCallback cb);
    void onRequestComplete(ResponseCallback cb);
    void onError(ErrorCallback cb);

    String lastOverloadTitle;
    std::function<void(const String&)> overloadCallback;

    void onOverload(std::function<void(const String&)> cb);
//    using OverloadCallback = std::function<void()>;

//    void onOverload(OverloadCallback cb);
    void poll();
    bool isFinished();
    void setLogToFile(bool enabled); // attiva/disattiva log su file


private:
    enum State { IDLE, CONNECTING, SENDING, RECEIVING, FINISHED };
    State state;

    WiFiClient* client;
    WiFiClientSecure secureClient;
    WiFiClient plainClient;

    String host, path;
    int port;
    bool useSSL;

    String method;
    String payload;
    std::vector<std::pair<String, String>> headers;

    unsigned long timeoutMs;
    unsigned long lastActivity;

    bool finished;
    bool chunkedEncoding;
    bool headersParsed;
    int statusCode;
    String responseBody;

    ChunkCallback chunkCallback;
    ResponseCallback responseCallback;
    ErrorCallback errorCallback;


    bool parseURL(const String& url);
    void reset();

    void log(const String& msg);

    int redirectCount;
    const int maxRedirects = 1;
//    bool internalRedirect = false;

    String pendingTitle;
    unsigned int requestCounter = 0;
    String logPrefix = "[REQ 0000] ";
    bool debugEnabled;
};
