/*
 * rgb.h
 * 
 * Control a RGB led via three analog outputs.
 * 
 */

#ifdef RGB_SUPPORT

/* It is correct that we use Dx pins here.
 * analogWrite() generates PWM signals on Dx pins.
 * 
 * NOTE: Not all pins are PWM capable.
 *       See Arduino reference pages.
 */
#define RGB_RED_PORT   D1
#define RGB_GREEN_PORT D2
#define RGB_BLUE_PORT  D3

class RgbAnalog : public Action {

  private:
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
    void setRGB( byte r, byte g, byte b);
};

#endif
