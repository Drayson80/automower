#ifndef TASKS_H 
#define TASKS_H

#include <dbgDefs.h>
#include <Ticker.h>
#include <mower.h>

// instances
Ticker sendCommand, readBuffer, wrteLog, getTime; // Tasks
bool sendCmdFlag=false;

//structures
struct sendStruct {
  bool respRecvd=false;
  uint32_t t;
  uint8_t tracker=0;
} sendTracker;

// Prototypes
void startTasks();
void stopTasks();
void blockingTasks();
void sendCmd();
void readBuf();
void writeLog();

// Functions
void startTasks() {
  // Wait 0.5 seconds for mower to start
  sendCommand.attach(10, sendCmd);    // Task that sends commands periodically.
  readBuffer.attach(0.25, readBuf);   // Takes care of data in serial buffer
  wrteLog.attach(10, writeLog);       // Write to log file
  //getTime.attach(5, synchTime);         // Check if we should synchronize clock
}

void stopTasks() {
  Serial.println("stopTasks");
  sendCommand.detach();    // Task that sends commands periodically.
  readBuffer.detach();   // Takes care of data in serial buffer
  wrteLog.detach();       // Write to log file
}

void blockingTasks(){
  bool resp=false;

  // Send if timeout or response receieved.
  if((sendTracker.tracker 
      && millis()-sendTracker.t > 1000) 
      || sendTracker.respRecvd ){

    if(sendTracker.tracker==1) resp=sendMowReq(commands.R_STATUS, sizeof(commands.R_STATUS));
    else if (sendTracker.tracker==2) resp=sendMowReq(commands.R_MAEHZEIT , sizeof(commands.R_MAEHZEIT));
    if(resp) Serial.println("Fail to send command."); 
    // set state of sendTracker so we can track the progress.
    sendTracker.respRecvd = false;
    sendTracker.t = millis(); // last time of execution
    sendTracker.tracker--;
  }
  
  if(sendTracker.tracker==0) sendCmdFlag=false; // all messages sent.
}

// set condition to send requests in blockingTasks()
void sendCmd(){
  sendTracker.tracker=2;
}

// read serial buffer in 5 bytes chunks and process response.
void readBuf(){

  uint8_t recvAutomower[6] = "";

  while (Serial.available() > 0) {            // Read data in 5 bytes chunks.
    int n = Serial.readBytes(recvAutomower, 5);
    if ( n > 5 ) Serial.println("Found more >= 5 bytes in buffer.");
    if ( n == 5) sendTracker.respRecvd=!processResp(recvAutomower, n, millis());        // Process response
    //if ( n == 5) sendTracker.respRecvd=!processResp({0x0f, 0x1, 0xf1, 0x10, 0x2}, n, millis());        // Process response
    memset(recvAutomower, 0, sizeof(recvAutomower));   // Clean array for next 5 byte chunk
    }

}

void writeLog(){
}

#endif // TASKS_H