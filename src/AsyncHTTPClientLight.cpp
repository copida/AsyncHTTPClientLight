#include "AsyncHTTPClientLight.h"

#define ASYNC_HTTP_DEBUG 1

#if ASYNC_HTTP_DEBUG
	#pragma message "### AsyncHTTPClientLight: Funzionalita DEBUG ATTIVATE  ###"
	
	#define ASYNC_HTTP_LOG_SD
	//#define ASYNC_HTTP_LOG_SPIFFS
	//#define MAXSIZEFILE_LOG 51200
	#define MAXSIZEFILE_LOG 512000
	
	#ifdef ASYNC_HTTP_LOG_SPIFFS
		#include <SPIFFS.h>
		#define FS_LOG SPIFFS
		#pragma message "### AsyncHTTPClientLight: Funzionalità 'SPIFFS' incluse. ###"
	#endif
	
	#ifdef ASYNC_HTTP_LOG_SD
		#include <SD.h>
		#define FS_LOG SD
		#pragma message "### AsyncHTTPClientLight: Funzionalità 'SD' incluse. ###"
	#endif
	
	#else
	#define log(x)
#endif


AsyncHTTPClientLight::AsyncHTTPClientLight() {
	reset();
	timeoutMs = 10000;
	debugEnabled = false;
	requestCounter = 0;
	maxRetries = 1;
	maxRedirects = 1;
	retryCount = 1;
	redirectCount = 0;
	//------
	
	sprintf(response.contentType,"application/octet-stream"); // default
	response.expectedLength = -1;
  	response.contentLength = 0;
  	response.isChunked = false;
	response.statusCode = -1;
	responsePayloadBuffer = lineBuffer;
	responsePayloadMaxLen = sizeof(lineBuffer);
  	response.ptr_workbuffer = PATHBUFFER;
}

void AsyncHTTPClientLight::triggerEvent(HTTPEventType type, const char* data) {
	if (unifiedCallback) {
		snprintf(response.msg_error, sizeof(response.msg_error),"%s", data);
		unifiedCallback(type, &response);
	}
}

void AsyncHTTPClientLight::triggerEvent(HTTPEventType type, const String& message) {
	triggerEvent(type, message.c_str());
}


void AsyncHTTPClientLight::setMaxRetries(int retries) {
	if (retries >= 1) maxRetries = retries;
}


void AsyncHTTPClientLight::setDebug(bool enabled) {
	debugEnabled = enabled;
}

#if ASYNC_HTTP_DEBUG
	void AsyncHTTPClientLight::log(const String& msg){
		#define oldLogFile "/old_Log.txt"
		#define logFile "/http_log.txt"
		
		//String fullMsg = logPrefix + msg;
		
		if (debugEnabled) Serial.println(logPrefix + msg);
		#if defined(ASYNC_HTTP_LOG_SPIFFS) || defined(ASYNC_HTTP_LOG_SD)
			if (logToFile) {
				
				File f = FS_LOG.open(logFile, FILE_APPEND);
				
				if (f) {
					f.print(logPrefix);
					f.println(msg);
					//f.println(fullMsg);
					//f.close();
				}
				// Rotazione semplice se supera maxSize
        if (f.size() > MAXSIZEFILE_LOG) {
					f.close();
					FS_LOG.remove(oldLogFile);
					FS_LOG.rename(logFile, oldLogFile);
					f = FS_LOG.open(logFile, FILE_WRITE);  // Crea nuovo
					}else{
					f.close();
				}
			}
		#endif
	}
#endif

#if ASYNC_HTTP_DEBUG
	void AsyncHTTPClientLight::setLogToFile(bool enabled) {
		logToFile = enabled;
		
		#if defined(ASYNC_HTTP_LOG_SPIFFS) || defined(ASYNC_HTTP_LOG_SD)
			if (enabled && !FS_LOG.begin(true)) {
				log("FS_LOG non inizializzato");
				}else{
				log("FS_LOG inizializzato");
			}
		#endif
		
	}
#endif


int AsyncHTTPClientLight::getLastHTTPcode() const {
	return response.statusCode;
}

// String AsyncHTTPClientLight::getLastTitle() const {
// return String(response.inprogressTitle);
// }

