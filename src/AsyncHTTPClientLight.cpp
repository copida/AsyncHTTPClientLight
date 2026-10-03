#include "AsyncHTTPClientLight.h"

#define ASYNC_HTTP_DEBUG 1

#if ASYNC_HTTP_DEBUG
	#pragma message "### AsyncHTTPClientLight: Funzionalita DEBUG ATTIVATE  ###"
	
	#define ASYNC_HTTP_LOG_SD
	//#define ASYNC_HTTP_LOG_SDMMC
	//#define ASYNC_HTTP_LOG_SPIFFS
	//#define ASYNC_HTTP_LOG_LittleFS
	
	#define MAXSIZEFILE_LOG 512000
	#define MAXSIZEBUFFER_LOG 512
	
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
	
	#ifdef ASYNC_HTTP_LOG_SDMMC
		#include <SD_MMC.h>
		#define FS_LOG SDMMC
		#pragma message "### AsyncHTTPClientLight: Funzionalità 'SD MMC' incluse. ###"
	#endif
	
	#ifdef ASYNC_HTTP_LOG_LittleFS
		#include <FS.h>
		#include <LittleFS.h>
		#define FS_LOG LittleFS
		#pragma message "### AsyncHTTPClientLight: Funzionalità 'LittleFS' incluse. ###"
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

void AsyncHTTPClientLight::setmaxRedirects(int ndirect) {
	if (ndirect >= 1) maxRedirects = ndirect;
}


void AsyncHTTPClientLight::setDebug(bool enabled) {
	debugEnabled = enabled;
}

#if ASYNC_HTTP_DEBUG
	
	// 1. Questa è la funzione che hai già convertito prima
	void AsyncHTTPClientLight::log(const char* formato, ...) {
		#define oldLogFile "/old_Log.txt"
		#define logFile "/http_log.txt"
		
		char bufferMessaggio[MAXSIZEBUFFER_LOG];
		
		va_list argomenti;
		va_start(argomenti, formato);
		vsnprintf(bufferMessaggio, sizeof(bufferMessaggio), formato, argomenti);
		va_end(argomenti);
		
		if (debugEnabled) {
			Serial.print(logPrefix);
			Serial.println(bufferMessaggio); 
		}
		
		#if defined(ASYNC_HTTP_LOG_SPIFFS) || defined(ASYNC_HTTP_LOG_SD) || defined(ASYNC_HTTP_LOG_LittleFS)
			if (logToFile) {
				File f = FS_LOG.open(logFile, FILE_APPEND);
				if (f) {
					f.print(logPrefix);
					f.println(bufferMessaggio);
				}
				if (f && f.size() > MAXSIZEFILE_LOG) {
					f.close();
					FS_LOG.remove(oldLogFile);
					FS_LOG.rename(logFile, oldLogFile);
					} else if (f) {
					f.close();
				}
			}
		#endif
	}
	
	// 2. NUOVA: Funzione di supporto per gestire la macro F()
	void AsyncHTTPClientLight::log(const __FlashStringHelper* formato, ...) {
		// Converte il puntatore Flash in un puntatore a caratteri leggibile
		const char* formatoInFlash = (const char*)formato;
		
		char bufferMessaggio[MAXSIZEBUFFER_LOG];
		
		va_list argomenti;
		va_start(argomenti, formato);
		// Nota: Su ESP32 vsnprintf_P gestisce nativamente i puntatori alla Flash
		vsnprintf_P(bufferMessaggio, sizeof(bufferMessaggio), formatoInFlash, argomenti);
		va_end(argomenti);
		
		// Ora che il messaggio è formattato nel buffer, chiama la funzione principale
		// Passiamo direttamente il buffer pronto, così non ripete la scrittura su file
		this->log("%s", bufferMessaggio);
	}
	
#endif

