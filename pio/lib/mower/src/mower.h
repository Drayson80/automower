#ifndef MOWER_H
#define MOWER_H
// includes
#include <serial.h> 

// definitions
#define CHARGING 1014
#define START_MOWER 1012
#define MOWING 1002 
// Above is in decimal and should not be in hex.
//#define MOWING 0xEA03 // klipper (wrong endian?!)
#define MOWING1 1004 // klipper
#define PINCODE 0xE803 // enter pin code, wrong endian
#define CHASSISOFF 0x1404 // Removed top chassis, wrong endian
#define YES_TO_START 0x1c04 // push yes to start on screen, wrong endian
#define PARK_IN_STATION 0x2004 // When parked in station and option choosed to stay in station, wrong endian
// 1204 2804 searching, wrong endian
// 1804 docking, wrong endian

// First byte should always be 0x0f omitting that
#define STATUSMOWER 0x01f1
#define READSECONDS 0x36b1
#define READMINUTE 0x36b3
#define READHOUR 0x36b5
#define CURRENTMOWINGTIME 0x0038
#define CHARGETIME 0x1ec

// structures
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
  genDataStruct chargeTime;
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

// function prototypes
int sendMowReq(uint8_t *request, uint8_t len);
int processResp(uint8_t *data, uint8_t len, uint32_t t);
int checkMowStatus(); // Decide what to do in different stages
int get_mow_status(); // Get status value. -1 = not valid data.
int get_mow_actCutTime(); // Actual cutTime. -1 = not valid data.
int time_valid( unsigned long t); 

// variables

// functions
int sendMowReq(uint8_t *request, uint8_t len){
  if(len < 5 || len > 5) return 1;
  swSer.write(request, 5);
  return 0;
}

int processResp(uint8_t *data, uint8_t len, uint32_t t){
  debugD("checkResp 0x%0*x 0x%0*x 0x%0*x 0x%0*x 0x%0*x to autmower", 2, data[0], 2, data[1], 2, data[2], 2, data[3], 2, data[4]); 

  if(data[0] != 0x0f || len != 5){
    debugD("Unexpected data at first byte in response or incorrect length.\r");
    return 1;
  }

  uint32_t respData = data[4] << 8 | data[3];
  uint32_t respCode = data[1] << 8 | data[2];

  switch(respCode) {
    case CHARGETIME:
      mow.chargeTime.t = t;
      mow.chargeTime.data = respData;
      debugD("mow.chargeTime.data: %d, mow.chargeTime.t: %ld\r\n", mow.chargeTime.data, mow.chargeTime.t);
      break;

    case STATUSMOWER:
      mow.stat.t = t;
      mow.stat.data = respData;
      //debugD("convert status: %d,", (uint32_t)(data[4]<<8 | data[3]));
      debugD("mow.stat.data: %d, mow.stat.t: %ld\r\n", mow.stat.data, mow.stat.t);
      break;

    case CURRENTMOWINGTIME:
      mow.actCutTime.t = t;
      mow.actCutTime.data = respData;
      debugD("Current mowing time: %u.\r\n", mow.actCutTime.data);
      break;

    case READSECONDS:
      debugD("Seconds: %u", respData);
      mow.mowClock.t = t;
      mow.mowClock.seconds = respData;
      break;

    case READMINUTE:
      debugD("Minute: %u", respData);
      mow.mowClock.t = t;
      mow.mowClock.minute = respData;
      break;
 
    case READHOUR:
      debugD("Hour: %u", respData);
      mow.mowClock.t = t;
      mow.mowClock.hour = respData;
      break;     
  }
  return 0;
}

int checkMowStatus() {
  // if(mow.stat.data == -1
  //   ||  (unsigned long) (millis()-mow.stat.t) > 60*1e3){
  //     //DEBUG_PRINTLN("No valid status.");  
  //     return 1; // No data recieved.
  //   }   

  // if(mow.stat.data == CHARGING){
  //     mowStateDesired = S_CHARGING;
  // }

  // if(mow.actCutTime.data == -1 
  //   || (unsigned long) (millis()-mow.actCutTime.t) > 60*1e3) {
  //     //Serial.println("actCutTime no new data.");
  //     return 1;
  // }

  // if(mow.stat.data == MOWING && mow.actCutTime.data > 2){
  //   mowStateDesired = S_MOWING_NOW;
  // }
  return 0;
}

int get_mow_status(){ // Get status value. -1 = not valid data.
  if( time_valid(mow.stat.t) ) return mow.stat.data;
  return -1;
}
int get_mow_actCutTime(){ // Actual cutTime. -1 = not valid data.
  if( time_valid(mow.actCutTime.t) ) return mow.actCutTime.data;
  return -1;
}

int time_valid(unsigned long t){
  return (unsigned long)(millis()-t) <= 60*1e3;
}
#endif // MOWER_H