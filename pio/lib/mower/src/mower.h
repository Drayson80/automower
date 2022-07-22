#ifndef MOWER_H
#define MOWER_H
// includes
#include <serial.h> 
#include <rtc.h>

// definitions
#define CHARGING 1014
#define IN_CHARGER(x)  x>=1014 && x<=1024 // 1024>=x>=1014 => mower in charger
#define START_MOWER 1012
#define MOWING 1002 
// Above is in decimal and should not be in hex.
// Readable code macros
#define IS_CHARGING IN_CHARGER(get_mow_status())
#define IS_MOWING get_mow_status() >= 1002 && get_mow_status() <= 1008  // 1008>=x>=1002 => mower is mowing

//#define MOWING 0xEA03 // klipper (wrong endian?!)
#define MOWING1 1004 // klipper
#define STOP 1044
#define SEARCHING 1042
#define PINCODE 0xE803 // enter pin code, wrong endian
#define CHASSISOFF 0x1404 // Removed top chassis, wrong endian
#define YES_TO_START 0x1c04 // push yes to start on screen, wrong endian
#define PARK_IN_STATION 0x2004 // When parked in station and option choosed to stay in station, wrong endian
// 1204 2804 searching, wrong endian
// 1804 docking, wrong endian

// First byte should always be 0x0f omitting that
#define STATUSMOWER 0x01f1
#define READSECONDS 0x36b0
#define READMINUTE 0x36b1
#define READHOUR 0x36b2
#define CURRENTMOWINGTIME 0x0037
#define CHARGETIME 0x1ec
#define BATCAPUSED 0x2ee0
#define OPSTATUS 0x0138
#define OPPROZENT 0x0134
#define OPREF 0x0137
#define BATCAPMA 0x01eb
#define BATCAPMAH 0x01ef
#define BATLASTCHRG 0x0234
#define BATVOLTAGE 0x2ef4

			// 	"6" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Linker Radmotor blockiert"}
			// 	"12" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Kein Schleifensignal"}
			// 	"16" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Außerhalb"}
			// 	"18" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Niedrige Batteriespannung"}
			// 	"26" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Ladestation blockiert"}
      //  28 behöver laddas manuellt.
			// 	"34" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Mäher hochgehoben"}
			// 	"52" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Kein Kontakt zur Ladestation"}
			// 	"54" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Pin abgelaufen"}
			// 	"1000" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Aus LS ausfahren"}
			// 	"1002" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Mähen"}
			// 	"1006" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Mähwerk starten"}
			// 	"1008" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Mähwerk gestartet"}
			// 	"1012" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Signal starte Mähwerk"}
			// 	"1014" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Laden"}
			// 	"1016" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: in LS wartend"}
			// 	"1024" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: aus LS einfahren"}
			// 	"1036" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Viereckmodus"}
			// 	"1038" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Festgefahren"}
			// 	"1040" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Kollision"}
			// 	"1042" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Suchen"}
			// 	"1044" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Stop"}
			// 	"1048" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Andocken"}
			// 	"1050" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: aus LS ausfahren"}
			// 	"1052" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Fehler"}
			// 	"1056" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Wartet (Modus Manuell/Home)"}
			// 	"1058" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Begrenzung folgen"}
			// 	"1060" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: N-Signal gefunden"}
			// 	"1062" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Festgefahren"}
			// 	"1064" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Suchen"}
			// 	"1070" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Suchschleife folgen"}
			// 	"1072" {set automower_status "[clock format [clock sec] -format %H:%M:%S]: Schleife folgen"}
			// }

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
  int8_t hour = 0;
  int8_t minute = 0;
  int8_t seconds = 0;
} ;

// Mower status as known
struct mowDataStruct {
  mowTime mowClock;
  genDataStruct stat;
  genDataStruct actCutTime;
  genDataStruct actMode;
  genDataStruct chargeTime;
  genDataStruct batCapUsed;
  genDataStruct opStatus;
  genDataStruct opProcent;
  genDataStruct opRef;
  genDataStruct batCapMah;
  genDataStruct batCapMa;
  genDataStruct batLstChrgTimeMin;
  genDataStruct batVoltage;
} mow;


struct allcommands {
  uint8_t R_STATUS[5] = {0xf,0x1,0xf1,0x0,0x0};
  uint8_t R_SEKUNDE[5] = {0xf,0x36,0xb0,0x0,0x0};
  uint8_t R_MINUTE[5] = {0xf,0x36,0xb1,0x0,0x0};
  uint8_t R_STUNDE[5] = {0xf,0x36,0xb2,0x0,0x0};
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
  uint8_t R_MAEHZEIT[5] = {0xf,0x0,0x37,0x0,0x0};
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
void incrementInternalClock(); // Update our mirror clock of that in automower.
void checkIfReset(unsigned long *tOffs, uint8_t *mowState, uint8_t *mowStateDesired);
void syncInternalClock();
void backupRAM(unsigned long time); // write data to ram backup

// variables
unsigned long lastClockSync=0; // time when internal Clock was synced with mower clock.

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
    