void AsyncHTTPClientLight::reset() {
	client = nullptr;
	port = 80;
	useSSL = false;
	method = "GET";
	finished = true;
	headersParsed = false;
	state = IDLE;
	redirectCount = 0;
	retryCount = 1;
	bufIndex = 0;
	
	// Resetta struttura response
  	memset(&response, 0, sizeof(response));
  	response.statusCode = -1;
  	response.contentLength = 0;
	response.expectedLength = -1;
	response.isStream = false;
	response.isChunked = false;
	
	if(!newPtrOut){
		responsePayloadBuffer = lineBuffer;
    responsePayloadMaxLen = LINE_BUFFER_SIZE;
		}else{
		newPtrOut = false;
	}
	responsePayloadBuffer[0] = '\0';
	response.msg_error[0] = '\0';
	response.ptr_workbuffer = WORKBUFFER;
	
}


bool AsyncHTTPClientLight::parseURL(const char* url) {
	
  useSSL = false;
  if(search_strbuf(url, "https://") == 0) useSSL = true;
	port = useSSL ? 443 : 80;
	
  int indexhost = search_strbuf(url, "://", 0);
  if (indexhost == -1) return false;
  indexhost = indexhost + 3;
  int pathIndex = search_strbuf(url, "/", indexhost);
	
  snprintf(PATHBUFFER, sizeof(PATHBUFFER),"%s", (pathIndex == -1)? "/" : &url[pathIndex] );	
	//http://10.255.255.1
	if(pathIndex == -1)pathIndex = strlen(url);
  snprintf(host, pathIndex - indexhost +1,"%s", &url[indexhost] );
	
	log("Protocollo: " + String(useSSL ? "HTTPS" : "HTTP"));
	log("Porta: " + String(port));
	log("Host: " + String(host));
	log("Path: " + String(PATHBUFFER));
	
  return true;
}

void AsyncHTTPClientLight::addTitle(const String& title) {
	snprintf(pendingTitle, sizeof(pendingTitle),"%s", title.c_str());
}

int AsyncHTTPClientLight::runSync(const char* url, const char* methodGET, const char* payload) {
	// aspetto se eventualmente c'è una richiesta asincrona in corso la porto a termine
	if(!finished){
		log("Wait end Asincrona : " + String(response.inprogressTitle));
		log("Pending Sincrona : " + String(pendingTitle));
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
	return response.statusCode;
}


void AsyncHTTPClientLight::beginRequest(const char* url, const char* method_, const char* payload_) {
	
	if (!isFinished() && redirectCount == 0) {
		log("Overload: richiesta già in corso: " + String(response.inprogressTitle));
		triggerEvent(HTTPEventType::Overload, String(pendingTitle));
		return;
	}
	
	requestCounter++;
	logPrefix = "[REQ " + String(requestCounter) + "] ";
	
	if (finished) {	//nuova richiesta
		reset();
		
		snprintf(response.inprogressTitle, sizeof(response.inprogressTitle), "%s", pendingTitle);
		log("\n === " + String(response.inprogressTitle) + " ===");
		snprintf(pendingTitle, sizeof(pendingTitle), "%s", "(nessun titolo)");
		
		inprogressHeaders.clear();
		inprogressHeaders = pendingHeaders;
		pendingHeaders.clear();
	}
	
	
	if (!parseURL(url)) {
		finished = true;
		log("ERRORE URL non valido");
		snprintf(response.msg_error, sizeof(response.msg_error), "URL non valido");
		if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
		//triggerEvent(HTTPEventType::Error, logPrefix + "URL non valido");
		return;
	}
	
	method = method_;
	
	if (!_isSyncMode && payload_ != nullptr) {
		//Serial.println("payload da allocare");
		// questa funzione server per fare una copia del payload in spazio memoria allocata in 
		// caso di richieta asincrona e che il payload sia locale alla funzione chiamante
		// perdita del puntatore --  a meno che sia dichiarato globalmente
		size_t len = strlen(payload_);
		if(len > 1){
			
			ptr_Inpayload = (char*)malloc(len + 1);
			if (ptr_Inpayload != nullptr) {
				strcpy(ptr_Inpayload, payload_);
				payloadAllocated = true;
				log("memoria allocata ");
				//Serial.print("memoria allocata ");
				//Serial.println(len + 1);
				} else {
				log("Errore malloc payload");
				payloadAllocated = false;
			}
		}
	}
	
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
	//unsigned long startTime;
	//  if (finished || !client) return;
	if (finished || state == IDLE) return;
	
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
		log("Timeout");
		snprintf(response.msg_error, sizeof(response.msg_error),"Timeout");
		if(retryCount < maxRetries){
			triggerEvent(HTTPEventType::Error, logPrefix + "Timeout");
		}
		
		client->stop();
		retryCount++;
		state = CONNECTING;
		lastActivity = millis();
		return;
	}
}

