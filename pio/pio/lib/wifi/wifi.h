#ifndef DWIFI_H
#define DWIFI_H

#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <tasks.h>

// function prototypes
void startWiFi(); 
void startOTA();

// Wifi
#define STASSID "amw"          // AP Name
#define STAPSK  "automower"               // AP Password (NOT USED NOW, no password)
const char *ssid_ap1 = "Printerzone";
const char *ssid_pass_ap1 = "zmoddans";
const char *ssid_ap2 = "dd-wrt";
const char *ssid_pass_ap2 = "Robertsson145";
const char *ssid_ap3 = "dd-wrt_RPT";
const char *ssid_pass_ap3 = "Robertsson145";

// OTA
const char *OTAName = "amwOTA";        // A name and a password for the OTA service
const char *OTAPassword = STAPSK;
const char *mdnsName = STASSID;       // Domain name for the mDNS responder
const char *ssid = STASSID;
const char *password = STAPSK;

// WifiMulti
ESP8266WiFiMulti wifiMulti;


// StartWiFi
void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  //WiFi.softAP(ssid, password);             // Start the access point
  
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


// StartOTA
void startOTA(){ 
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    stopTasks();
  });
  ArduinoOTA.onEnd([]() {
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

#endif