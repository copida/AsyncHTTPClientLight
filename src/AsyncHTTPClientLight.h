#pragma once

#include <Arduino.h>
#include <vector>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <functional>

#define LINE_BUFFER_SIZE 768
#define PATH_BUFFER 512

#define CHUNK_WAITING -1

#define WORKBUFFER workBuffer
#define PATHBUFFER workBuffer
#define WORKBUFFER_SIZE 512


enum class HTTPEventType {
  Response,	// fine risposta
  Error,
  Timeout,
  Overload,
  Chunk,	// dati chunked
  Receiving,
  Line	// linea normale (non chunked)
};

enum ChunkState {
	WAITING_SIZE,
	READING_DATA,
	SKIPPING_CRLF,
	FINISHED
};

struct HTTPResponse {
	int statusCode = -1;
	char inprogressTitle[64] = {0};
	char contentType[45] = {0};
	int contentLength = 0;
	bool isStream = false;
	bool isChunked = false;
	int expectedLength = CHUNK_WAITING;
	
	char* ptr_workbuffer = nullptr;			// per chunked e altro..
	char msg_error[40] = {0};			// per messaggi di errore
};


//using UnifiedCallback = std::function<void(HTTPEventType, const char* data, size_t length)>;
using UnifiedCallback = std::function<void(HTTPEventType type, const HTTPResponse* response)>;


class AsyncHTTPClientLight {
	public:
  AsyncHTTPClientLight();
		
	void setResponsePayload(char* buffer, size_t maxLen) {
    responsePayloadBuffer = buffer;
    responsePayloadMaxLen = maxLen;
		newPtrOut = true;
	}
	
	const char* getResponsePayload() {
		return responsePayloadBuffer;
	}
	
  void beginRequest(const String& url, const String& method = "GET", const String& payload = ""){
		const char* p = (payload.length() > 0)? payload.c_str(): nullptr;
		return beginRequest(url.c_str(), method.c_str(), p);
	};
	void beginRequest(const char* url, const char* method = "GET", const char* payload = nullptr);
	
	
	int runSync(const String& url, const String& method = "GET", const String& payload = ""){
		const char* p = (payload.length() > 0)? payload.c_str(): nullptr;
		return runSync(url.c_str(), method.c_str(), p);
	};
	
	int runSync(const char* url, const char* method = "GET", const char* payload = "");
	
	
  void addHeader(const String& key, const String& value);
	void addTitle(const String& title);
  void setTimeout(unsigned long ms);
  void setDebug(bool enabled);
  void setLogToFile(bool enabled);
  void setMaxRetries(int retries);
  void onEvent(UnifiedCallback cb);
	
	int getLastHTTPcode() const;
	//String getLastTitle() const;
	
	void poll();
	bool isFinished();
	bool finished;
	

	private:
	WiFiClient plainClient;
	WiFiClientSecure secureClient;
	WiFiClient* client;
	
	
	HTTPResponse response;
	
	char* responsePayloadBuffer = nullptr;
	size_t responsePayloadMaxLen = 0;
	bool newPtrOut = false;
	
	char host[40];
	char workBuffer[WORKBUFFER_SIZE];
	int port;
	bool useSSL;
	String method = "       ";
	char* ptr_Inpayload = nullptr;
	
	std::vector<std::pair<String, String>> pendingHeaders;
	std::vector<std::pair<String, String>> inprogressHeaders;
	
	enum State { IDLE,
		CONNECTING,
		SENDING,
	RECEIVING };
	State state;
	
	char lineBuffer[LINE_BUFFER_SIZE];
	size_t bufIndex = 0;
	String logPrefix;
	char pendingTitle[64] = {0};  // titolo massimo 63 caratteri + terminatore
	
	bool _isSyncMode = false;
	unsigned long timeoutMs;
	unsigned long lastActivity;
	bool payloadAllocated = false;
	bool headersParsed;
	int chunkState = WAITING_SIZE;
	
	int requestCounter;
	int maxRetries;
	int retryCount;
	int redirectCount;
	int maxRedirects = 1;
		
	bool debugEnabled;
	bool logToFile;
		
	
	UnifiedCallback unifiedCallback;
	std::function<void(int)> retryCallback;
	
	void reset();
	bool parseURL(const char* url);
	void log(const String& msg);
	void checktimeout();
	void connecting();
	void sending();
	void receiving();
	int trimmer(char* buftrim,  int dadove = 0);
	int search_strbuf(const char* buffer, char* str_cmp, int fromwhere = 0);
	void releasePayload();
	void parseHeaders();
	int readUntilTerminator(Stream* client, char* buffer, size_t maxLen, char terminator, unsigned long timeoutMs, bool delCR = true);
	bool readChunked();
	int readStream(char* buffer, int lenbuffer, int ndati );
	
	void triggerEvent(HTTPEventType type, const char* data);
	void triggerEvent(HTTPEventType type, const String& message);
	
};
