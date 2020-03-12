/*
 * ws2801.h
 * 
 * Control WS2801 LED strip.
 * 
 */

#ifdef WS2801_SUPPORT

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
    void timeout();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

  private:
    void setWS2801( byte r, byte g, byte b);
    void serialByteOut( byte b);
};

#endif
