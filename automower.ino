#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <ESP_EEPROM.h>

/*__________________________________________________________CONSTANTS__________________________________________________________*/

// The neatest way to access variables stored in EEPROM is using a structure
struct saveStruct {
  uint16_t  timerManMode;
} eepromVar1;

ESP8266WiFiMulti wifiMulti;             // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);            // create a web server on port 80
WebSocketsServer webSocket(81);         // create a websocket server on port 81

File fsUploadFile;                      // a File variable to temporarily store the received file

const char *ssid = "automower";         // The name of the Wi-Fi network that will be created
const char *password = "robertsson";    // The password required to connect to it, leave blank for an open network

const char *OTAName = "automower";        // A name and a password for the OTA service
const char *OTAPassword = "automower";

#define LED_RED     15            // specify the pins with an RGB LED connected
#define LED_GREEN   12
#define LED_BLUE    2

const char* mdnsName = "automower";     // Domain name for the mDNS responder

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  //pinMode(LED_RED, OUTPUT);    // the pins with LEDs connected are outputs
  //pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, 0);
  
  Serial.begin(74880);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  loadEEPROM();                // Fetch setup values
  
  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startOTA();                  // Start the OTA service
  
  startLittleFS();             // Start the LittleFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
  
  digitalWrite(LED_BLUE, 1);
}

/*__________________________________________________________LOOP__________________________________________________________*/
unsigned long prevMillis = millis();
int hue = 0;

void loop() {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
}

