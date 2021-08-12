// includes
#include <Arduino.h>
#include <dbgDefs.h>  // Debug definitions
#include <tasks.h>    // Tasks
#include <wifi.h>     // WIFI settings
#include <rtc.h>
#include <serial.h>
#include <mower.h>    // mower definitions and structs
#include <filesystem.h>

// global stuff
#define S_CHARGING 0
#define S_MOWING_TO_MAN 1
#define S_MOWING_NOW 2
#define S_STOP_MOWING 3 // Set to AUTO
#define S_ERROR 254
#define S_UNKOWN 255
const uint8_t maxMowTime = 180; // [minutes] Maximum mowing time
uint8_t mowState=255, mowStateDesired=255; // Statemachine actual and desired state.
unsigned long timMowStart; // millis() the time mowing started.

// function prototypes
void stateChanger();
void printState();
void printWiFiDebug();

// functions
void stateChanger(){

  // Check for state change and execute state change code
  if(mowState != mowStateDesired){

    if((mowState==S_UNKOWN || mowState==S_ERROR || mowState==S_STOP_MOWING) 
        && mowStateDesired == S_CHARGING){
      //WiFi.forceSleepWake();
      //delay(1); //Insert code to connect to WiFi, start your servers or clients or whatever
      debugV("Charging, starting WiFi.");
    } 
    else if ( (mowState==S_UNKOWN || mowState==S_CHARGING)
                && mowStateDesired == S_MOWING_NOW ){
      debugV("Switching to manual mode.");
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      
      timMowStart = millis(); // Save time started.
      debugV("timMowStart: %lu", timMowStart);
      // Insert whatever code here to turn off all your web-servers and clients and whatnot
      //WiFi //wifiMulti.disconnect();
      //WiFi //wifiMulti.forceSleepBegin();
      //delay(1); //For some reason the modem won't go to sleep unless you do a delay(non-zero-number) -- no delay, no sleep and delay(0), no sleep
    } 
    else if ( mowState==S_MOWING_NOW 
              && mowStateDesired==S_STOP_MOWING ){
      debugV("Switching to auto mode.");
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);

      debugV("timMowStart: %lu", timMowStart);

    }

    mowState = mowStateDesired;
  }

  // Run actions in state
  switch(mowState){
    case S_UNKOWN:
      wifiMulti.run();
      if ( get_mow_status() == CHARGING ) mowStateDesired = S_CHARGING;
      //else if ( get_mow_status() == MOWING) mowStateDesired = S_MOWING_NOW;
      if(mowStateDesired != mowState) printState();
      break;

    case S_CHARGING:
      wifiMulti.run();
      if ( get_mow_status() == MOWING 
           && ((get_mow_actCutTime() > 2) || true)) {
        mowStateDesired = S_MOWING_NOW;
        printState();
      } 
      break;

    case S_MOWING_NOW:
      if( (unsigned long) (millis()-timMowStart) >= maxMowTime*60*1000// cast for overflow to work
          || (get_mow_actCutTime() > maxMowTime && get_mow_status() == MOWING) ){         
        mowStateDesired = S_STOP_MOWING;
        printState();
      }
      break;

    case S_STOP_MOWING:
      if( get_mow_status() == CHARGING ){
        mowStateDesired = S_CHARGING;
        printState();
      }
      break;

    //case S_ERROR:
    //  break;
  }
  
}

void setup() {
  // put your setup code here, to run once:
  //pinMode(2, OUTPUT);
  
  /* 
  for(int i=0;i<256;i++){
    readRTCMemory();
    DBG_OUTPUT_PORT.printf("Read rtcMem.count: %d\r\n", rtcMem.count);
    writeRTCMemory(i);  
    yield();
  }*/
  readRTCMemory(); // Read in RTC Memory. TODO: Make a counter updated in memory.
  setupUART();
  startWiFi();
  startOTA();      // Waits for 60 sec to always allow OTA flash on power on.
  startTasks();

  #ifndef DEBUG_DISABLE
  // Initialize RemoteDebug
	Debug.begin(STASSID); // Initialize the WiFi server
  Debug.setResetCmdEnabled(true); // Enable the reset command
	Debug.showProfiler(true); // Profiler (Good to measure times, to optimize codes)
	Debug.showColors(true); // Colors
  #endif

  Serial.println("Run program.");
  debugA("Run program");

}

uint8_t wifiStatePrev=0, wifiState=0; // last loop status
void loop() {
  // put your main code here, to run repeatedly:
  while(true){
    if(sendTracker.tracker) blockingTasks();
    stateChanger();

    wifiState = wifiMulti.run();
    ArduinoOTA.handle();  // listen for OTA events
    Debug.handle();
    MDNS.update();
    
    if( millis()%10000 == 0) printState();
    if( wifiState != wifiStatePrev) printWiFiDebug();
    wifiStatePrev = wifiState;

    yield();
  }
}

void printWiFiDebug(){
  debugD("wifistate: %u, wifiStatePrev: %u", wifiState, wifiStatePrev);
/*
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_WRONG_PASSWORD   = 6,
    WL_DISCONNECTED     = 7
*/
}
void printState(){
  debugD("State change request. state: %u, mowStateDesired: %u", mowState, mowStateDesired);
  //debugD("rtcMem timer: %u bmowing: %u", rtcMem.count, rtcMem.bMowing);
}