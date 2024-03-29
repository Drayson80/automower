
/*
  AUTOMOWER - A automower addon for your 2nd generation automower.
  Designed to make use of your new battery in a better way.

  Requirements:
  Install ESP8266 Core
  In Arduino set up Lolin (Wemos) D1 mini pro as board to get correct pin definitions.
  ESP_EEPROM by j-watson
  Websockets by Markus Sattler
  
  Serial 0 (pin labeled RX) - RX port for receiving data.
  Serial 1 (pin labeled D4) - TX port for sending data.
  
  Connections
  D1 Mini Pro     ->      Robotic lawnmower
  RX              ->      Tx
  D4              ->      Rx
  3v3             ->      3v3
  GND             ->      GND

   
 * 
  FSBrowser - A web-based FileSystem Browser for ESP8266 filesystems

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  See readme.md for more information.
*/
////////////////////////////////

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <ESP_EEPROM.h>
#include <Ticker.h>
//#include <SoftwareSerial.h>
//#include <ESP8266HTTPUpdateServer.h>

// RTC MEMORY
extern "C" {
#include "user_interface.h"
}

typedef struct {
  uint8_t count;
} rtcStore;

rtcStore rtcMem;

#define RTCMEMORYSTART 65
#define MAXHOUR 5 // number of hours to deep sleep for

////////////////////////////////
//  SETUP             
                                          // Will start a accespoint (AP) when not connected to a Wi-Fi.
#define STASSID "amw"          // AP Name
#define STAPSK  "automower"               // AP Password (NOT USED NOW, no password)
const char* host = "am-update";           // HTTP update hostname
const char *ssid_ap1 = "Printerzone";
const char *ssid_pass_ap1 = "zmoddans";
const char *ssid_ap2 = "dd-wrt";
const char *ssid_pass_ap2 = "Robertsson145";
const char *ssid_ap3 = "dd-wrt_RPT";
const char *ssid_pass_ap3 = "Robertsson145";

#define LOG_ENABLE
#ifdef LOG_ENABLE
  #define _LOG(a) strcat(logStr, a)
  //#define _LOG(a) strcat(strcat(logStr, "\r"), a)
  //#define _LOG(a) memset(tempStr, 0, sizeof tempStr); strcat(tempStr, a); strcat(tempStr, "\r"); strcat(logStr, tempStr);
#else
  #define _LOG(a)
#endif

// Sw Serial
//SoftwareSerial swSer1;

// Set send and receive serial port.
#define SEND_PORT Serial1
#define RCV_PORT Serial

// Debug outputs on/off
#define DEBUG_THIS

#define DBG_OUTPUT_PORT Serial // Still needed for printf

#ifdef DEBUG_THIS
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif


////////////////////////////////
// GLOBAL and constructors
const char *OTAName = "amwOTA";        // A name and a password for the OTA service
const char *OTAPassword = "automower";
const char *mdnsName = "automower";       // Domain name for the mDNS responder
const char *ssid = STASSID;
const char *password = STAPSK;

unsigned long tSendCmd = 0;           // millis() for latest request.

// The neatest way to access variables stored in EEPROM is using a structure
struct saveStruct {
  uint16_t  timerManMode;
} eepromVar1;

char logPath[] = "/logs/logfile.txt";

uint8_t statusAutomower[6];             // Temporary storage
char logStr[500], tempStr[50];          // -||-

LittleFSConfig fileSystemConfig = LittleFSConfig();

Ticker sendCommand, readBuffer, wrteLog, getTime; // Tasks

ESP8266WiFiMulti wifiMulti;             // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);            // create a instance of webserver on port 80
WebSocketsServer webSocket(81);         // create a websocket server on port 81
//ESP8266HTTPUpdateServer httpUpdater;    // http update server

uint8_t num_websock = 0; // IP to send WIFI-scan to over websocket

String unsupportedFiles = String();
File uploadFile;                        // Global file handle
File f;                                 

// Statics
#define UPDATE_TIME 24*3600*1000        // How often should we update the time in milliseconds
#define LED_BLUE 2                      // shared with Serial1 Tx specify the pin with LED

#define LOG_READ_BYTES 0                // Constants to call writeToLogBuffer
#define LOG_SEND_BYTES 1                    

static bool fsOK;
static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

// Data structures

// Generic struct for data
// Can hold two bytes of data
// and time for latest update.
struct genDataStruct {
  unsigned long t;
  int32_t data = -1;
} ;

// Time struct
// Holds latest time and what value internal clock ( millis() )
// was latest synched at.
struct mowTime {
  unsigned long t = 0;
  int8_t hour = -1;
  int8_t minute = -1;
  int8_t seconds = -1;
} ;

// Mower status as known
struct mowDataStruct {
  mowTime mowClock;
  genDataStruct stat;
  genDataStruct actCutTime;
  genDataStruct actMode;
} mow;


