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

// functions
void stateChanger(){

  // Check for state change and execute state change code
  if(mowState != mowStateDesired){

    if((mowState==S_UNKOWN || mowState==S_ERROR) 
        && mowStateDesired == S_CHARGING){
      //WiFi.forceSleepWake();
      //delay(1); //Insert code to connect to WiFi, start your servers or clients or whatever
      
    } else if ( (mowState==S_UNKOWN || mowState==S_CHARGING)
                && mowStateDesired == S_MOWING_NOW ){
      DEBUG_PRINTLN("Switching to manual mode.");
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
      delay(250);
      
      timMowStart = millis(); // Save time started.
      // Insert whatever code here to turn off all your web-servers and clients and whatnot
      //WiFi //wifiMulti.disconnect();
      //WiFi //wifiMulti.forceSleepBegin();
      //delay(1); //For some reason the modem won't go to sleep unless you do a delay(non-zero-number) -- no delay, no sleep and delay(0), no sleep
    } 
    else if ( mowState==S_MOWING_NOW 
              && mowStateDesired==S_STOP_MOWING ){
      DEBUG_PRINTLN("Switching to auto mode.");
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);
      sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
      delay(250);
    }

    mowState = mowStateDesired;
  }

  // Run actions in state
  switch(mowState){
    case S_UNKOWN:
      wifiMulti.run();
      //DBG_OUTPUT_PORT.printf("mowstat: %d\r\n", get_mow_status());
      if ( get_mow_status() == CHARGING ) mowStateDesired = S_CHARGING;
      //else if ( get_mow_status() == MOWING) mowStateDesired = S_MOWING_NOW;
      break;

    case S_CHARGING:
      wifiMulti.run();
      if ( get_mow_status() == MOWING 
           && ((get_mow_actCutTime() > 2) || true)) {
        DEBUG_PRINTLN("stateDesired change to S_MOWING_NOW");
        mowStateDesired = S_MOWING_NOW;
      } 
      break;

    case S_MOWING_NOW:
      if( (unsigned long) (millis()-timMowStart) >= maxMowTime*60*1000 
          || (get_mow_actCutTime() > maxMowTime && get_mow_status() == MOWING) ){ // cast for overflow to work
        DEBUG_PRINTLN("Mowing cut time end reached.");       
        mowStateDesired = S_STOP_MOWING;
      }
      break;

    case S_STOP_MOWING:
      if( get_mow_status() == CHARGING ){
        DEBUG_PRINTLN("stateDesired change to S_CHARGING");
        mowStateDesired = S_CHARGING;
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
  setupUART();
  startWiFi();
  startOTA();
  startTasks();

}

void loop() {
  // put your main code here, to run repeatedly:
  //digitalWrite(2, LOW);
  //delay(500);
  //digitalWrite(2, HIGH);
  //delay(500);
  //sendMowReq(commands.R_STATUS, sizeof(commands.R_STATUS));
  //uint8_t recvAutomower[6] = {0x0f};
  //processResp(recvAutomower, 5, millis());

  if(sendTracker.tracker) blockingTasks();
  stateChanger();
  //checkMowStatus();

  if(millis()%2000 == 0){
    Serial.printf("millis: %ld state: %u, mowStateDesired: %u\r\n", millis(), mowState, mowStateDesired);
    delay(1);
  }
  wifiMulti.run();
  ArduinoOTA.handle();  // listen for OTA events
  //MDNS.update();
}
