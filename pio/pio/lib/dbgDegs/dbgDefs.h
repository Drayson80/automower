// DEBUG
#define DEBUG_THIS
#define DBG_OUTPUT_PORT Serial // Still needed for printf
#ifdef DEBUG_THIS
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif
