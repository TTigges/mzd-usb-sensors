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

size_t Tpms433::setup(unsigned int eepromLocation)
{
  size_t sizeOfConfig = sizeof(tpms433Config);
  
  /* Remember where the configuration is stored in EEPROM */
  configLocation = eepromLocation;
  
  EEPROM.get( eepromLocation, tpms433Config);

  if( tpms433Config.checksum != computeChecksum( &tpms433Config, sizeOfConfig)) {
    /* TODO: Set default values in case the checksum is wrong and there is no valid date in the EEPROM */
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

const char *Tpms433::getName()
{
  return "TPMS";
}

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
  }
}

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

void Tpms433::sendConfig()
{
  sendMoreData( "FL="+String(tpms433Config.smac[FRONT_LEFT]));
  sendMoreData( "FR="+String(tpms433Config.smac[FRONT_RIGHT]));
  sendMoreData( "RL="+String(tpms433Config.smac[REAR_LEFT]));
  sendMoreData( "RR="+String(tpms433Config.smac[REAR_RIGHT]));
}

void Tpms433::setConfig()
{
  /* TODO: Get new config data */
//  config_data = getIntParam( /* some config option key */, /* default value */ );

//  if( /* TODO configuration has changed  */ ) {

    /* TODO: set config data here */
    
//    tpms433Config.checksum = computeChecksum( &tpms433Config, sizeof(tpms433Config));
//    EEPROM.put( configLocation, tpms433Config);
//  }  
}

#endif
