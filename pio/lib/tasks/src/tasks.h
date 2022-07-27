#ifndef TASKS_H 
#define TASKS_H

#include <dbgDefs.h>
#include <Ticker.h>
#include <mower.h>

// instances
Ticker sendCommand, readBuffer, wrteLog, getTime, sendDB; // Tasks
bool sendCmdFlag=false;
const uint8_t fac = 1;
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
void sendToDb();

// Functions
void startTasks() {
  // Wait 0.5 seconds for mower to start
  sendCommand.attach(fac*10, sendCmd);    // Task that sends commands periodically.
  readBuffer.attach(fac*0.25, readBuf);   // Takes care of data in serial buffer
  //sendDB.attach(fac*10, sendToDb);
  //wrteLog.attach(10, writeLog);       // Write to log file
  //getTime.attach(5, synchTime);         // Check if we should synchronize clock
}

void stopTasks() {
  Serial.println("stopTasks");
  sendCommand.detach();    // Task that sends commands periodically.
  readBuffer.detach();   // Takes care of data in serial buffer
  //wrteLog.detach();       // Write to log file
}

// Send measurement data to influxdb
void sendToDb(){
  // Store measured value into point
  status.clearFields();
  // Report RSSI of currently connected network
  status.addField("value", mow.stat.data);
  // Print what are we exactly writing
  debugD("Writing: ");
  //debugD("%s", client.pointToLineProtocol(status).c_str());

  // Write point
  if (!client.writePoint(status)) {
    debugW("InfluxDB write failed: ");
    //debugW("%s", client.getLastErrorMessage().c_str());
  }
}

void blockingTasks(){
  bool resp=false;

  // Send if timeout or response receieved.
  if((sendTracker.tracker 
      && (unsigned long)(millis()-sendTracker.t) > 500) // Wait 500ms between requests 
      || sendTracker.respRecvd ){

    if(sendTracker.tracker==1) resp=sendMowReq(commands.R_STATUS, sizeof(commands.R_STATUS));
    else if (sendTracker.tracker==2) resp=sendMowReq(commands.R_AKKU_LADEZEIT_MIN , sizeof(commands.R_AKKU_LADEZEIT_MIN));
    else if (sendTracker.tracker==3) resp=sendMowReq(commands.R_AKKU_KAPAZITAET_MA, sizeof(commands.R_AKKU_KAPAZITAET_MA));
    else if (sendTracker.tracker==4) resp=sendMowReq(commands.R_AKKU_LETZTER_LADEVORGANG_MIN, sizeof(commands.R_AKKU_LETZTER_LADEVORGANG_MIN));
    else if (sendTracker.tracker==5) resp=sendMowReq(commands.R_AKKU_SPANNUNG_MV, sizeof(commands.R_AKKU_SPANNUNG_MV));
    //else if (sendTracker.tracker==2) resp=sendMowReq(commands.R_MAEHZEIT , sizeof(commands.R_MAEHZEIT)); // doesnt work always 0
    //else if (sendTracker.tracker==4) resp=sendMowReq(commands.R_VIERECKMODUS_STATUS , sizeof(commands.R_VIERECKMODUS_STATUS));
    //else if (sendTracker.tracker==5) resp=sendMowReq(commands.R_VIERECKMODUS_PROZENT, sizeof(commands.R_VIERECKMODUS_PROZENT));
    //else if (sendTracker.tracker==6) resp=sendMowReq(commands.R_VIERECKMODUS_REFERENZ, sizeof(commands.R_VIERECKMODUS_REFERENZ));
    //else if (sendTracker.tracker==8) resp=sendMowReq(commands.R_AKKU_KAPAZITAET_MAH, sizeof(commands.R_AKKU_KAPAZITAET_MAH));
    //else if (sendTracker.tracker==10) resp=sendMowReq(commands.R_AKKU_KAPAZITAET_GENUTZT_MAH, sizeof(commands.R_AKKU_KAPAZITAET_GENUTZT_MAH));
    
    if(resp) debugW("Failed to send mowCommand."); 
    // set state of sendTracker so we can track the progress.
    sendTracker.respRecvd = false;
    sendTracker.t = millis(); // last time of execution
    sendTracker.tracker--;
  }
  
  if(sendTracker.tracker==0) sendCmdFlag=false; // all messages sent.
}

// set condition to send requests in blockingTasks()
void sendCmd(){
  sendTracker.tracker=5;
}

// read serial buffer in 5 bytes chunks and process response.
void readBuf(){

  uint8_t recvAutomower[6] = "";

  //debugD("Readbuf length: %d/r/n", Serial.available());
  // Consume characters until 0x0f
  while( Serial.peek() != 0x0f 
         && Serial.available() ) {   
    debugD("Expected 0xf got 0x%x. Removing from buffer.", Serial.peek());
    Serial.read();
  }

  while (Serial.available() >= 5) {            // Read data in 5 bytes chunks.
    int n = Serial.readBytes(recvAutomower, 5);
    if ( n > 5 ) debugD("Found more >= 5 bytes in buffer.");
    if ( n == 5) sendTracker.respRecvd=!processResp(recvAutomower, n, millis());        // Process response
    //if ( n == 5) sendTracker.respRecvd=!processResp({0x0f, 0x1, 0xf1, 0x10, 0x2}, n, millis());        // Process response
    memset(recvAutomower, 0, sizeof(recvAutomower));   // Clean array for next 5 byte chunk
    }

}

void writeLog(){
}

#endif // TASKS_H