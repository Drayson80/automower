// RTC MEMORY
extern "C" {
#include "user_interface.h"
}

typedef struct {
  uint32_t count;   // Timer at mowing start
  uint32_t bMowing; // Boolean: True if mowing.
} rtcStore;

rtcStore rtcMem;

const uint32_t buckets=(sizeof(rtcMem)/4);

#define RTCMEMORYSTART 65
#define RTCMEMORYLEN 128

void readRTCMemory() {
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));
  yield();
}

void writeRTCMemory(uint32_t cnt, uint32_t bMowing) {
  rtcMem.count = cnt;
  rtcMem.bMowing = bMowing;
  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, buckets*4);
  yield();
}