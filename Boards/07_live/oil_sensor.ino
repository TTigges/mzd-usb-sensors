/*
 * oil_sensor.ino
 * 
 * Engine oil pressure and temperature.
 * 
 * Supported functions:
 * ====================
 * 
 * Query data: Yes
 *   send oil temperature and pressure
 *
 * Query config: No
 *   
 * Set config: No
 * 
 */

#ifdef OIL_SUPPORT

size_t OilSensor::setup(unsigned int eepromLocation)
{
  /* Nothing to do */

  return (size_t)0;
}

void OilSensor::timeout()
{
  /* nothing */  
}

const char *OilSensor::getName()
{
  return "OIL";
}

void OilSensor::getData()
{
  if( actionSimulate)
  {
    /* TESTING ONLY, GENERATE RANDOM OIL PRESSURE AND TEMPERATURE
     * Pressure: Two digits right of the dot.
     * Temperature: One digit right of the dot.
     */
    oilTemp = (-100 + random(1200)) / 10.0; // -10.0 ... 120.0 C
    oilPress = (0 + random(1000)) / 100.0; // 0.00 ... 10.00 bar
  }
  else 
  {
    /* Real implementation goes here */
    float tPinValue = analogRead(OIL_T_PIN);
    oilTemp = calcTemp(tPinValue);

    float pPinValue = analogRead(OIL_P_PIN);
    oilPress = calcPress(pPinValue);
  }
}

void OilSensor::sendData()
{
  /* oiltemp: xx oilpress: yy */
  sendMoreData( "oiltemp: "+String(oilTemp,1)+" oilpress: "+String(oilPress,2));
}

void OilSensor::sendConfig()
{
  /* Not supported */
}

void OilSensor::setConfig()
{
  /* Not supported */
}


/* ***************** PRIVATE *************************************************/

#define OIL_ABSZERO  273.15

#ifdef REDBEAR_DUO
  #define OIL_MAXANALOGREAD 4095.0
  #define OIL_PMINVAL        410.0
  #define OIL_PMAXVAL       3689.0
#endif
#ifdef ARDUINO_GENERIC
  #define OIL_MAXANALOGREAD 1023.0
  #define OIL_PMINVAL        102.0
  #define OIL_PMAXVAL        922.0
#endif

float OilSensor::calcTemp(float tPinValue)
{  
  float T0 = 40 + OIL_ABSZERO;
  float T1 = 150 + OIL_ABSZERO;
  float R0 = 5830;
  float R1 = 316;
  float RV = 1000;
  float VA_VB = tPinValue/OIL_MAXANALOGREAD;
  float B = (T0 * T1) / (T1-T0) * log(R0/R1);
  float RN = RV*VA_VB / (1-VA_VB);

  return T0 * B / (B + T0 * log(RN/R0)) -OIL_ABSZERO;
}

float OilSensor::calcPress(float pPinValue)
{  
  if (pPinValue < OIL_PMINVAL) {
    pPinValue = OIL_PMINVAL;
  }
  else if (pPinValue > OIL_PMAXVAL) {
    pPinValue = OIL_PMAXVAL;
  }
  
  return pPinValue * 0.00305250305250305 - 1.25;
}

#endif
