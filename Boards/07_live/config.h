/*
 * config.h
 * 
 * Some platform dependent defines to make Redbear Duo more Arduino like
 *
 */


/* Select ONLY ONE of... */

//#define REDBEAR_DUO
//#define ARDUINO_PRO
#define ARDUINO_NANO

/* END PLATFORM SELECTION */



#ifdef REDBEAR_DUO

#if defined(ARDUINO) 
  SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define LED_BUILTIN D7

#ifndef true
 #define true TRUE
 #define false FALSE
#endif

#endif

#ifdef ARDUINO_PRO
 #define ARDUINO_GENERIC
#endif

#ifdef ARDUINO_NANO
 #define ARDUINO_GENERIC
#endif


#ifdef ARDUINO_GENERIC

 #include "EEPROM.h"

 #define D0 0
 #define D1 1
 #define D2 2
 #define D3 3
 #define D4 4
 #define D5 5
 #define D6 6
 #define D7 7

#endif

/* Enable supported features per platform */

#ifdef REDBEAR_DUO

 #define TPMS_BLE_SUPPORT
 #define OIL_SUPPORT
 #define RGB_SUPPORT
 #define WS2801_SUPPORT

#endif

#ifdef ARDUINO_GENERIC

 #define TPMS_433_SUPPORT
 #define OIL_SUPPORT
 #define RGB_SUPPORT
 #define WS2801_SUPPORT

#endif