void AsyncHTTPClientLight::connecting() {
	
	if (retryCount > maxRetries || redirectCount > maxRedirects){
		//client->stop();
		releasePayload();
		
		if(redirectCount > maxRedirects){
			snprintf(response.msg_error, sizeof(response.msg_error), "Too many redirection");
			if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
			//if (unifiedCallback) unifiedCallback(HTTPEventType::Response, "Too many redirection");
		}
		
		
		if (retryCount > maxRetries){
			snprintf(response.msg_error, sizeof(response.msg_error), "Superato num tentativi");
			if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
			//if (unifiedCallback) unifiedCallback(HTTPEventType::Response, "Superato num tentativi", -1);
		}
		
		// retryCount = 1;
		// redirectCount = 0;
		finished = true;
		state = IDLE;
		return;
	}
	
	responsePayloadBuffer[0] = '\0';
	
	client = useSSL ? &secureClient : &plainClient;
	if (useSSL) secureClient.setInsecure();
	//  log("Client creato puntatore: " + String((uintptr_t)client));
	
	if(retryCount == 1 && redirectCount == 1){
		log("REDIRECT..");
	}
	log("Tentativo n:" + String(retryCount));
	
	if (client->connect(host, port)) {
		state = SENDING;
		lastActivity = millis();
		log("Connessione riuscita");
		return;
		} else {
		client->stop();
		log("Connessione fallita");
		retryCount++;
		state = CONNECTING;
		
	}
}

void AsyncHTTPClientLight::sending() {
	
	// Serial.println("=== HEADER LIST ===");
	// for (auto& h : inprogressHeaders) {
	// Serial.println(h.first + ": " + h.second);
	// }
	// Serial.println("===================");
	
	int x = 0;
	
	//Serial.println(ptr_Inpayload);
	//Serial.println((unsigned long)&ptr_Inpayload, HEX);
	
  snprintf(lineBuffer, sizeof(lineBuffer), "%s %s HTTP/1.1\r\n", method, PATHBUFFER);
  client->print(lineBuffer);
	
	
  // Host header
  snprintf(lineBuffer, sizeof(lineBuffer), "Host: %s\r\n", host);
  client->print(lineBuffer);
	
	//headers
	for (auto& h : inprogressHeaders) {
		//Serial.println("headers trovato");
		client->print(h.first + ": " + h.second + "\r\n");
	}
	
  if(ptr_Inpayload != nullptr){
		
		x = strlen(ptr_Inpayload);
		snprintf(lineBuffer, sizeof(lineBuffer), "Content-Length: %d\r\n", x);
    client->print(lineBuffer);
		client->print("Accept: text/html,application/json\r\n");
	}
	
	client->print("Connection: close\r\n\r\n");
	
	if(x >0) client->print(ptr_Inpayload);
	
	log("SHIPPED: " + (String(ptr_Inpayload != nullptr ? ptr_Inpayload : "done")));
	
	
	state = RECEIVING;
	lastActivity = millis();
	//client->flush();
}


void AsyncHTTPClientLight::receiving() {
	int len;
	
	if(!headersParsed){
		
		
		while (client->available()) {
			lastActivity = millis();
			//while (millis() - lastActivity < timeoutMs && !headersParsed) {
			len = readUntilTerminator(client, lineBuffer, sizeof(lineBuffer)-1, '\n', timeoutMs);
			if (len < 0) break;
			
			// Fine degli header
			if (strlen(lineBuffer) > 0){
				parseHeaders();
				} else{
				log("Fine header");
				headersParsed = true;
				break;
			}
		}
		// if(millis() - lastActivity > timeoutMs && !headersParsed){
		// triggerEvent(HTTPEventType::Error, logPrefix + "Timeout");
		// return;
		// }
		if(!headersParsed)return;
	}
	
	
	if (response.statusCode == 301 || response.statusCode == 302 || response.statusCode == 307) {
		client->stop();
		headersParsed = false;
		response.contentLength = 0;
		redirectCount++;
		retryCount = 1;
		state = CONNECTING;
		return;
	}
	
	
	if(response.isStream) {
		lastActivity = millis();
		
		len = 0;
		len = readStream(responsePayloadBuffer, responsePayloadMaxLen -1, response.contentLength );
		
		if(len != response.contentLength) {
			log("Errore dati nello stream");
			triggerEvent(HTTPEventType::Error, logPrefix + "n. dati non corrispondono");
		} 
		response.isStream = false;
	} 
	
	
	if (response.isChunked){
		lastActivity = millis();
		if(readChunked())response.isChunked = false;		// stop lettura chunked
		if(!response.isChunked){
			//Serial.print(responsePayloadBuffer);
			len = strlen(responsePayloadBuffer);
			
		} 	
		// if (diagnostics.chunkMalformed) {
		// triggerEvent(HTTPEventType::Warning, "Chunk malformato");
		// }
	} 
	
	if(!response.isStream  && !response.isChunked){			//finito
		state = IDLE;
		finished = true;
		client->stop();
		releasePayload();
		if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
	}
	
}	//end
//-----------------------

