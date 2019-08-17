/*
 * ws2801.h
 * 
 * Control WS2801 LED strip.
 * 
 */

#ifdef WS2801_SUPPORT

/* Number of LEDs of the LED strip.
 * This is the default value. It can be changed via set command.
 * The value is saved in the EEPROM and survives a reboot.
 */
#define WS2801_LED_COUNT 4

#define WS2801_DATA_PIN  D4
#define WS2801_CLOCK_PIN D5

typedef struct ws2801_config_t {
  byte ledCount;

  /* checksum MUST BE LAST */
  checksum_t checksum;
} ws2801_config_t;

class WS2801 : public Action {

  private:
    unsigned int configLocation;
    ws2801_config_t ws2801Config;
    byte red;
    byte green;
    byte blue;

  public:
    size_t setup(unsigned int eepromLocation);
    const char *getName();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

  private:
    void setWS2801( byte r, byte g, byte b);
    void serialByteOut( byte b);
};

#endif