struct allcommands {
  uint8_t R_STATUS[5] = {0xf,0x1,0xf1,0x0,0x0};
  uint8_t R_SEKUNDE[5] = {0xf,0x36,0xb1,0x0,0x0};
  uint8_t R_MINUTE[5] = {0xf,0x36,0xb3,0x0,0x0};
  uint8_t R_STUNDE[5] = {0xf,0x36,0xb5,0x0,0x0};
  uint8_t R_TAG[5] = {0xf,0x36,0xb7,0x0,0x0};
  uint8_t R_MONAT[5] = {0xf,0x36,0xb9,0x0,0x0};
  uint8_t R_JAHR[5] = {0xf,0x36,0xbd,0x0,0x0};
  uint8_t R_TIMERSTATUS[5l] = {0xf,0x4a,0x4e,0x0,0x0};
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
  uint8_t W_TIMERAKTIV[5] = {0xf,0xca,0x4e,0x0,0x0};
  uint8_t W_TIMERINAKTIV[5] = {0xf,0xca,0x4e,0x0,0x1};
  uint8_t W_MODE_HOME[5] = {0xf,0x81,0x2c,0x0,0x3};
  uint8_t W_MODE_MAN[5] = {0xf,0x81,0x2c,0x0,0x0};
  uint8_t W_MODE_AUTO[5] = {0xf,0x81,0x2c,0x0,0x1};
  uint8_t W_MODE_DEMO[5] = {0xf,0x81,0x2c,0x0,0x4};
  uint8_t W_KEY_0[5] = {0xf,0x80,0x5f,0x0,0x0};
  uint8_t W_KEY_1[5] = {0xf,0x80,0x5f,0x0,0x1};
  uint8_t W_KEY_2[5] = {0xf,0x80,0x5f,0x0,0x2};
  uint8_t W_KEY_3[5] = {0xf,0x80,0x5f,0x0,0x3};
  uint8_t W_KEY_4[5] = {0xf,0x80,0x5f,0x0,0x4};
  uint8_t W_KEY_5[5] = {0xf,0x80,0x5f,0x0,0x5};
  uint8_t W_KEY_6[5] = {0xf,0x80,0x5f,0x0,0x6};
  uint8_t W_KEY_7[5] = {0xf,0x80,0x5f,0x0,0x7};
  uint8_t W_KEY_8[5] = {0xf,0x80,0x5f,0x0,0x8};
  uint8_t W_KEY_9[5] = {0xf,0x80,0x5f,0x0,0x9};
  uint8_t W_PRG_A[5] = {0xf,0x80,0x5f,0x0,0xa};
  uint8_t W_PRG_B[5] = {0xf,0x80,0x5f,0x0,0xb};
  uint8_t W_PRG_C[5] = {0xf,0x80,0x5f,0x0,0xc};
  uint8_t W_KEY_HOME[5] = {0xf,0x80,0x5f,0x0,0xd};
  uint8_t W_KEY_MANAUTO[5] = {0xf,0x80,0x5f,0x0,0xe};
  uint8_t W_KEY_C[5] = {0xf,0x80,0x5f,0x0,0xf};
  uint8_t W_KEY_UP[5] = {0xf,0x80,0x5f,0x0,0x10};
  uint8_t W_KEY_DOWN[5] = {0xf,0x80,0x5f,0x0,0x11};
  uint8_t W_KEY_YES[5] = {0xf,0x80,0x5f,0x0,0x12};
} commands;

// Message structure.
struct msgStruct {
  unsigned long tMsg;
  uint8_t data[6]="";
} ;

struct fifo {
  msgStruct msg[10];
  uint8_t n = 0;      // How many items
} fifoReq;

///////////////////
void readFromRTCMemory() {
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));

  //DEBUG_PRINT("count = ");
  //DEBUG_PRINTLN(rtcMem.count);
  yield();
}

void writeToRTCMemory(uint8_t cnt) {
  rtcMem.count = cnt;
  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));

  //DEBUG_PRINT("count = ");
  //DEBUG_PRINTLN(rtcMem.count);
  yield();
}

////////////////////////////////
// Utils to return HTTP codes, and determine content-type

void replyOK() {
  server.send(200, FPSTR(TEXT_PLAIN), "");
}

void replyOKWithMsg(String msg) {
  server.send(200, FPSTR(TEXT_PLAIN), msg);
}

void replyNotFound(String msg) {
  server.send(404, FPSTR(TEXT_PLAIN), msg);
}

