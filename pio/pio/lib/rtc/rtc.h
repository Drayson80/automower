// RTC MEMORY
extern "C" {
#include "user_interface.h"
}

typedef struct {
  uint32_t count;
} rtcStore;

rtcStore rtcMem;

#define RTCMEMORYSTART 65
#define MAXHOUR 5 // number of hours to deep sleep for

void readRTCMemory() {
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));
  yield();
}

void writeRTCMemory(uint32_t cnt) {
  rtcMem.count = cnt;
  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, 4);
  yield();
}