/*
	#if ASYNC_HTTP_DEBUG
	void AsyncHTTPClientLight::log(const String& msg){
	#define oldLogFile "/old_Log.txt"
	#define logFile "/http_log.txt"
	
	
	if (debugEnabled) {
	Serial.print(logPrefix);
	Serial.println(msg); 
	}
	#if defined(ASYNC_HTTP_LOG_SPIFFS) || defined(ASYNC_HTTP_LOG_SD) || defined(ASYNC_HTTP_LOG_LittleFS)
	if (logToFile) {
	
	File f = FS_LOG.open(logFile, FILE_APPEND);
	
	if (f) {
	f.print(logPrefix);
	f.println(msg);
	}
	// Rotazione semplice se supera maxSize
	if (f.size() > MAXSIZEFILE_LOG) {
	f.close();
	FS_LOG.remove(oldLogFile);
	FS_LOG.rename(logFile, oldLogFile);
	//f = FS_LOG.open(logFile, FILE_WRITE);  // Crea nuovo
	}else{
	f.close();
	}
	}
	#endif
	}
	#endif
*/

#if ASYNC_HTTP_DEBUG
	void AsyncHTTPClientLight::setLogToFile(bool enabled) {
		logToFile = enabled;
		
		#if defined(ASYNC_HTTP_LOG_SPIFFS) || defined(ASYNC_HTTP_LOG_SD)
			if (enabled && !FS_LOG.begin(true)) {
				log(F("[HTTP]FS_LOG non inizializzato"));
				}else{
				log(F("[HTTP]FS_LOG inizializzato"));
			}
		#endif
		
	}
#endif


int AsyncHTTPClientLight::getLastHTTPcode() const {
	return response.statusCode;
}


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
  response.statusCode = -1;				// codice ritorno http
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
	
	log(F("Protocollo: %s"), useSSL ? "HTTPS" : "HTTP");
	log(F("Porta: %d"), port);
	log(F("Host: %s"), host);
	log(F("Path: %s"), PATHBUFFER);
	
  return true;
}

void AsyncHTTPClientLight::addTitle(const String& title) {
	snprintf(pendingTitle, sizeof(pendingTitle),"%s", title.c_str());
}

void AsyncHTTPClientLight::readHeader(const String& _header) {
	snprintf(_backHeader, sizeof(_backHeader),"%s", _header.c_str());
}


/**
	* @brief Esegue una richiesta HTTP sincrona.
	* @param url URL completo della richiesta.
	* @param method Metodo HTTP (GET, POST...).
	* @param payload Corpo della richiesta (solo per POST/PUT).
	* @return Codice di stato HTTP ricevuto.
*/
int AsyncHTTPClientLight::runSync(const char* url, const char* method_, const char* payload) {
	// aspetto se eventualmente c'è una richiesta asincrona in corso la porto a termine
	if(!finished){
		log(F("Wait end Asincrona : %s"), response.inprogressTitle);
		log(F("Pending Sincrona : %s"), pendingTitle);
		//}
		
		while(!finished){
			vTaskDelay(pdMS_TO_TICKS(10));
			poll2();
		}
	}
	
	_isSyncMode = true;
	//unsigned long startTime;
	//startTime = millis();
	//lastActivity = millis();
	
	beginRequest(url, method_, payload);
	
	while (!isFinished()) {
		vTaskDelay(pdMS_TO_TICKS(10));
		//delay(100);
		poll2();
	}
	
	// clean
	//client->stop();	// dovrebbe essere gia chiuso verificare
	_isSyncMode = false;
	//finished = true;
	log(F("SYNC CLOSED"));
	return response.statusCode;
}


