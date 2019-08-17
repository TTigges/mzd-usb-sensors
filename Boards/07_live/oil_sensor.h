/*
 * oil_sensor.h
 * 
 * Engine oil pressure and temperature.
 * 
 */

#ifdef OIL_SUPPORT

/* Analog input pins (A/D converter) */
#define OIL_T_PIN A0
#define OIL_P_PIN A5

class OilSensor : public Action {

  private:
    float oilTemp  = 0.0;
    float oilPress = 0.0;

  public:
    size_t setup(unsigned int eepromLocation);
    const char *getName();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

  private:
    float calcTemp(float tPinValue);
    float calcPress(float pPinValue);
};

#endif
