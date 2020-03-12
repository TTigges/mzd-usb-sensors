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

 #define D0   0
 #define D1   1
 #define D2   2
 #define D3   3
 #define D4   4
 #define D5   5
 #define D6   6
 #define D7   7
 #define D8   8
 #define D9   9
 #define D10 10
 #define D11 11
 #define D12 12
 #define D13 13

#endif

/* Enable supported features per platform */

#ifdef REDBEAR_DUO

 #define TPMS_BLE_SUPPORT
 #define OIL_SUPPORT
 #define RGB_SUPPORT
 #define WS2801_SUPPORT

#endif

#ifdef ARDUINO_GENERIC

 #define DISPLAY_SUPPORT
 #define CC1101_SUPPORT
 #define TPMS_433_SUPPORT
 #define OIL_SUPPORT
 #define RGB_SUPPORT
// #define WS2801_SUPPORT

#endif

/* **************************************************************** */

#ifdef DISPLAY_SUPPORT

 #define DISPLAY_I2C_ADDRESS 0x3C

#endif

/*
 * Configuration of all modules.
 * Make sure there is no duplication of ports !
 * 
 */
#ifdef CC1101_SUPPORT
  /*
   * Pin assignment
   */
 #define CC1101_CS       D10 // Chip Select pin
 #define CC1101_RXPin    D2  // GDO2
 #define CC1101_TXPin    D9  // wlowi: GDO0 is also TX pin
 #define CC1101_CDPin    D9  // wlowi: GDO0 carrier detect pin

#endif

#ifdef OIL_SUPPORT

 /* Analog input pins (A/D converter) */
 #define OIL_T_PIN       A0
 #define OIL_P_PIN       A2

#endif

#ifdef RGB_SUPPORT

 /* It is correct that we use Dx pins here.
  * analogWrite() generates PWM signals on Dx pins.
  * 
  * NOTE: Not all pins are PWM capable.
  *       See Arduino reference pages.
  */
  
 #define RGB_RED_PORT     D4
 #define RGB_GREEN_PORT   D5
 #define RGB_BLUE_PORT    D7

#endif

#ifdef WS2801_SUPPORT

 /* Number of LEDs of the LED strip.
  * This is the default value. It can be changed via set command.
  * The value is saved in the EEPROM and survives a reboot.
  */
 #define WS2801_LED_COUNT 4

 #define WS2801_DATA_PIN  D4
 #define WS2801_CLOCK_PIN D5
 
#endif