void replyBadRequest(String msg) {
  DEBUG_PRINTLN(msg);
  server.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void replyServerError(String msg) {
  DEBUG_PRINTLN(msg);
  server.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

////////////////////////////////
// Request handlers

/*
   Return the FS type, status and size info
*/
void handleStatus() {
  DEBUG_PRINTLN("handleStatus");
  FSInfo fs_info;
  String json;
  json.reserve(128);

  json = "{\"type\":\"";
  json += "LittleFS";
  json += "\", \"isOk\":";
  if (fsOK) {
    LittleFS.info(fs_info);
    json += F("\"true\", \"totalBytes\":\"");
    json += fs_info.totalBytes;
    json += F("\", \"usedBytes\":\"");
    json += fs_info.usedBytes;
    json += "\"";
  } else {
    json += "\"false\"";
  }
  json += F(",\"unsupportedFiles\":\"");
  json += unsupportedFiles;
  json += "\"}";

  server.send(200, "application/json", json);
}


/*
   Return the list of files in the directory specified by the "dir" query string parameter.
   Also demonstrates the use of chuncked responses.
*/
void handleFileList() {
  if (!fsOK) {
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  if (!server.hasArg("dir")) {
    return replyBadRequest(F("DIR ARG MISSING"));
  }

  String path = server.arg("dir");
  if (path != "/" && !LittleFS.exists(path)) {
    return replyBadRequest("BAD PATH");
  }

  DEBUG_PRINTLN(String("handleFileList: ") + path);
  Dir dir = LittleFS.openDir(path);
  path.clear();

  // use HTTP/1.1 Chunked response to avoid building a huge temporary string
  if (!server.chunkedResponseModeStart(200, "text/json")) {
    server.send(505, F("text/html"), F("HTTP1.1 required"));
    return;
  }

  // use the same string for every line
  String output;
  output.reserve(64);
  while (dir.next()) {
    if (output.length()) {
      // send string from previous iteration
      // as an HTTP chunk
      server.sendContent(output);
      output = ',';
    } else {
      output = '[';
    }

    output += "{\"type\":\"";
    if (dir.isDirectory()) {
      output += "dir";
    } else {
      output += F("file\",\"size\":\"");
      output += dir.fileSize();
    }

    output += F("\",\"name\":\"");
    // Always return names without leading "/"
    if (dir.fileName()[0] == '/') {
      output += &(dir.fileName()[1]);
    } else {
      output += dir.fileName();
    }

    output += "\"}";
  }

  // send last string
  output += "]";
  server.sendContent(output);
  server.chunkedResponseFinalize();
}


/*
   Read the given file from the filesystem and stream it back to the client
*/
bool handleFileRead(String path) {
  DEBUG_PRINTLN(String("handleFileRead: ") + path);
  if (!fsOK) {
    replyServerError(FPSTR(FS_INIT_ERROR));
    return true;
  }

  if (path.endsWith("/")) {
    path += "index.htm";
  }

  String contentType;
  if (server.hasArg("download")) {
    contentType = F("application/octet-stream");
  } else {
    contentType = mime::getContentType(path);
  }

  if (!LittleFS.exists(path)) {
    // File not found, try gzip version
    path = path + ".gz";
  }
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (server.streamFile(file, contentType) != file.size()) {
      DEBUG_PRINTLN("Sent less data than expected!");
    }
    file.close();
    return true;
  }

  return false;
}

/*
   As some FS (e.g. LittleFS) delete the parent folder when the last child has been removed,
   return the path of the closest parent still existing
*/
String lastExistingParent(String path) {
  while (!path.isEmpty() && !LittleFS.exists(path)) {
    if (path.lastIndexOf('/') > 0) {
      path = path.substring(0, path.lastIndexOf('/'));
    } else {
      path = String();  // No slash => the top folder does not exist
    }
  }
  DEBUG_PRINTLN(String("Last existing parent: ") + path);
  return path;
}

/*
   Handle the creation/rename of a new file
   Operation      | req.responseText
   ---------------+--------------------------------------------------------------
   Create file    | parent of created file
   Create folder  | parent of created folder
   Rename file    | parent of source file
   Move file      | parent of source file, or remaining ancestor
   Rename folder  | parent of source folder
   Move folder    | parent of source folder, or remaining ancestor
*/
void handleFileCreate() {
  if (!fsOK) {
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  String path = server.arg("path");
  if (path.isEmpty()) {
    return replyBadRequest(F("PATH ARG MISSING"));
  }

  if (path == "/") {
    return replyBadRequest("BAD PATH");
  }
  if (LittleFS.exists(path)) {
    return replyBadRequest(F("PATH FILE EXISTS"));
  }

  String src = server.arg("src");
  if (src.isEmpty()) {
    // No source specified: creation
    DEBUG_PRINTLN(String("handleFileCreate: ") + path);
    if (path.endsWith("/")) {
      // Create a folder
      path.remove(path.length() - 1);
      if (!LittleFS.mkdir(path)) {
        return replyServerError(F("MKDIR FAILED"));
      }
    } else {
      // Create a file
      File file = LittleFS.open(path, "w");
      if (file) {
        file.write((const char *)0);
        file.close();
      } else {
        return replyServerError(F("CREATE FAILED"));
      }
    }
    if (path.lastIndexOf('/') > -1) {
      path = path.substring(0, path.lastIndexOf('/'));
    }
    replyOKWithMsg(path);
  } else {
    // Source specified: rename
    if (src == "/") {
      return replyBadRequest("BAD SRC");
    }
    if (!LittleFS.exists(src)) {
      return replyBadRequest(F("SRC FILE NOT FOUND"));
    }

    DEBUG_PRINTLN(String("handleFileCreate: ") + path + " from " + src);

    if (path.endsWith("/")) {
      path.remove(path.length() - 1);
    }
    if (src.endsWith("/")) {
      src.remove(src.length() - 1);
    }
    if (!LittleFS.rename(src, path)) {
      return replyServerError(F("RENAME FAILED"));
    }
    replyOKWithMsg(lastExistingParent(src));
  }
}

/*
   Delete the file or folder designed by the given path.
   If it's a file, delete it.
   If it's a folder, delete all nested contents first then the folder itself

   IMPORTANT NOTE: using recursion is generally not recommended on embedded devices and can lead to crashes (stack overflow errors).
   This use is just for demonstration purpose, and FSBrowser might crash in case of deeply nested filesystems.
   Please don't do this on a production system.
*/
void deleteRecursive(String path) {
  File file = LittleFS.open(path, "r");
  bool isDir = file.isDirectory();
  file.close();

  // If it's a plain file, delete it
  if (!isDir) {
    LittleFS.remove(path);
    return;
  }

  // Otherwise delete its contents first
  Dir dir = LittleFS.openDir(path);

  while (dir.next()) {
    deleteRecursive(path + '/' + dir.fileName());
  }

  // Then delete the folder itself
  LittleFS.rmdir(path);
}

/*
   Handle a file deletion request
   Operation      | req.responseText
   ---------------+--------------------------------------------------------------
   Delete file    | parent of deleted file, or remaining ancestor
   Delete folder  | parent of deleted folder, or remaining ancestor
*/
void handleFileDelete() {
  if (!fsOK) {
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  String path = server.arg(0);
  if (path.isEmpty() || path == "/") {
    return replyBadRequest("BAD PATH");
  }

  DEBUG_PRINTLN(String("handleFileDelete: ") + path);
  if (!LittleFS.exists(path)) {
    return replyNotFound(FPSTR(FILE_NOT_FOUND));
  }
  deleteRecursive(path);

  replyOKWithMsg(lastExistingParent(path));
}

/*
   Handle a file upload request
*/
void handleFileUpload() {
  if (!fsOK) {
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    // Make sure paths always start with "/"
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    DEBUG_PRINTLN(String("handleFileUpload Name: ") + filename);
    uploadFile = LittleFS.open(filename, "w");
    if (!uploadFile) {
      return replyServerError(F("CREATE FAILED"));
    }
    DEBUG_PRINTLN(String("Upload: START, filename: ") + filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        return replyServerError(F("WRITE FAILED"));
      }
    }
    DEBUG_PRINTLN(String("Upload: WRITE, Bytes: ") + upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    DEBUG_PRINTLN(String("Upload: END, Size: ") + upload.totalSize);
  }
}

/*
   The "Not Found" handler catches all URI not explicitely declared in code
   First try to find and return the requested file from the filesystem,
   and if it fails, return a 404 page with debug information
*/
void handleNotFound() {
  if (!fsOK) {
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  String uri = ESP8266WebServer::urlDecode(server.uri()); // required to read paths with blanks

  if (handleFileRead(uri)) {
    return;
  }

  // Dump debug data
  String message;
  message.reserve(100);
  message = F("Error: File not found\n\nURI: ");
  message += uri;
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += '\n';
  for (uint8_t i = 0; i < server.args(); i++) {
    message += F(" NAME:");
    message += server.argName(i);
    message += F("\n VALUE:");
    message += server.arg(i);
    message += '\n';
  }
  message += "path=";
  message += server.arg("path");
  message += '\n';
  DEBUG_PRINT(message);

  return replyNotFound(message);
}

/*
   This specific handler returns the index.htm (or a gzipped version) from the /edit folder.
   If the file is not present but the flag INCLUDE_FALLBACK_INDEX_HTM has been set, falls back to the version
   embedded in the program code.
   Otherwise, fails with a 404 page with debug information
*/
void handleGetEdit() {
  if (handleFileRead(F("/edit/index.htm"))) {
    return;
  }
}

////////////////////////////////
// SETUP
void setup(void) {
  ////////////////////////////////
  // INIT
  //pinMode(LED_BLUE, OUTPUT);    // the pins with LEDs connected are outputs
  //digitalWrite(LED_BLUE, LOW);  // LED ON
  _LOG("Starting...\r");

  //SEND_PORT.begin(9600); 
//  pinMode(D5, OUTPUT); 
 // while( true ){
//    digitalWrite(D5, HIGH);
//    digitalWrite(LED_BLUE, LOW);
//    delay(500);
//    digitalWrite(D5, LOW);
//    digitalWrite(LED_BLUE, HIGH);
//    delay(500);  
  //SEND_PORT.println("a");
  //delay(500);
  //}
  
  /// RTC MEMORY TEST
//  Serial.begin(9600);
//  DEBUG_PRINTLN("Reading ");
//  readFromRTCMemory();
//  DEBUG_PRINT("Writing ");
//  writeToRTCMemory(0);
//
//  DEBUG_PRINT("Sleeping for 5 seconds. ");
//  if (rtcMem.count < 5) {
//    DEBUG_PRINTLN("Will wake up with radio!");
//    ESP.deepSleep(5000000, WAKE_RFCAL);
//  } else {
//    DEBUG_PRINTLN("Will wake up without radio!");
//    ESP.deepSleep(5000000, WAKE_RF_DISABLED);
//  }
  
  ////////////////////////////////
  readFromRTCMemory();
  Serial.begin(9600);
  Serial.println("test");

  setupUART();
  loadEEPROM();                // Fetch setup values
  startLittleFS();             // Start the LittleFS and list all content
  //Serial.println("HELLO");
  //DEBUG_PRINT("rtcMem.count: ");
  //DEBUG_PRINTLN(rtcMem.count);
  
  // Coldbooting, charging (make sure to get wifi up and running)
  if (rtcMem.count != 1) {
    startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
    startMDNS();                 // Start the mDNS responder
    startWebSocket();            // Start a WebSocket server
    startOTA();                  // Start the OTA service
    startServer();               // Start a HTTP server with a file read handler and an upload handler
  }
  
  startTasks();
  
  _LOG("Start completed.\r");
}

// Status meaning
#define CHARGING 1014
#define START_MOWER 1012
#define MOWING 1002
#define PINCODE E803
#define CHASSISOFF 1404 // Removed top chassis
////////////////////////////////
// LOOP
void loop(void) {
  if (rtcMem.count != 1) {                    // Cold boot or charging
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
  //httpServer.handleClient();
  wifiMulti.run();
  MDNS.update();
  } else {                                    // Wake from sleep
    yield();
  }

  // Check what sleep mode to use
  if( mow.stat.data == CHARGING 
      && millis()-mow.stat.t < 60*1000
      && millis() > 60*10*1000 ) {
    _LOG("Charging, sleep then wifi on.");
    DEBUG_PRINT("Sleep then wifi. Chrg > 1 min");
    writeToRTCMemory(0);   // Enable wifi after sleep
    ESP.deepSleep(1, WAKE_RFCAL);    
     
  } else if ( mow.stat.data == MOWING 
              && mow.actCutTime.data > 1
              && millis()-mow.actCutTime.t > 1
              && millis() > 60*10*1000) {
    _LOG("Mowing, going to sleep");
    DEBUG_PRINT("Mowing, going to sleep");
    writeToRTCMemory(1); 
    ESP.deepSleep(30*10e6, WAKE_RF_DISABLED);
  }
  
}

////////////////////////////////
// EVENTS

////////////////////////////////
// WEBSOCKETS
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      DBG_OUTPUT_PORT.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        num_websock = num; // Set global
        IPAddress ip = webSocket.remoteIP(num);
        DBG_OUTPUT_PORT.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        String sendStr = "timer:" + String(eepromVar1.timerManMode, DEC);
        webSocket.sendTXT(num, sendStr);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      DBG_OUTPUT_PORT.printf("[%u] get Text: %s\r\n", num, payload);
        if ( !memcmp(payload, "save", 4) ) {                      // save data to EEPROM
          eepromVar1.timerManMode = (uint16_t) strtol((const char *) &payload[4], NULL, 10);   // decode timerManMode data
          DBG_OUTPUT_PORT.printf("Timer Received: %d\rn", eepromVar1.timerManMode);
          EEPROM.put(0, eepromVar1);
          bool ok = EEPROM.commit();
          DEBUG_PRINTLN((ok) ? "Save OK" : "Commit failed");
          String s = ok ? "SaveOk" : "SaveFail";
          webSocket.sendTXT(num, s);
        } else if ( !memcmp(payload,"scan",4) ) {
          WiFi.scanNetworksAsync(prinScanResult);    // Start search on request
          DEBUG_PRINTLN("Network scan");
        } else if ( !memcmp(payload,"restart", 7) ) {
          DEBUG_PRINTLN("Restart request");
          delay(100);
          ESP.restart();
        }
      break;
  }
}

////////////////////////////////
// SEND REQUEST TO AUTOMOWER AND LOG
int sendReq( uint8_t *request, int len ) {
  //char requestAutomower[6] = ""; // Store the request to send
  uint8_t *ptr;
  ptr = request; 

  DBG_OUTPUT_PORT.printf("[Insert time here]: Len %d Sent data 0x%0*x 0x%0*x 0x%0*x 0x%0*x 0x%0*x to autmower.\r\n", len, 2, request[0], 2, request[1], 2, request[2], 2, request[3], 2, request[4]);

  if ( len > 5 || len < 5 ) {
    return 1; // Error
  } else if ( len == 5 ) {
    SEND_PORT.write(request, 5);
    tSendCmd = millis();
    writeToLogBuffer(LOG_SEND_BYTES, 0, ptr); 
  }
  return 0;
}

//int sendReqv2( ) {
//  //char requestAutomower[6] = ""; // Store the request to send
//  //uint8_t *ptr;
//  //ptr = request;
//  int len = sizeof(fifoReq.n);
//  
//  DBG_OUTPUT_PORT.printf("[Insert time here]: Len %d Sent data 0x%0*x 0x%0*x 0x%0*x 0x%0*x 0x%0*x to autmower.\r\n", len, 2, request->data[0], 2, request->data[1], 2, request->data[2], 2, request->data[3], 2, request->data[4]);
//
//  if ( len > 5 || len < 5 ) {
//    return 1; // Error
//  } else if ( len == 5 ) {
//    SEND_PORT.write(request->data, 5);
//    tSendCmd = millis();
//    writeToLogBuffer(LOG_SEND_BYTES, 0, request->data); 
//  }
//  return 0;
//}

////////////////////////////////
// GET TIME RESPONSE
int getResp(){
  uint8_t recvAutomower[6] = "";
  uint8_t *ptr;
  ptr = recvAutomower;
  
  int n = RCV_PORT.readBytes(recvAutomower, 5);
  DBG_OUTPUT_PORT.printf("n: %u\r\n", n);
  if ( n > 5 ) _LOG("Found more >= 5 bytes in buffer.\r");
  checkResp(ptr, n, millis());
  if ( n > 0 ) {
    writeToLogBuffer(LOG_READ_BYTES, n, ptr);       // Write received bytes to log buffer
  } else {
    _LOG("No response received.");
  }
  
  // Just for debugging
  //int process = 10;
  return 0;
}


// Quick and dirty to get it functioning.
int nglob = 0, timerStarted = 0;
unsigned long tTimerZero = 0;
////////////////////////////////
// SEND REQUESTS
void sendCmd() {
  unsigned long timLocal = millis()-tTimerZero;
  int maxCutTime = eepromVar1.timerManMode;
  
  DBG_OUTPUT_PORT.printf("Mow status %u, eepromTimer: %u, timerStarted %u, cutTime %u, intTimer: %lu \r\n", mow.stat.data, maxCutTime, timerStarted, mow.actCutTime.data, timLocal);
  if ( mow.stat.data == -1 ) {
    _LOG("No response from mower. Skipping mow check.\r\n");
  } else if ( mow.stat.data == CHARGING && millis()-mow.stat.t < 60*1000 ) {        // Charging
    timerStarted = 0;
  } else if ( mow.stat.data == MOWING 
              && millis()-mow.stat.t < 60*1000 
              && mow.actCutTime.data > 1 
              && millis()-mow.actCutTime.t < 60*1000 
              && timerStarted < 3 ) {                                  // Mowing for more than one minute
    _LOG("Setting mower in manual mode.\r\n");
    DBG_OUTPUT_PORT.printf("Setting mower in manual mode.\r\n");
    sendReq( commands.W_MODE_MAN, 5 );
    tTimerZero = millis();
    timerStarted++;
  } else if ( ((mow.actCutTime.data > maxCutTime           // Must have started mowing before quitting
              && millis()-mow.actCutTime.t < 60*1000) 
              || millis()-tTimerZero > maxCutTime*60*1000)
              && timerStarted != 0) {
    _LOG("Max cut time reached. Setting mower in AUTO mode.\r\n");            
    DBG_OUTPUT_PORT.print("Max cut time reached. Setting mower in AUTO mode.\r\n");
    if( timerStarted < 6 ) {  // Only send three times. ( If theres something wrong will spam every 10second )
      sendReq( commands.W_MODE_AUTO, 5 );
      timerStarted++;
    }
  }
  
  if ( nglob == 0 ) {
    sendReq( commands.R_STATUS, 5 );
  } else if ( nglob == 1 ) {
    sendReq( commands.R_MAEHZEIT, 5 );
  } else if ( nglob == 2 ) {
    sendReq( commands.R_MAEHZEIT, 5 );  
  }

  //DBG_OUTPUT_PORT.printf("Serial mem ready for write: %u millis: %u\r\n", Serial.availableForWrite(), millis());
  
  // TODO: Create some switch that reads in a list
  //       then sends the command.
  //       For now only support status command.
  //memcpy( requestAutomower, commands.R_STATUS, 5 ); // Fix to 5 bytes, will make sure no overflow occurs.
  //SEND_PORT.write(requestAutomower, 5);
  //tSendCmd = millis();
   
 // writeToLogBuffer(LOG_SEND_BYTES, 0, ptr);
  
  synchTime(); // Check if its time to synch internal clock with autmower
  
  nglob++;
  nglob = ( nglob > 1 ) ? 0 : nglob;
}

////////////////////////////////
// PROCESS SERIAL BUFFER
void readBuf() {
  unsigned long tCurrent = millis();
  const uint16_t tWait = 5000;            // Min time (mS) to wait for answer
  uint8_t recvAutomower[6] = "";
  uint8_t *ptr;
  ptr = recvAutomower;
  
  //DBG_OUTPUT_PORT.printf("tCurrent: %lx, tSendCmd %lx, villkor a: %u, villkor b: %u \r\n", tCurrent, tSendCmd, (tCurrent > tSendCmd+tWait),(tCurrent < (0xFFFFFFFF-(tWait-1))));
  if ( tCurrent > tSendCmd+tWait                  // Give some time for autmower to answer our request.
       && tCurrent < (0xFFFFFFFF-(tWait-1)) ) {   // Deal with wrap around
    while (RCV_PORT.available() > 0) {            // Read data in 5 bytes chunks.
        int n = RCV_PORT.readBytes(recvAutomower, 5);
        if ( n > 5 ) _LOG("[Insert time here]: Found more >= 5 bytes in buffer.\r");
        writeToLogBuffer(LOG_READ_BYTES, n, ptr);        // Write received bytes to log buffer
        if ( n == 5) checkResp(ptr, n, tCurrent);        // Process response
        memset(recvAutomower, 0, sizeof(recvAutomower)); // Set all to 0.

    }
  }
}

////////////////////////////////
// First byte should always be 0x0f omitting that
#define STATUS 0x01f1
#define READSECONDS 0x36b1
#define READMINUTE 0x36b3
#define READHOUR 0x36b5
#define CURRENTMOWINGTIME 0x0038
////////////////////////////////
// Check data and update global variables
int checkResp(uint8_t *data, uint8_t len, unsigned long t) {
  
  DBG_OUTPUT_PORT.printf("checkResp 0x%0*x 0x%0*x 0x%0*x 0x%0*x 0x%0*x to autmower\r\n", 2, data[0], 2, data[1], 2, data[2], 2, data[3], 2, data[4]);

  if(data[0] != 0x0f || len != 5){
    DBG_OUTPUT_PORT.println("Unexpected data at first byte in response or incorrect length.\r");
    return 1;
  }
  
  unsigned int respData = data[3] << 8 | data[4];
  unsigned int respCode = data[1] << 8 | data[2];
  
  DBG_OUTPUT_PORT.printf("respData: %u, respCode: %u\r\n", respData, respCode);
  
  switch(respCode) {
    case STATUS:
      DBG_OUTPUT_PORT.printf("Status: %u.\r\n", respData);
      if(respData > 1073){
        _LOG("Incorrect status value received.\r\n");
        return 1;
      } else {
      mow.stat.t = t;
      mow.stat.data = respData;
      }
      break;

    case CURRENTMOWINGTIME:
      if( respData == 0x3800 ) {
        DBG_OUTPUT_PORT.printf("Mowing time not set: %u.\r\n", respData);
        break;
      }
      
      DBG_OUTPUT_PORT.printf("Current mowing time: %u.\r\n", respData);
      mow.actCutTime.t = t;
      mow.actCutTime.data = respData;
      break;

    case READSECONDS:
      DBG_OUTPUT_PORT.printf("Seconds: %u.\r\n", respData);
      mow.mowClock.t = t;
      mow.mowClock.seconds = respData;
      break;

    case READMINUTE:
      DBG_OUTPUT_PORT.printf("Minute: %u.\r\n", respData);
      mow.mowClock.t = t;
      mow.mowClock.minute = respData;
      break;
 
    case READHOUR:
      DBG_OUTPUT_PORT.printf("Hour: %u.\r\n", respData);
      mow.mowClock.t = t;
      mow.mowClock.hour = respData;
      break;     
  }
}

////////////////////////////////
// WRITE TO LOG BUFFER
void writeToLogBuffer(uint8_t type, int n, uint8_t *data){
  
  char temp[100] = "", temp1[6] = "";
  int8_t hour=0, minut=0, sekund=0;

  // Get time
  calcTime(&hour, &minut, &sekund);
  
  if( type == LOG_READ_BYTES ) {
    DBG_OUTPUT_PORT.printf("Read bytes %u\r\n", n);
    sprintf(temp, "[%0*u:%0*u:%0*u]: Incoming data %i bytes ", 2, hour, 2, minut, 2, sekund, n);
    
    for (int i=0; i < n; i++) {
      sprintf(temp1, "0x%x ", data[i]);
      strcat(temp, temp1);
      memset(temp1, 0, sizeof(temp1));   // Reset temp1
    }
    strcat(temp, "\r");

  } else if ( type == LOG_SEND_BYTES ) {
    sprintf(temp, "[%0*u:%0*u:%0*u]: Sent data 0x%0*x 0x%0*x 0x%0*x 0x%0*x 0x%0*x to autmower\r", 2, hour, 2, minut, 2, sekund, 2, data[0], 2, data[1], 2, data[2], 2, data[3], 2, data[4]);
    //DBG_OUTPUT_PORT.printf("%s", temp);
  }
  
 _LOG(temp);                         // Write to buffer
 memset(temp, 0, sizeof(temp));      // Reset temp   
}

int nFile=0; // Incrementing number
////////////////////////////////
// WRITE TO LOG
void writeLog() {
  int res = 0;
  char newName[30] = "";
  size_t fileSize = 0;
  
  // Rename log file
  res = LittleFS.exists(logPath);
  if( res ) { 
    Dir dir = LittleFS.openDir("logs");
    while( dir.fileName() != "logfile.txt" ) { 
      dir.next(); 
    }
    fileSize = dir.fileSize();
  }
  
  DBG_OUTPUT_PORT.printf("Filesize: %zu, nFile: %u\r\n", fileSize, nFile);
  
  if ( fileSize > 1024*1024 ) { // Filesize bigger than 300KiB, rename and save.
    _LOG("Logfile big. Renaming...");
    sprintf(newName, "/logs/%u-logfile.txt", nFile);
    res = LittleFS.rename(logPath, newName);
    DBG_OUTPUT_PORT.printf("rename result: %u\r\n", res);
    if ( !res ) {
      res = LittleFS.remove(newName);
      DBG_OUTPUT_PORT.printf("rem aft ren fail: %u\r\n", res);
      res = LittleFS.rename(logPath, newName);
      DBG_OUTPUT_PORT.printf("rename aft ren fail: %u\r\n", res);
    }
    
    nFile++;
    nFile = (nFile > 19) ? 0 : nFile; // Write over old log files.
  }

  // Open and write
  res = LittleFS.exists(logPath);
  //DBG_OUTPUT_PORT.printf("log file exists result: %u\r\n", res);
  //if ( !res ) LittleFS.
  
  f = LittleFS.open(logPath, "a");
  ///_LOG(f ? "LOG: Open log file.\r" : "LOG: FAILED to open file.\r"); 
  if (f) {
    f.print(logStr);
    f.close();
    memset(logStr, 0, sizeof(logStr));
  } else if ( !f ) {
    DBG_OUTPUT_PORT.printf("fail to open log file\r\n");
  }
  //DBG_OUTPUT_PORT.printf("logStr size: %u, : %s \r\n", sizeof(logStr), logStr);
}

////////////////////////////////
// Synch time
void synchTime() {
  unsigned long tCurrent = millis();
  
  if( tCurrent > mow.mowClock.t+UPDATE_TIME                       // Check if we should synchronize      
      && tCurrent < (0xFFFFFFFF-(UPDATE_TIME-1)) ) {     // internal clock with autmower. Deal with wrap-around also...
        
        _LOG("Starting to synchronize time\r");
        setTime();
      }
  
}

////////////////////////////////////////////
// Set internal clock from automower clock.
void setTime() {
  _LOG("Trying to get time.\r");
  RCV_PORT.setTimeout(1000);                        // Increase timeout during this.

  sendReq(commands.R_SEKUNDE, sizeof(commands.R_SEKUNDE));
  getResp();

  sendReq(commands.R_STUNDE, sizeof(commands.R_STUNDE));
  getResp();

  sendReq(commands.R_MINUTE, sizeof(commands.R_MINUTE));
  getResp();

  sendReq(commands.R_SEKUNDE, sizeof(commands.R_SEKUNDE));
  getResp();
    
  RCV_PORT.setTimeout(1);                           // Timeout after 1 millisecond of waiting for data. Only when using Serial.readBytes() functions.
}

////////////////////////////////
// START SERVICES

////////////////////////////////
// UART
void setupUART() {
  //DBG_OUTPUT_PORT.begin(74880); // Start the Serial communication to send messages to the computer
  delay(10);
  DEBUG_PRINTLN("\r\n");
  RCV_PORT.begin(9600);             // Listen for mower data on Serial0
  SEND_PORT.begin(9600);            // Send mower data on Serial1

  // Empty all data in receive buffer
  RCV_PORT.setTimeout(1);
  while ( RCV_PORT.available() > 0 ) {
    RCV_PORT.read();
  }
  
  _LOG("Setup UART done\r");
}

////////////////////////////////
// Tasks
void startTasks() {
  // Wait 0.5 seconds for mower to start
  while( millis() < 500 ) {
    yield();
  }
  setTime();
  sendCommand.attach(10, sendCmd);    // Task that sends commands periodically.
  readBuffer.attach(0.25, readBuf);   // Takes care of data in serial buffer
  wrteLog.attach(10, writeLog);       // Write to log file
  //getTime.attach(5, synchTime);         // Check if we should synchronize clock
}

void stopTasks() {
  sendCommand.detach();    // Task that sends commands periodically.
  readBuffer.detach();   // Takes care of data in serial buffer
  wrteLog.detach();       // Write to log file
}

////////////////////////////////
// WIFI
void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  //WiFi.softAP(ssid);             // Start the access point
  //WiFi.mode(WIFI_AP_STA);
  
  //DEBUG_PRINT("Access Point \"");
  //DEBUG_PRINT(ssid);
  //DEBUG_PRINTLN("\" started\r\n");

  wifiMulti.addAP(ssid_ap1, ssid_pass_ap1);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP(ssid_ap2, ssid_pass_ap2);
  wifiMulti.addAP(ssid_ap3, ssid_pass_ap3);
  
  DEBUG_PRINTLN("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    DEBUG_PRINT('.');
  }
  DEBUG_PRINTLN("\r\n");
  
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    DEBUG_PRINT("Connected to ");
    DEBUG_PRINTLN(WiFi.SSID());             // Tell us what network we're connected to
    DEBUG_PRINT("IP address:\t");
    DEBUG_PRINT(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    DEBUG_PRINT("Station connected to ESP8266 AP");
  }
  DEBUG_PRINTLN("\r\n");
}


////////////////////////////////
// LITTLEFS
void startLittleFS() {
  fileSystemConfig.setAutoFormat(false);
  LittleFS.setConfig(fileSystemConfig);
  fsOK = LittleFS.begin();
  DEBUG_PRINTLN(fsOK ? F("Filesystem initialized.") : F("Filesystem init failed!"));

  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {                      // List the file system contents
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    //DBG_OUTPUT_PORT.printf("\tFS File: %s, size: %s\r", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  //DBG_OUTPUT_PORT.printf("\r\n");
}

////////////////////////////////
// MDNS
void startMDNS() { // Start the mDNS responder
  if (MDNS.begin(mdnsName)) {
    MDNS.addService("http", "tcp", 80);                        // start the multicast domain name server
    DEBUG_PRINT("mDNS responder started: http://");
    DEBUG_PRINT(mdnsName);
    DEBUG_PRINTLN(".local");
  }else {
    DEBUG_PRINTLN("mDNS fail.");
  }
}

////////////////////////////////
// WEB SERVER INIT
void startServer() {
  // Filesystem status
  server.on("/status", HTTP_GET, handleStatus);

  // List directory
  server.on("/list", HTTP_GET, handleFileList);

  // Load editor
  server.on("/edit", HTTP_GET, handleGetEdit);

  // Create file
  server.on("/edit",  HTTP_PUT, handleFileCreate);

  // Delete file
  server.on("/edit",  HTTP_DELETE, handleFileDelete);

  // Upload file
  // - first callback is called after the request has ended with all parsed arguments
  // - second callback handles file upload at that location
  server.on("/edit",  HTTP_POST, replyOK, handleFileUpload);

  // Default handler for all URIs not defined above
  // Use it to read files from filesystem
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  DEBUG_PRINTLN("HTTP server started");
}

////////////////////////////////
// OTA
void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    stopTasks();
    DEBUG_PRINTLN("Start");
    //digitalWrite(LED_BLUE, 1); // turn off the LEDs
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DBG_OUTPUT_PORT.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DBG_OUTPUT_PORT.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
  DEBUG_PRINTLN("OTA ready\r\n");
}

////////////////////////////////
// WEBSOCKETS
void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  DEBUG_PRINTLN("WebSocket server started.");
}


////////////////////////////////
// EEPROM (Could be replaced by reading a setup file....)
void loadEEPROM() {
  EEPROM.begin(sizeof(saveStruct));
  
  if(EEPROM.percentUsed()>=0) {
    EEPROM.get(0, eepromVar1);
    eepromVar1.timerManMode = (eepromVar1.timerManMode > 270) ? 180 : eepromVar1.timerManMode;
    //eepromVar1.timerManMode;     // make a change to our copy of the EEPROM data
    DEBUG_PRINTLN("EEPROM has data from a previous run.");
    DEBUG_PRINT(EEPROM.percentUsed());
    DEBUG_PRINTLN("% of ESP flash space currently used");
  } else {
    DEBUG_PRINTLN("EEPROM size changed - EEPROM data zeroed - commit() to make permanent");
    eepromVar1.timerManMode = 180;  // Set default value.
    EEPROM.put(0, eepromVar1);
    bool ok = EEPROM.commit();
    DEBUG_PRINTLN((ok) ? "Commit OK" : "Commit failed");
    EEPROM.get(0, eepromVar1);
  }
  DBG_OUTPUT_PORT.printf("Readback data: %u\r", eepromVar1.timerManMode);
}

////////////////////////////////
// HELPER FUNCTIONS
void calcTime(int8_t *hour, int8_t *minut, int8_t *sekunder) {
  // TODO: Fix wrap around.
  unsigned long diff = (millis() - mow.mowClock.t) / 1000;

  *hour = diff / 3600;
  *minut = (diff-*hour*3600) / 60;
  *sekunder = (diff-*hour*3600-*minut*60);

  //DBG_OUTPUT_PORT.printf("Time: %u:%u:%u\r\n", *hour, *minut, *sekunder);
}

void prinScanResult(int networksFound)
{
  char tmp[200] = "", num[20] = "";
  
  DBG_OUTPUT_PORT.printf("%d network(s) found\r\n", networksFound);
  for (int i = 0; i < networksFound; i++)
  {
    DBG_OUTPUT_PORT.printf("%d: %s, Ch:%d (%ddBm) %s\r\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
    (i>=0 && i<20) ? sprintf(num, "%d;", i) : sprintf(num, "-1;", 2); // Store in num
    strcat(tmp, num);
    memset(num, 0, sizeof num); // Clear
    sprintf(num, "%ddBm;", WiFi.RSSI(i));
    strcat(tmp, num);
    strcat(tmp, WiFi.SSID(i).c_str());
    strcat(tmp, ";");
    memset(num, 0, sizeof num); // Clear temp num
  }
  webSocket.sendTXT(num_websock, tmp);
  DEBUG_PRINTLN(tmp);
}

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
