/*
 * tmps_433.h
 * 
 * Reading 433Mhz TPMS sensors.
 * 
 * Supported functions:
 * ====================
 * 
 * Query data: Yes
 *   send tmps temperature and pressure
 *   
 * Query config: Yes
 *   send smac for all sensors
 *     Keys: FL, FR, RL, RR
 *   
 * Set config: Yes
 *   set smac for all sensors
 *     Keys: FL, FR, RL, RR
 */

#ifdef TPMS_433_SUPPORT

/*
 * This function is called once when the MC boots up.
 * It reads configuration data from EEPROM and returns the size of the data read.
 * 
 * It should also perform any one time initialization of the sensor code.
 * 
 * IT IS IMPORTANT TO RETURN THE CORRECT SIZE OF THE CONFIGURATION DATA STORED 
 * IN EEPROM AS THE VALUE IS USED TO COMPUTE THE EEPROM START LOCATION OF THE 
 * NEXT MODULE!
 */
size_t Tpms433::setup(unsigned int eepromLocation)
{
  size_t sizeOfConfig = sizeof(tpms433Config);
  
  /* Remember where the configuration is stored in EEPROM */
  configLocation = eepromLocation;
  
  EEPROM.get( eepromLocation, tpms433Config);

  if( tpms433Config.checksum != computeChecksum( &tpms433Config, sizeOfConfig)) {
    
    /* TODO: Set default values in case the checksum is wrong and there is no valid data in the EEPROM */
    
    strcpy( tpms433Config.smac[FRONT_LEFT],  "000000000000");
    strcpy( tpms433Config.smac[FRONT_RIGHT], "111111111111");
    strcpy( tpms433Config.smac[REAR_LEFT],   "222222222222");
    strcpy( tpms433Config.smac[REAR_RIGHT],  "333333333333");

    tpms433Config.checksum = computeChecksum( &tpms433Config, sizeOfConfig);

    EEPROM.put( eepromLocation, tpms433Config);
  }

  /* TODO: Initialilzation code goes here */

  return (size_t)sizeOfConfig;
}

/*
 * This function returns the name of this action module.
 * The name is used to identify this module for requests sent by the CMU.
 * 
 * usbget -d <device> -l
 * usbget -d <devcie> -q TMPS
 * usbget -d <devcie> -c TPMS
 * usbget -d <devcie> -s TPMS -p <param1> -p <param2> ...
 * 
 */
const char *Tpms433::getName()
{
  return "TPMS";
}

/*
 * This function is called before sendData() is called in response to a CMU query.
 * 
 * It is responsible for collecting and/or preprocessing TPMS data from the sensors.
 */
void Tpms433::getData()
{
  if( actionSimulate)
  {
    /* TESTING ONLY, GENERATE RANDOM TIRE PRESSURE BETWEEN 1.8 AND 2.2 
     * Pressure: Two digits right of the dot.
     * Temperature: One digit right of the dot.
     */
    tpmsPress[FRONT_RIGHT] = (180 + random(40)) / 100.0;
    tpmsPress[FRONT_LEFT] = (180 + random(40)) / 100.0;
    tpmsPress[REAR_RIGHT] = (180 + random(40)) / 100.0;
    tpmsPress[REAR_LEFT] = (180 + random(40)) / 100.0;

    tpmsTemp[FRONT_RIGHT] = (-100 + random(900)) / 10.0;
    tpmsTemp[FRONT_LEFT] = (-100 + random(900)) / 10.0;
    tpmsTemp[REAR_RIGHT] = (-100 + random(900)) / 10.0;
    tpmsTemp[REAR_LEFT] = (-100 + random(900)) / 10.0;
  }
  else 
  {
    /* TODO: Real implementation goes here */
    
    /* Use configuration data (sensor IDs) to filter sampled data */
    
  }
}

/*
 * This function is called to send TPMS data to the CMU.
 * 
 * The CMU executes: usbget -d <device> -q TPMS
 * which causes this function to be called.
 */
void Tpms433::sendData()
{
  String data;

  /* FL: temp press FR: temp press RL: temp press RR: temp press */
  data = "FL: "+String(tpmsTemp[FRONT_LEFT],1)+" "+String(tpmsPress[FRONT_LEFT],2)
       +" FR: "+String(tpmsTemp[FRONT_RIGHT],1)+" "+String(tpmsPress[FRONT_RIGHT],2)
       +" RL: "+String(tpmsTemp[REAR_LEFT],1)+" "+String(tpmsPress[REAR_LEFT],2)
       +" RR: "+String(tpmsTemp[REAR_RIGHT],1)+" "+String(tpmsPress[REAR_RIGHT],2);
  
  sendMoreData( data);
}

/*
 * This function is called to send configuration values to the CMU.
 * In our case we are sending the sensor IDs for all 4 tires.
 * 
 * The CMU executes: usbget -d <device> -c TPMS
 * which causes this function be be called.
 */
void Tpms433::sendConfig()
{
  sendMoreData( "FL="+String(tpms433Config.smac[FRONT_LEFT]));
  sendMoreData( "FR="+String(tpms433Config.smac[FRONT_RIGHT]));
  sendMoreData( "RL="+String(tpms433Config.smac[REAR_LEFT]));
  sendMoreData( "RR="+String(tpms433Config.smac[REAR_RIGHT]));
}

/*
 * This function is called to set configuration values for this action.
 * The configuration is stored in EEPROM and survives a reboot.
 * 
 * The CMU executes usbget -d <device> -s TPMS -p "FL=xxxx" -p "FR=xxxx" -p "RL=xxxx" -p "RR=xxxx"
 */
void Tpms433::setConfig()
{
  boolean configHasChanged = false;
  
  /* TODO: Get new config data */
  
  const char *config_FL = getStringParam( "FL" );
  const char *config_FR = getStringParam( "FR" );
  const char *config_RL = getStringParam( "RL" );
  const char *config_RR = getStringParam( "RR" );

  if( config_FL != NULL) {
      /* TODO: set config data here */
      configHasChanged = true;
  }

  if( config_FL != NULL) {
      /* TODO: set config data here */
      configHasChanged = true;
  }

  if( config_FL != NULL) {
      /* TODO: set config data here */  
      configHasChanged = true;
  }

  if( config_FL != NULL) {
      /* TODO: set config data here */  
      configHasChanged = true;
  }

  if( configHasChanged ) {

      tpms433Config.checksum = computeChecksum( &tpms433Config, sizeof(tpms433Config));
      EEPROM.put( configLocation, tpms433Config);
  }  
}

#endif