struct allcommands {
  uint8_t R_STATUS[5] = {0xf,0x1,0xf1,0x0,0x0};
  uint8_t R_SEKUNDE[5] = {0xf,0x36,0xb1,0x0,0x0};
  uint8_t R_MINUTE[5] = {0xf,0x36,0xb3,0x0,0x0};
  uint8_t R_STUNDE[5] = {0xf,0x36,0xb5,0x0,0x0};
  uint8_t R_TAG[5] = {0xf,0x36,0xb7,0x0,0x0};
  uint8_t R_MONAT[5] = {0xf,0x36,0xb9,0x0,0x0};
  uint8_t R_JAHR[5] = {0xf,0x36,0xbd,0x0,0x0};
  uint8_t R_TIMERSTATUS[5] = {0xf,0x4a,0x4e,0x0,0x0};
  uint8_t R_WOCHEN_TIMER1_START_STD[5] = {0xf,0x4a,0x38,0x0,0x0};
  uint8_t R_WOCHEN_TIMER1_START_MIN[5] = {0xf,0x4a,0x39,0x0,0x0};
  uint8_t R_WOCHEN_TIMER1_STOP_STD[5] = {0xf,0x4a,0x3a,0x0,0x0};
  uint8_t R_WOCHEN_TIMER1_STOP_MIN[5] = {0xf,0x4a,0x3b,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER1_START_STD[5] = {0xf,0x4a,0x3c,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER1_START_MIN[5] = {0xf,0x4a,0x3d,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER1_STOP_STD[5] = {0xf,0x4a,0x3e,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER1_STOP_MIN[5] = {0xf,0x4a,0x3f,0x0,0x0};
  uint8_t R_WOCHEN_TIMER2_START_STD[5] = {0xf,0x4a,0x40,0x0,0x0};
  uint8_t R_WOCHEN_TIMER2_START_MIN[5] = {0xf,0x4a,0x41,0x0,0x0};
  uint8_t R_WOCHEN_TIMER2_STOP_STD[5] = {0xf,0x4a,0x42,0x0,0x0};
  uint8_t R_WOCHEN_TIMER2_STOP_MIN[5] = {0xf,0x4a,0x43,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER2_START_STD[5] = {0xf,0x4a,0x44,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER2_START_MIN[5] = {0xf,0x4a,0x45,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER2_STOP_STD[5] = {0xf,0x4a,0x46,0x0,0x0};
  uint8_t R_WOCHENEND_TIMER2_STOP_MIN[5] = {0xf,0x4a,0x47,0x0,0x0};
  uint8_t R_TIMER_TAGE[5] = {0xf,0x4a,0x50,0x0,0x0};
  uint8_t R_MAEHZEIT[5] = {0xf,0x0,0x38,0x0,0x0};
  uint8_t R_VIERECKMODUS_STATUS[5] = {0xf,0x1,0x38,0x0,0x0};
  uint8_t R_VIERECKMODUS_PROZENT[5] = {0xf,0x1,0x34,0x0,0x0};
  uint8_t R_VIERECKMODUS_REFERENZ[5] = {0xf,0x1,0x37,0x0,0x0};
  uint8_t R_AKKU_LADEZEIT_MIN[5] = {0xf,0x1,0xec,0x0,0x0};
  uint8_t R_AKKU_KAPAZITAET_MA[5] = {0xf,0x1,0xeb,0x0,0x0};
  uint8_t R_AKKU_KAPAZITAET_MAH[5] = {0xf,0x1,0xef,0x0,0x0};
  uint8_t R_AKKU_KAPAZITAET_SUCHSTART_MAH[5] = {0xf,0x1,0xf0,0x0,0x0};
  uint8_t R_AKKU_KAPAZITAET_GENUTZT_MAH[5] = {0xf,0x2e,0xe0,0x0,0x0};
  uint8_t R_AKKU_SPANNUNG_MV[5] = {0xf,0x2e,0xf4,0x0,0x0};
  uint8_t R_AKKU_TEMPERATUR_AKTUELL[5] = {0xf,0x2,0x33,0x0,0x0};
  uint8_t R_AKKU_TEMPERATUR_LADEN[5] = {0xf,0x2,0x35,0x0,0x0};
  uint8_t R_AKKU_LETZTER_LADEVORGANG_MIN[5] = {0xf,0x2,0x34,0x0,0x0};
  uint8_t R_AKKU_NAECHSTE_TEMPERATURMESSUNG_SEK[5] = {0xf,0x2,0x36,0x0,0x0};
  uint8_t R_GESCHWINDIGKEIT_MESSERMOTOR[5] = {0xf,0x2e,0xea,0x0,0x0};
  uint8_t R_GESCHWINDIGKEIT_RECHTS[5] = {0xf,0x24,0xbf,0x0,0x0};
  uint8_t R_GESCHWINDIGKEIT_LINKS[5] = {0xf,0x24,0xc0,0x0,0x0};
  uint8_t R_FIRMWARE_VERSION[5] = {0xf,0x33,0x90,0x0,0x0};
  uint8_t R_SPRACHDATEI_VERSION[5] = {0xf,0x3a,0xc0,0x0,0x0};
  uint8_t W_TIMERAKTIV[5] = {0xf,0xca,0x4e,0x0,0x0,};
  uint8_t W_TIMERINAKTIV[5] = {0xf,0xca,0x4e,0x0,0x1,};
  uint8_t W_MODE_HOME[5] = {0xf,0x81,0x2c,0x0,0x3,};
  uint8_t W_MODE_MAN[5] = {0xf,0x81,0x2c,0x0,0x0,};
  uint8_t W_MODE_AUTO[5] = {0xf,0x81,0x2c,0x0,0x1,};
  uint8_t W_MODE_DEMO[5] = {0xf,0x81,0x2c,0x0,0x4,};
  uint8_t W_KEY_0[5] = {0xf,0x80,0x5f,0x0,0x0,};
  uint8_t W_KEY_1[5] = {0xf,0x80,0x5f,0x0,0x1,};
  uint8_t W_KEY_2[5] = {0xf,0x80,0x5f,0x0,0x2,};
  uint8_t W_KEY_3[5] = {0xf,0x80,0x5f,0x0,0x3,};
  uint8_t W_KEY_4[5] = {0xf,0x80,0x5f,0x0,0x4,};
  uint8_t W_KEY_5[5] = {0xf,0x80,0x5f,0x0,0x5,};
  uint8_t W_KEY_6[5] = {0xf,0x80,0x5f,0x0,0x6,};
  uint8_t W_KEY_7[5] = {0xf,0x80,0x5f,0x0,0x7,};
  uint8_t W_KEY_8[5] = {0xf,0x80,0x5f,0x0,0x8,};
  uint8_t W_KEY_9[5] = {0xf,0x80,0x5f,0x0,0x9,};
  uint8_t W_PRG_A[5] = {0xf,0x80,0x5f,0x0,0xa,};
  uint8_t W_PRG_B[5] = {0xf,0x80,0x5f,0x0,0xb,};
  uint8_t W_PRG_C[5] = {0xf,0x80,0x5f,0x0,0xc,};
  uint8_t W_KEY_HOME[5] = {0xf,0x80,0x5f,0x0,0xd,};
  uint8_t W_KEY_MANAUTO[5] = {0xf,0x80,0x5f,0x0,0xe,};
  uint8_t W_KEY_C[5] = {0xf,0x80,0x5f,0x0,0xf,};
  uint8_t W_KEY_UP[5] = {0xf,0x80,0x5f,0x0,0x10,};
  uint8_t W_KEY_DOWN[5] = {0xf,0x80,0x5f,0x0,0x11,};
  uint8_t W_KEY_YES[5] = {0xf,0x80,0x5f,0x0,0x12,};
} commands;

/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void loadEEPROM() {
  EEPROM.begin(sizeof(saveStruct));
  
  if(EEPROM.percentUsed()>=0) {
    EEPROM.get(0, eepromVar1);
    eepromVar1.timerManMode = (eepromVar1.timerManMode > 270) ? 180 : eepromVar1.timerManMode;
    //eepromVar1.timerManMode;     // make a change to our copy of the EEPROM data
    Serial.println("EEPROM has data from a previous run.");
    Serial.print(EEPROM.percentUsed());
    Serial.println("% of ESP flash space currently used");
  } else {
    Serial.println("EEPROM size changed - EEPROM data zeroed - commit() to make permanent");
    eepromVar1.timerManMode = 180;  // Set default value.
    EEPROM.put(0, eepromVar1);
    bool ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
    EEPROM.get(0, eepromVar1);
  }
  Serial.printf("Readback data: %u\n", eepromVar1.timerManMode);
}

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("Printerzone", "zmoddans");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("ddwrt", "RobertssoN1337");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  //ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    digitalWrite(LED_BLUE, 1); // turn off the LEDs
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startLittleFS() { // Start the LittleFS and list all contents
  LittleFS.begin();                             // Start the SPI Flash File System (LittleFS)
  Serial.println("LittleFS started. Contents:");
  {
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (LittleFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = LittleFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the LittleFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(LittleFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         LittleFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = LittleFS.open(path, "w");            // Open the file for writing in LittleFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        String sendStr = "timer:" + String(eepromVar1.timerManMode, DEC);
        webSocket.sendTXT(num, sendStr);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
        if (payload[0] == 's') {                      // save data to EEPROM
          eepromVar1.timerManMode = (uint16_t) strtol((const char *) &payload[1], NULL, 10);   // decode timerManMode data
          Serial.printf("Timer Received: %d\n", eepromVar1.timerManMode);
          EEPROM.put(0, eepromVar1);
          bool ok = EEPROM.commit();
          Serial.println((ok) ? "Save OK" : "Commit failed");
          String s = ok ? "SaveOk" : "SaveFail";
          webSocket.sendTXT(num, s);
        }
      break;
  }
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