int AsyncHTTPClientLight::readStream(char* buffer, int lenbuffer, int ndati ){
	
	int count_ch = 0;
	int readed = 0;
	
	if(ndati == 0)return ndati;
	log("verranno letti:" + String((ndati > lenbuffer -1)? lenbuffer -1:ndati) + " bytes di " +String(ndati));
	if(lenbuffer -1 < ndati)triggerEvent(HTTPEventType::Error, logPrefix + "Buffer too small " + String(lenbuffer -1) + " need " + String(ndati));
	
	while (millis() - lastActivity < timeoutMs) {
		
		if (client->available()) {
			char c = client->read();
			readed++;
			#if ASYNC_HTTP_DEBUG
				if(c != '\r' && c != '\n') Serial.print(c);
				if (c == '\r')Serial.print("CR");
				if (c == '\n')Serial.print("LF");
			#endif
			if(count_ch < lenbuffer-1)buffer[count_ch++] = c;
			if (readed == ndati)break;
		}
		
	}
	Serial.println();
	buffer[count_ch] = '\0';
	response.expectedLength = readed;
	log("Letti n. bytes: " + String(readed));
	return readed;
	
}
//---------------------------------------
bool AsyncHTTPClientLight::readChunked(){
	
	bool ok_chunk = false;
	bool endchunk = false;
	int d = 0;
	
	while (millis() - lastActivity < timeoutMs) {
		
		while (client->available()) {
			char c = client->read();
			if(c == '\n' && WORKBUFFER[bufIndex-1] == '\r'){
				WORKBUFFER[bufIndex -1] = '\0';
				ok_chunk = true;
				break;
				}else{
				WORKBUFFER[bufIndex++] = c;
				if(bufIndex == sizeof(WORKBUFFER)-1){
					WORKBUFFER[bufIndex] = '\0';
					//response.sizeBuffer += bufIndex; //
					ok_chunk = true;
					break;
				}
			}
		}
		
		if(!ok_chunk)return ok_chunk;
		
		switch (chunkState) {
			case WAITING_SIZE:
			response.expectedLength = (int)strtol(WORKBUFFER, nullptr, 16);
			//Serial.printf("lunghezza chunk: %d\r\n", response.expectedLength);//
			if (response.expectedLength == 0) {
				chunkState = SKIPPING_CRLF;
				} else {
				chunkState = READING_DATA;
			}
			
			break;
			case READING_DATA:
			response.contentLength += strlen(WORKBUFFER);
			//Serial.printf("arrivati n.%d byte %d", strlen(WORKBUFFER), response.expectedLength);
			if(bufIndex == sizeof(WORKBUFFER)){
				log("Portion Chunk: " + String(WORKBUFFER));		
				}else{
				log("Chunk: " + String(WORKBUFFER));
				chunkState = WAITING_SIZE;
			}
			
			if(strlen(WORKBUFFER) != response.expectedLength){
				snprintf(responsePayloadBuffer, responsePayloadMaxLen ,"%s","Chunk malformato:\n" );
			} 
			if (unifiedCallback) unifiedCallback(HTTPEventType::Chunk, &response);
			d = snprintf(responsePayloadBuffer, responsePayloadMaxLen ,"%s%s",responsePayloadBuffer, WORKBUFFER );
			//Serial.printf("#########################%d %d\n", d,responsePayloadMaxLen);
			if(d >= responsePayloadMaxLen)triggerEvent(HTTPEventType::Error, logPrefix + "Buffer too small");
			
			break;
			case SKIPPING_CRLF:
			endchunk = true;
			chunkState = FINISHED;
			log("Chunked Finished");
			return endchunk;
			break;
			
			default:
			break;
		}
		bufIndex = 0;
		return endchunk;
	}
}

//------------------------------------
void AsyncHTTPClientLight::parseHeaders(){
	
	log("header: " + String(lineBuffer));
	
	if (search_strbuf(lineBuffer, "HTTP/") == 0) {
		int space = search_strbuf(lineBuffer, " ");
		response.statusCode = atoi(&lineBuffer[space + 1]);
		log("Status code: " + String(response.statusCode));
		return;
	}
	
	
	if (search_strbuf(lineBuffer, "Transfer-Encoding:") == 0 && search_strbuf(lineBuffer, "chunked") != -1) {
		response.isChunked = true;
		chunkState = WAITING_SIZE;
		log("Chunked encoding rilevato");
		return;
	}
	
	if (search_strbuf(lineBuffer, "Location:") == 0 && (response.statusCode == 301 || response.statusCode == 302 || response.statusCode == 307)) {
		trimmer(lineBuffer, 9);
		log("Redirect verso: " + String(lineBuffer));
		if(lineBuffer[0] == '/'){					// PATHBUFFER relativo
			snprintf(PATHBUFFER, sizeof(PATHBUFFER),"%s", lineBuffer);
			}else{
		parseURL(lineBuffer);
		}
		return;
		}
		
		
		if (search_strbuf(lineBuffer, "Content-Type:") == 0) {
		trimmer(lineBuffer, 14);
		snprintf(response.contentType, sizeof(response.contentType),"%s", lineBuffer );
		}
		else if (search_strbuf(lineBuffer, "Content-Length:") == 0) {
		trimmer(lineBuffer, 16);
		response.contentLength = atoi(lineBuffer);
		response.isStream = true;
		}
		
		}
		//------------------------------------
		int AsyncHTTPClientLight::readUntilTerminator(Stream* client, char* buffer, size_t maxLen, char terminator, unsigned long timeoutMs, bool delCR) {
		size_t index = 0;
		unsigned long start = millis();
		bool isdati = false;
		
		while (millis() - start < timeoutMs && index < maxLen - 1) {
		if (client->available()) {
		char c = client->read();
		isdati = true;
		if (c == '\r'){
		if (delCR)continue;	// ignora il carriage return
		}
		if (c == terminator) break;	
		buffer[index++] = c;
		}
		}
		
		buffer[index] = '\0';
		
		if (index == 0 && millis() - start >= timeoutMs && !isdati) {
		return -1; // timeout senza dati
		}
		
		return index; // numero di caratteri letti
		}
		//-----------------------------------
		void AsyncHTTPClientLight::releasePayload() {
		
		if(!payloadAllocated) return;
		if (payloadAllocated && ptr_Inpayload) {
			free(ptr_Inpayload);
			ptr_Inpayload = nullptr;
			payloadAllocated = false;
			log("Payload liberato");
		}
		}
		
		//-------------------------------------------------
		// FUNZIONE TRIMMER toglie spazi iniziali e finali da un buffer
		int AsyncHTTPClientLight::trimmer(char* buftrim,  int dadove) {
		
		int s = dadove;
		int e = 0;
		int _last_ch = 0;
		
		if (buftrim[0] == '\0') return _last_ch;
		do {
		if (buftrim[s] == ' ' && _last_ch == 0) {  //tolgo spazi iniziali
		s++;
		} else {
		buftrim[e] = buftrim[s];
		if (buftrim[e] != ' ') _last_ch = e;  // ultima lettera valida
		s++;
		e++;
		}
		} while (buftrim[s] != '\0');
		
		buftrim[_last_ch + 1] = '\0';
		return _last_ch;
		}
		
		// cerca una stringa nel buffer e ritorna la posizione.. -1 se non trova
		int AsyncHTTPClientLight::search_strbuf(const char* buffer, char* str_cmp, int fromwhere) {
		
		int t = fromwhere;
		int x = 0;
		int pos = -1;
		//bool findok = false;
		
		while (str_cmp[x] != '\0' && buffer[t] != '\0') {
		
		do {
		if (buffer[t] != str_cmp[x]) {
		t++;
		pos = -1;
		//findok = false;
		x = 0;
		} else {
		//if(!findok) pos = t;
		if(pos == -1)pos = t;
		//findok = true;
		x++;
		t++;
		break;
		}
		} while (str_cmp[x] != '\0' && buffer[t] != '\0');
		
		}
		
		return pos;
		
		}																																													
