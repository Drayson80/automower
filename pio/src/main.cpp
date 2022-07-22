// global stuff
#define S_CHARGING 0
#define S_MOWING_TO_MAN 1
#define S_MOWING_NOW 2
#define S_STOP_MOWING 3 // Set to AUTO
#define S_ERROR 254
#define S_UNKOWN 255
#define AUTO 0
#define MAN 1
// includes
#include <Arduino.h>
#include <dbgDefs.h>  // Debug definitions
#include <tasks.h>    // Tasks
#include <wifi.h>     // WIFI settings
#include <rtc.h>
#include <serial.h>
#include <mower.h>    // mower definitions and structs

const uint8_t maxMowTime = 180; // [minutes] Maximum mowing time
uint8_t mowState=255, mowStateDesired=255, mowMode=AUTO; // Statemachine actual and desired state.
unsigned long tOffsetMow=0; // Mowing offset added to current mowing time. (if reset detected)
unsigned long timMowStart; // millis() the time mowing started.
#define MOWTIME ((unsigned long)(millis()-timMowStart))

// function prototypes
void printWiFiDebug();
void printState();
void stateChanger();

void stateChanger(){
  // Check for state change and execute state change code
  if(mowState != mowStateDesired){
    if((mowState==S_UNKOWN || mowState==S_ERROR || mowState==S_STOP_MOWING) 
        && (mowStateDesired == S_CHARGING || mowStateDesired == S_UNKOWN)){
      //WiFi.forceSleepWake();
      //delay(1); //Insert code to connect to WiFi, start your servers or clients or whatever
      //debugV("Charging, starting WiFi.");
      backupRAM(0UL, mowStateDesired, rtcMem);  // Update ram backup
    } 
    else if ( (mowState==S_UNKOWN || mowState==S_CHARGING)
                && mowStateDesired == S_MOWING_NOW ){
      rtcMem.reset_counter = 0; // Reset reset counter.
      writeRTCMemory();

      timMowStart = millis(); // time when cutting started.

      debugI("Switching to manual mode.");
      manModeMowReq();
      mowMode=MAN;

      debugI("Time at cutting start. timMowStart: %lu", timMowStart);
      
      // Insert whatever code here to turn off all your web-servers and clients and whatnot
      //WiFi //wifiMulti.disconnect();
      //WiFi //wifiMulti.forceSleepBegin();
      //delay(1); //For some reason the modem won't go to sleep unless you do a delay(non-zero-number) -- no delay, no sleep and delay(0), no sleep
    } 
    else if ( mowState==S_MOWING_NOW 
              && mowStateDesired==S_STOP_MOWING ){
      debugI("Switching to auto mode.");
      autoModeMowReq();
      mowMode=AUTO;
    }

    mowState = mowStateDesired; // update mowState
  }

  // Run actions in state
  switch(mowState){
    case S_UNKOWN:
      if ( IS_CHARGING ) mowStateDesired = S_CHARGING;
      //else if ( get_mow_status() == MOWING) mowStateDesired = S_MOWING_NOW;
      if(mowStateDesired != mowState) printState();
      break;

    case S_CHARGING:
      if ( IS_MOWING ) {
           //&& ((get_mow_actCutTime() > 2))) {
        mowStateDesired = S_MOWING_NOW;
        printState();
      }
      break;

    case S_MOWING_NOW:
      if( MOWTIME >= maxMowTime*60*1000 ) {  // cast for overflow to work
          //|| (get_mow_actCutTime() > maxMowTime && IS_MOWING ) ){         
        mowStateDesired = S_STOP_MOWING;
        printState();
      }

      if( (MOWTIME % 60000 == 0) ) backupRAM(MOWTIME, mowState, rtcMem); // Update ram backup every minute.
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

// functions
void setup() {
  // put your setup code here, to run once:
  readRTCMemory(); // Read in RTC Memory. TODO: Make a counter updated in memory.
  checkIfReset(&tOffsetMow, &mowState, &mowStateDesired);  // If reset during mowing, restore cuttingtime and statemachine.
  setupUART();
  autoModeMowReq(); // If reset occurs. Go back to AUTO.
  startWiFi();
  startOTA();      // Waits for 60 sec, allows OTA flash on power on. Even if code is broken further down.
  startTasks();
  startDebug();
  //yield();
  //delay(1000);
  //yield();

  Serial.println("Run program.");
  debugA("Run program");

}

uint8_t wifiStatePrev=0, wifiState=0; // last loop status
void loop() {
  // put your main code here, to run repeatedly:
  while(true){
    if(sendTracker.tracker) blockingTasks();
    stateChanger();           // Update state-machine
    incrementInternalClock(); // Update our internal mirror clock.
    syncInternalClock();

    wifiState = wifiMulti.run();
    ArduinoOTA.handle();  // listen for OTA events
    Debug.handle();
    MDNS.update();

    
    if( millis()%20000 == 0) printState();
    //if( wifiState != wifiStatePrev) printWiFiDebug();
    //wifiStatePrev = wifiState;

    yield();
  }
}

void printState(){
  debugI("mowMode: %u, State: %u, mowStateDesired: %u", mowMode, mowState, mowStateDesired);
  debugI("mowClock: %u:%u:%u, mowTime: %lu", mow.mowClock.hour, mow.mowClock.minute, mow.mowClock.seconds, MOWTIME/(1000*60*60));
  debugI("mowStatus: %i, reset_counter: %u, mowTime_b: %lu, mowstate_b: %i, mowstatedesired_b: %i", get_mow_status(), rtcMem.reset_counter, rtcMem.mowTime_b, rtcMem.mowState_b, rtcMem.mowStateDesired_b);
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