    case BATCAPMA:
      mow.batCapMa.t = t;
      mow.batCapMa.data = (int16_t)respData;
      debugD("Battery cap MA: %d.\r\n", mow.batCapMa.data);
      break;
 
    case BATCAPMAH:
      mow.batCapMah.t = t;
      mow.batCapMah.data = (int16_t)respData;
      debugD("Battery cap MAH: %d.\r\n", mow.batCapMah.data);
      break;
 
    case BATLASTCHRG:
      mow.batLstChrgTimeMin.t = t;
      mow.batLstChrgTimeMin.data = respData;
      debugD("Battery last charge time min: %u.\r\n", mow.batLstChrgTimeMin.data);
      break;
 
    case BATCAPUSED:
      mow.batCapUsed.t = t;
      mow.batCapUsed.data = respData;
      debugD("Battery cap used: %u.\r\n", mow.batCapUsed.data);
      break;

    case BATVOLTAGE:
      mow.batVoltage.t = t;
      mow.batVoltage.data = respData;
      debugD("Battery voltage: %u.\r\n", mow.batVoltage.data);
      break;

    case OPPROZENT:
      mow.opProcent.t = t;
      mow.opProcent.data = respData;
      debugD("Op procent: %u.\r\n", mow.opProcent.data);
      break;

    case OPREF:
      mow.opRef.t = t;
      mow.opRef.data = respData;
      debugD("Op ref: %u.\r\n", mow.opRef.data);
      break;

    case OPSTATUS:
      mow.opStatus.t = t;
      mow.opStatus.data = respData;
      debugD("Op status: %u.\r\n", mow.opStatus.data);
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

void incrementInternalClock(){ // Update our mirror clock of that in automower.
  if ( (unsigned long)(millis()-mow.mowClock.t) >= 1000 ) {
    //debugV("Update clock. %u:%u:%u", mow.mowClock.hour, mow.mowClock.minute, mow.mowClock.seconds);
    if ( ++mow.mowClock.seconds == 60) {
      mow.mowClock.minute++;
      mow.mowClock.seconds = 0;
    }
    if ( mow.mowClock.minute == 60 ) {
      mow.mowClock.hour++;
      mow.mowClock.minute = 0;
    }
    if ( mow.mowClock.hour == 24) mow.mowClock.hour = 0;

    mow.mowClock.t = millis(); // Update last time updated.
    debugV("New clock. %u:%u:%u", mow.mowClock.hour, mow.mowClock.minute, mow.mowClock.seconds);
  }

}

struct	rst_info	*rtc_info;

void checkIfReset(unsigned long *tOffs, uint8_t *mowState, uint8_t *mowStateDesired){
  // Check rtc memory to determine if we restarted from a reset.
  rtc_info = system_get_rst_info(); // Get reset cause
  // TODO: use in combination with info from ram.
  
  if(rtcMem.mowTime_b >= 0 
    && rtcMem.mowState_b == S_MOWING_NOW
    && rtcMem.mowStateDesired_b == S_MOWING_NOW) {
      // Restore values if reset detected
      *tOffs = rtcMem.mowTime_b;
      *mowState = (uint8_t) rtcMem.mowState_b;
      *mowStateDesired = (uint8_t) rtcMem.mowStateDesired_b;
      rtcMem.reset_counter++; // Count up reset counter
    }
}

void syncInternalClock(){
  if( (unsigned long)(millis()-lastClockSync) >= 60*60*1000 || lastClockSync==0 ) { // Synchronize internal clock with mower every hour.
    char resp = sendMowReq(commands.R_STUNDE, sizeof(commands.R_STUNDE));
    if( !resp ) resp = sendMowReq(commands.R_MINUTE, sizeof(commands.R_MINUTE));
    if( !resp ) resp = sendMowReq(commands.R_SEKUNDE, sizeof(commands.R_SEKUNDE));

    if( !resp) lastClockSync = millis(); 
  }
}

void manModeMowReq(){ // Manual mode request
  sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
  yield();
  delay(100);
  //sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
  //delay(250);
  //sendMowReq(commands.W_MODE_MAN, sizeof(commands.W_MODE_MAN));
  //delay(250);

} 

void autoModeMowReq(){ // Auto mode request
  sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
  yield();
  delay(100);
  //sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
  //delay(250);
  //sendMowReq(commands.W_MODE_AUTO, sizeof(commands.W_MODE_AUTO));
  //delay(250);
}

void backupRAM(unsigned long time, uint8_t mowState, rtcStore mem){
  rtcMem.mowTime_b = time;
  rtcMem.mowState_b = (uint32_t) mowState;
  rtcMem.mowStateDesired_b = (uint32_t) mowState;
  writeRTCMemory(); // Write to ram

  debugD("mowTime_b: %lu mowstate_b: %i, mowstatedesired_b: %i", rtcMem.mowTime_b, rtcMem.mowState_b, rtcMem.mowStateDesired_b);
}

#endif // MOWER_H