/*
 * rgb.h
 * 
 * Control a RGB led via three analog outputs.
 * 
 */

#ifdef RGB_SUPPORT

class RgbAnalog : public Action {

  private:
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
    void setRGB( byte r, byte g, byte b);
};

#endif