void AsyncHTTPClientLight::beginRequest(const char* url, const char* method_, const char* payload_) {
	
	if (!isFinished() && redirectCount == 0) {
		log(F("Overload: richiesta già in corso: %s"),response.inprogressTitle);
		triggerEvent(HTTPEventType::Overload, String(pendingTitle));
		pendingTitle[0] = '\0';
		return;
	}
	
	requestCounter++;
	snprintf(logPrefix, sizeof(logPrefix), "[REQ %d]", requestCounter);
	
	if (finished) {	//nuova richiesta
		reset();
		
		snprintf(response.inprogressTitle, sizeof(response.inprogressTitle), "%s", pendingTitle);
		log(F("\n === %s ==="), response.inprogressTitle);
		snprintf(pendingTitle, sizeof(pendingTitle), "%s", "(nessun titolo)");
		
		inprogressHeaders.clear();
		inprogressHeaders = pendingHeaders;
		pendingHeaders.clear();
	}
	
	
	if (!parseURL(url)) {
		finished = true;
		//log("ERRORE URL non valido");
		snprintf(response.msg_error, sizeof(response.msg_error), "ERRORE URL non valido");
		log(response.msg_error);
		if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
		//triggerEvent(HTTPEventType::Error, logPrefix + "URL non valido");
		return;
	}
	
	method = method_;
	
	if (!_isSyncMode && payload_ != nullptr) {
		//Serial.println("payload da allocare");
		// questa funzione serve per fare una copia del payload in spazio memoria allocata in 
		// caso di richieta asincrona e che il payload sia locale alla funzione chiamante
		// perdita del puntatore --  a meno che sia dichiarato globalmente
		size_t len = strlen(payload_);
		if(len > 1){
			
			ptr_Inpayload = (char*)malloc(len + 1);
			if (ptr_Inpayload != nullptr) {
				strcpy(ptr_Inpayload, payload_);
				payloadAllocated = true;
				log(F("memoria allocata "));
				//Serial.print("memoria allocata ");
				//Serial.println(len + 1);
				} else {
				log(F("Errore malloc payload"));
				payloadAllocated = false;
			}
		}
	}
	
	state = CONNECTING;
	
	lastActivity = millis();
	response.restime = lastActivity;
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

void AsyncHTTPClientLight::poll2() {
	
	if (finished || state == IDLE) return;
	
	checktimeout();
	switch (state) {
		case CONNECTING:
		connecting();
		break;
		case SENDING:
		sending();
		break;
		case RECEIVING:
		if (!_isSyncMode) vTaskDelay(pdMS_TO_TICKS(10)); 
		if (client->available()) receiving();
		break;
		default:
		break;
	}
}

// in caso di TASK freeRTOS e richiesta runSync in corso non esegue il poll dall'esterno(loop)
void AsyncHTTPClientLight::poll(){
	if(_isSyncMode)return;
	poll2();
}

void AsyncHTTPClientLight::checktimeout() {
	if (millis() - lastActivity > timeoutMs) {
		log(F("Timeout"));
		snprintf(response.msg_error, sizeof(response.msg_error),"Timeout %d", retryCount);
		if(retryCount <= maxRetries){		// se non supero maxRetries solo messaggio
			triggerEvent(HTTPEventType::Timeout, String(logPrefix) + "Timeout"  + String(retryCount));
		}
		
		//client->stop();
		endclient();
		retryCount++;
		state = CONNECTING;
		lastActivity = millis();
		return;
	}
}

void AsyncHTTPClientLight::connecting() {
	
	if (retryCount > maxRetries){
		
		releasePayload();
		
		snprintf(response.msg_error, sizeof(response.msg_error), "Superato num tentativi: %d", retryCount);
		log(response.msg_error);
		endhttp();
		//if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
		
		return;
	}
	
	responsePayloadBuffer[0] = '\0';
	
	client = useSSL ? &secureClient : &plainClient;
	if (useSSL) secureClient.setInsecure();
	//  log(F("Client creato puntatore: %lu", (uintptr_t)client);
	
	if(redirectCount > 0){
		log(F("REDIRECT.."));
		log(F("Host: %s"), host);
		//log("Path: %s" PATHBUFFER);
	}
	log(F("Tentativo n: %d"), retryCount);
	
	
	if (client->connect(host, port)) {
		state = SENDING;
		lastActivity = millis();
		log(F("Connessione riuscita"));
		return;
		} else {
		//client->stop();
		endclient();
		log(F("Connessione fallita"));
		retryCount++;
		lastActivity = millis();
		state = CONNECTING;
		
	}
}

void AsyncHTTPClientLight::sending() {
	
	int x = 0;
	
	//Serial.println(ptr_Inpayload);
	//Serial.println((unsigned long)&ptr_Inpayload, HEX);
	log(F("Sending.."));
  snprintf(lineBuffer, sizeof(lineBuffer), "%s %s HTTP/1.1\r\n", method, PATHBUFFER);
	//log(lineBuffer);
	log(F("%s"), lineBuffer);
  client->print(lineBuffer);
	
	
  // Host header
  snprintf(lineBuffer, sizeof(lineBuffer), "Host: %s\r\n", host);
	log(lineBuffer);
  client->print(lineBuffer);
	
	//headers
	for (auto& h : inprogressHeaders) {
		//Serial.println("headers trovato");
		log(F("aggiungo headers"));//
		//log(h.first + ": " + h.second + "\r\n");//
		log(F("%s: %s"), h.first, h.second.c_str());//
		client->print(h.first + ": " + h.second + "\r\n");
	}
	
  if(ptr_Inpayload != nullptr && redirectCount == 0){
		
		x = strlen(ptr_Inpayload);
		snprintf(lineBuffer, sizeof(lineBuffer), "Content-Length: %d\r\n", x);
    client->print(lineBuffer);
		client->print("Accept: text/html,application/json\r\n");
	}
	
	client->print("Connection: close\r\n\r\n");
	
	if(x >0) client->print(ptr_Inpayload);
	
	log(F("SHIPPED: %s"), (ptr_Inpayload != nullptr) ? ptr_Inpayload : "done");
	
	inprogressHeaders.clear();		// non devo più spedire gli headers
	state = RECEIVING;
	//lastActivity = millis();
	//client->flush();
}


void AsyncHTTPClientLight::receiving() {
	int len;
	
	// readAll(client, lineBuffer, sizeof(lineBuffer)-1);
	// endhttp();
	// return;
	
	if(!headersParsed){
		
		while (client->available()) {
			lastActivity = millis();
			len = readUntilTerminator(client, lineBuffer, sizeof(lineBuffer)-1, '\n', timeoutMs);
			
			if (len < 0) break;
			
			// Fine degli header
			if (strlen(lineBuffer) > 0){
				parseHeaders();
				if(headersParsed)break;
				} else{
				log(F("Fine header"));
				headersParsed = true;
				_backHeader[0] = '\0';
				break;
			}
		}
		
		// lascio il tempo ad altre funzioni poi ritorno mentre aspetto altri header
		// fino a quando trovo un header vuoto
		if(!headersParsed)return;			
	}
	
	response.msg_error[0] = '\0';		// reset errori precednti
	
	// --- Gestione redirect HTTP --------------------------------------------
	if (response.statusCode >= 300 && response.statusCode <= 308) {
		
		// Svuota velocemente tutti i dati residui nel buffer
		//while (client->available() > 0) {
		//client->read(); // Legge il byte e lo scarta immediatamente, liberando la RAM
		//}
		//client->stop();
		endclient();
		
		headersParsed = false;
		response.contentLength = 0;
		redirectCount++;
		
		if(redirectCount > maxRedirects){
			snprintf(response.msg_error, sizeof(response.msg_error), "Too many redirection");
			log(response.msg_error);
			endhttp();
			//if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
			return;
		}
		
    // Comportamento conforme a RFC 7231
    switch (response.statusCode) {
			case 301:
			case 302:
			releasePayload();   // il vecchio payload non serve più
			method = "GET";
			case 303:
			releasePayload();   // il vecchio payload non serve più
			method = "GET";
			break;
			case 307:
			case 308:
			// Mantiene il metodo e il payload originali
			break;
			default:
			break;
		}
		
    retryCount = 1;	// ricomincio da capo dopo un redirect
    state = CONNECTING;
		
    return;
	}
	
	if(response.isStream) {
		lastActivity = millis();
		
		len = 0;
		len = readStream(responsePayloadBuffer, responsePayloadMaxLen -1, response.contentLength );
		
		if(len != response.contentLength) {
			log(F("Errore dati nello stream"));
			triggerEvent(HTTPEventType::Error, String(logPrefix) + "n. dati non corrispondono");
		} 
		response.isStream = false;	// stop lettura stream
	} 
	
	
	if (response.isChunked){
		if(readChunked()){
			response.isChunked = false;		// stop lettura chunked
		} 
	} 
	
	if(!response.isStream  && !response.isChunked){			//finito
		client->stop();
		endclient();
		releasePayload();
		endhttp();
		//if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
	}
	
}	//end
//-----------------------

int AsyncHTTPClientLight::readStream(char* buffer, int lenbuffer, int ndati ){
	
	int count_ch = 0;
	int readed = 0;
	
	if(ndati == 0)return ndati;
	
	log(F("verranno letti: %d bytes di %d"),(ndati > lenbuffer -1)? lenbuffer -1 : ndati, ndati);
	if(lenbuffer -1 < ndati)triggerEvent(HTTPEventType::Error, String(logPrefix) + "Buffer too small " + String(lenbuffer -1) + " need " + String(ndati));
	
	while (millis() - lastActivity < timeoutMs) {
		
		if (client->available()) {
			char c = client->read();
			readed++;
			#if ASYNC_HTTP_DEBUG
				if(c != '\r' && c != '\n') Serial.print(c);
				if (c == '\r')Serial.print("CR");
				if (c == '\n')Serial.println("LF");
			#endif
			if(count_ch < lenbuffer-1)buffer[count_ch++] = c;
			if (readed == ndati)break;
		}
		
	}
	Serial.println();
	buffer[count_ch] = '\0';
	response.expectedLength = readed;
	log(F("Letti n. bytes: %d"), readed);
	return readed;
	
	}/*
	//---------------------------------------
	// ritorna vero se lettura chunk finita
	bool AsyncHTTPClientLight::readChunked(){
	
	bool ok_chunk = false;
	//bool endchunk = false;
	
	//log("lung. buffer:" + String(responsePayloadMaxLen));
	while (millis() - lastActivity < timeoutMs) {
	
	while (client->available()) {
	char c = client->read();
	if(c == '\n' && WORKBUFFER[bufIndex-1] == '\r'){
	WORKBUFFER[bufIndex -1] = '\0';
	ok_chunk = true;
	break;
	}else{
	WORKBUFFER[bufIndex++] = c;
	// se buffer pieno...
	if(bufIndex == sizeof(WORKBUFFER)-1){
	
	if(WORKBUFFER[bufIndex-1] == '\r') {
	bufIndex--;
	}
	
	WORKBUFFER[bufIndex] = '\0';
	ok_chunk = true;
	break;
	}
	}
	}
	
	//if(!ok_chunk)return ok_chunk;
	if(!ok_chunk)return false;
	
	switch (chunkState) {
	case WAITING_SIZE:
	response.expectedLength = (int)strtol(WORKBUFFER, nullptr, 16);
	//Serial.printf("Exp. lunghezza chunk: (%d)\r\n", response.expectedLength);//
	if (response.expectedLength == 0) {
	chunkState = SKIPPING_CRLF;
	} else {
	chunkState = READING_DATA;
	}
	
	break;
	case READING_DATA:
	//response.contentLength += strlen(WORKBUFFER);
	
	_lenchunk += strlen(WORKBUFFER);
	
	if(bufIndex == sizeof(WORKBUFFER) -1){
	log("Portion Chunk:\n" + String(WORKBUFFER));		
	}else{
	// riga chunk completa
	if(_lenchunk != response.expectedLength){
	log("sizeChunk:\n" + String(_offset) + "/" + String(strlen(WORKBUFFER)) +"/" + String(response.expectedLength));
	triggerEvent(HTTPEventType::Error, logPrefix + "Chunk imperfect\n");
	} 
	log("Chunk:\n" + String(WORKBUFFER));
	_lenchunk = 0;
	chunkState = WAITING_SIZE;
	}
	
	if (unifiedCallback) unifiedCallback(HTTPEventType::Chunk, &response);
	
	if (_offset < responsePayloadMaxLen - 1) {
	_offset += snprintf(&responsePayloadBuffer[_offset], responsePayloadMaxLen - _offset, "%s", WORKBUFFER);
	} else {
	triggerEvent(HTTPEventType::Error, logPrefix + "Buffer too small");
	}
	
	break;
	case SKIPPING_CRLF:
	//endchunk = true;
	chunkState = FINISHED;
	log("Chunked Finished");
	return true;
	break;
	
	default:
	break;
	}
	
	bufIndex = 0;
	if())
	return false;		// processo chunk non ancora finito ..ritornero'
	}
	// superato timeout
	log("Timeout Chunk:\n");
	return true;		// interrompo chunk
	}
*/


bool AsyncHTTPClientLight::readChunked() {
	
	if (client->available()) {
		lastActivity = millis(); // <--- Aggiornato UNA SOLA VOLTA per questa esecuzione!
	}
	
	while (client->available()) {
		
		switch (chunkState) {
			
			case WAITING_SIZE: {
				// In questo stato cerchiamo la riga del valore esadecimale
				char c = client->read();
				
				if (c == '\n' && bufIndex > 0 && WORKBUFFER[bufIndex-1] == '\r') {
					WORKBUFFER[bufIndex-1] = '\0'; // Taglia il \r
					response.expectedLength = (int)strtol(WORKBUFFER, nullptr, 16);
					totChunk += response.expectedLength;
					
					bufIndex = 0; // Resetta il buffer di lavoro
					_lenchunk = 0; // Resetta il contatore dei byte letti per il prossimo stato
					
					if (response.expectedLength == 0) {
						chunkState = SKIPPING_CRLF; // Fine della trasmissione
						} else {
						chunkState = READING_DATA;
					}
					} else {
					// Accumula i caratteri della dimensione esadecimale
					if (bufIndex < sizeof(WORKBUFFER) - 1) {
						WORKBUFFER[bufIndex++] = c;
					}
				}
				break;
			}
			
			case READING_DATA: {
				// STATO UNIVERSALE: Leggiamo i byte contando, senza guardare cosa c'è dentro!
				char c = client->read();
				_lenchunk++; // Incrementiamo il contatore dei byte reali estratti
				
				if (_offset < responsePayloadMaxLen - 1) {
					responsePayloadBuffer[_offset++] = c;
					responsePayloadBuffer[_offset] = '\0'; // Mantieni la stringa terminata
					//if(_offset >= responsePayloadMaxLen -1 )triggerEvent(HTTPEventType::Error, logPrefix + "Buffer too small");
					response.contentLength = _offset;
				}
				
				// 2. Abbiamo estratto tutti i byte promessi da questo chunk?
				if (_lenchunk == response.expectedLength) {
					//totChunk += _lenchunk;
					if (unifiedCallback) unifiedCallback(HTTPEventType::Chunk, &response);
					//chunkState = SKIPPING_CHUNK_CRLF; // Passiamo a scartare il \r\n di cortesia
					chunkState = SKIPPING_CRLF; // Passiamo a scartare il \r\n di cortesia
				}
				break;
			}
			
			//case SKIPPING_CHUNK_CRLF: {
			// Svuota i due caratteri di controllo (\r\n) che separano i chunk
			//	char c = client->read();
			// Aspettiamo il \n che chiude la sequenza di cortesia
			//	if (c == '\n') {
			//		chunkState = WAITING_SIZE; // Canale pulito, aspettiamo il prossimo chunk
			//	}
			//	break;
			//}
			
			case SKIPPING_CRLF: {
				char c = client->read();
				if (c == '\n') {
					if (response.expectedLength == 0){
						// Svuota eventuali dati rimasti nel buffer di lettura prima di chiudere
						while (client->available() > 0) {
							client->read();
						}
						chunkState = FINISHED;
						//log("Chunked Finished letti");
						if(_offset < totChunk)triggerEvent(HTTPEventType::Error, String(logPrefix) + "Buffer too small " + String(_offset) + " need " + String(totChunk));
						log(F("Chunked Finished letti: %d bytes di %d"), _offset, totChunk);
						return true; // Trasmissione completata con successo!
						}else{
						chunkState = WAITING_SIZE; // Canale pulito, aspettiamo il prossimo chunk
					}
				}
				break;
			}
			
			default:
			break;
		}
	}
	
	if (chunkState == FINISHED) return true;
	
	if (millis() - lastActivity >= timeoutMs) {
		log(F("Timeout Chunk"));
		return true; // Interrompe per timeout
	}
	
	return false; 
}

//------------------------------------
void AsyncHTTPClientLight::parseHeaders(){
	
	// modifica per header Reporting-Endpoints ..che non serve
	
	if (search_strbuf(lineBuffer, "Reporting-Endpoints:") == 0) {
		// taglio header per risparmiare byte e uso di string e di log
		// non serve 
		lineBuffer[21] = '\0';
	}
	
	// END MODIFICA
	
	
	log(F("header: %s"), lineBuffer);
	
	if(_backHeader[0] != '\0'){
		if (search_strbuf(lineBuffer, _backHeader) == 0) {
			log(F("trovato HEADER: %s"),lineBuffer);
			snprintf(response.backHeader, sizeof(response.backHeader), "%s", lineBuffer);
			_backHeader[0] = '\0';
		}
	}
	
	
	if (search_strbuf(lineBuffer, "HTTP/") == 0) {
		int space = search_strbuf(lineBuffer, " ");
		response.statusCode = atoi(&lineBuffer[space + 1]);
		log(F("Status code: %d"), response.statusCode);
		return;
	}
	//transfer-encoding:
	if (search_strbuf(lineBuffer, "Transfer-Encoding:") == 0 && search_strbuf(lineBuffer, "chunked") != -1) {
		response.isChunked = true;
		_offset = 0;
		_lenchunk = 0;
		totChunk = 0;
		chunkState = WAITING_SIZE;
		lastActivity = millis();
		log(F("Chunked encoding rilevato"));
		return;
	}
	
	if (search_strbuf(lineBuffer, "Location:") == 0 && (response.statusCode == 301 || response.statusCode == 302 || response.statusCode == 307)) {
		trimmer(lineBuffer, 9);
		if(lineBuffer[0] == '/'){					// PATHBUFFER relativo
			snprintf(PATHBUFFER, sizeof(PATHBUFFER),"%s", lineBuffer);
			}else{
			parseURL(lineBuffer);
		}
		headersParsed = true;		// interrompo parseheaders e faccio subito redirect
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
	
	while (millis() - start < timeoutMs) {
		if (client->available()) {
			char c = client->read();
			isdati = true;
			if (c == '\r'){
				if (delCR)continue;	// ignora il carriage return
			}
			if (c == terminator) break;	
			if(index < maxLen - 1){		// elimino tutti i dati in eccesso
				buffer[index++] = c;
			}
		}
	}
	
	buffer[index] = '\0';
	
	if (index == 0 && millis() - start >= timeoutMs && !isdati) {
		return -1; // timeout senza dati
	}
	
	return index; // numero di caratteri letti
}
//------------------------------------
void AsyncHTTPClientLight::readAll(Stream* client, char* buffer, size_t maxLen) {
	size_t index = 0;
	unsigned long start = millis();
	
	while (millis() - start < timeoutMs) {
		if (client->available()) {
			char c = client->read();
			buffer[index++] = c;
			if(c == '\n' || index == maxLen - 1) {
				buffer[index] = '\0';
				log(buffer);
				index = 0;
			}
		}
	}
	
	if(index > 0){
		buffer[index +1] = '\0';
		log(buffer);
	}		
	
	log("END ALL");
	
}
//-----------------------------------
void AsyncHTTPClientLight::releasePayload() {
	
	if(!payloadAllocated) return;
	if (payloadAllocated && ptr_Inpayload) {
		free(ptr_Inpayload);
		ptr_Inpayload = nullptr;
		payloadAllocated = false;
		log(F("Payload liberato"));
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
int AsyncHTTPClientLight::search_strbuf(const char* buffer, char* word_cmp, int fromwhere) {
	
	if (word_cmp[0] == '\0') return fromwhere;
	
	int t = fromwhere;
	int x = 0;
	int pos = -1;
	
	while (buffer[t] != '\0') {
		//if (buffer[t] != word_cmp[x]) {
		if (tolower((unsigned char)buffer[t]) != tolower((unsigned char)word_cmp[x])) {
			if(pos != -1) t = pos;
			t++;
			pos = -1;
			x = 0;
			} else {
			if (pos == -1) pos = t;
			x++;
			t++;
			if (word_cmp[x] == '\0') return pos;
		}
	}
	return -1;
}

void AsyncHTTPClientLight::endhttp(){
	response.restime = (millis() - response.restime);
	log(F("Tempo: %lu"), response.restime);
	finished = true;
	if (unifiedCallback) unifiedCallback(HTTPEventType::Response, &response);
	state = IDLE;
}
void AsyncHTTPClientLight::endclient(){
	// Svuota velocemente tutti i dati residui nel buffer
	while (client->available() > 0) {
		client->read(); // Legge il byte e lo scarta immediatamente, liberando la RAM
	}
	client->stop();
}
