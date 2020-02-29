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

CC1101 receiver;

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

    memset( (void*)&tpms433Config, 0, sizeof(tpms433Config));
    tpms433Config.checksum = computeChecksum( &tpms433Config, sizeOfConfig);

    EEPROM.put( eepromLocation, tpms433Config);
  }

  /* Clear sensor array */
  memset( (void*)&sensor[0], 0, sizeof(tpms433_sensor_t) * (TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS));
  
  /* Configure receiver, setup ISR and start receiving */
  receiver.reset();

  return (size_t)sizeOfConfig;
}

/* This method is called periodically on USB receive timeout which is 20msec.
 *  
 * Check if the receiver has data available and decode them.
 */
void Tpms433::timeout()
{
  byteArray_t data;
  byte id;
  byte i;
    
  if ( receiver_state == STATE_DATA_AVAILABLE)
  {
    /* Returns true is decoding went fine and checksum was ok. */
    if( decode_tpms( timings, timings_len, first_edge_state, &data)) {

      id = find_sensor( &data);

      for( i = 0; i < TPMS_433_ID_LENGTH; i++) {
        sensor[id].sensorId[i] = data.bytes[i];
      }
      sensor[id].press_bar = (float)data.bytes[5] * 1.38 / 100; //pressure in bar
      sensor[id].temp_c    = data.bytes[6] - 50;
    }

    init_buffer();
    receiver_state = STATE_IDLE;
  }
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
    for( byte i = 0; i < TPMS_433_NUM_SENSORS; i++) {
      sensor[i].press_bar = (180 + random(40)) / 100.0;
      sensor[i].temp_c    = (-100 + random(900)) / 10;
    }
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
  char hexstr[ 2 * TPMS_433_ID_LENGTH +1];

  /* FL: temp press FR: temp press RL: temp press RR: temp press */
  
  for( byte i = 0; i < TPMS_433_NUM_SENSORS; i++) {
    id2hex( sensor[i].sensorId, hexstr );
    hexstr[ 2 * TPMS_433_ID_LENGTH ] = '\0';
    
    sendMoreData( String("ID:")+hexstr+" "+String( sensor[i].temp_c,1)+" "+String(sensor[i].press_bar,2));
  }
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
  char hexstr[ 2 * TPMS_433_ID_LENGTH +1];

  for( byte i = 0; i < TPMS_433_NUM_SENSORS; i++) {
    id2hex( tpms433Config.sensorId[i], hexstr );
    hexstr[ 2 * TPMS_433_ID_LENGTH ] = '\0';
    sendMoreData( hexstr);
  }
}

/*
 * This function is called to set configuration values for this action.
 * The configuration is stored in EEPROM and survives a reboot.
 * 
 * The CMU executes: usbget -d <device> -s TPMS -p "0=xxxx" -p "1=xxxx" -p "2=xxxx" -p "3=xxxx"
 */
void Tpms433::setConfig()
{
  boolean configHasChanged = false;
    
  const char *id = getStringParam( "0" );

  if( id != NULL) {
    hex2id( id, tpms433Config.sensorId[0]);
    configHasChanged = true;
  }

  id = getStringParam( "1" );
  if( id != NULL) {
    hex2id( id, tpms433Config.sensorId[1]);
    configHasChanged = true;
  }

  id = getStringParam( "2" );
  if( id != NULL) {
    hex2id( id, tpms433Config.sensorId[2]);
    configHasChanged = true;
  }

  id = getStringParam( "3" );
  if( id != NULL) {
    hex2id( id, tpms433Config.sensorId[3]);
    configHasChanged = true;
  }

  if( configHasChanged ) {

      tpms433Config.checksum = computeChecksum( &tpms433Config, sizeof(tpms433Config));
      EEPROM.put( configLocation, tpms433Config);
  }  
}

/* ***************** PRIVATE ******************* */

/*
 * Convert 4 bytes sensor ID to 8 hex chars
 */
void Tpms433::id2hex( byte b[], char hex[]) {

  byte ci = 0;
  byte v;
  char ch;
  
  for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
      v = b[i];
 
      ch = ((v >> 4) & 0x0f);
      ch += (ch > 9) ? ('a'-10) : '0';
      hex[ ci++ ] = ch;
      
      ch = v & 0x0f;
      ch += (ch > 9) ? ('a'-10) : '0';
      hex[ ci++ ] = ch;
  }
}

/*
 * Convert a hex string into 4 sensor ID bytes.
 */
void Tpms433::hex2id( char hex[], byte b[]) {

  byte ci = 0;
  byte v;
  char ch;
  
  for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
    ch = hex[ci++];
    if( !ch) break;
    v = (ch - ((ch > '9') ? 'a'-10 : '0')) << 4;

    ch = hex[ci++];
    if( !ch) break;
    v |= ch - ((ch > '9') ? 'a'-10 : '0');
    
    b[i] = v;
  }
}

byte Tpms433::find_sensor( byteArray_t *data)
{
  byte last = TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS -1;
  byte id;

  /* First check whether this is one of your predefined sensors */
  for( id = 0; id < TPMS_433_NUM_SENSORS; id++) {
    if( match_id( tpms433Config.sensorId[id], data)) {
      return id;
    }
  }

  if( TPMS_433_EXTRA_SENSORS > 0) {
    /* Check for existence */
    for( id = TPMS_433_NUM_SENSORS; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {
      if( match_id( sensor[id].sensorId, data)) {
        return id;
      }
    }
  
    /* Check for empty slot */
    for( id = TPMS_433_NUM_SENSORS; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {
      if( is_empty( sensor[id].sensorId)) {
        return id;
      }
    }
  }

  return last;  
}

bool Tpms433::match_id( byte b[], byteArray_t *data) 
{
  byte i;

  for( i = 0; i < TPMS_433_ID_LENGTH; i++) {
    if( b[i] != get_byte( data, i)) {
      return false;
    }
  }

  return true;
}

bool Tpms433::is_empty( byte b[])
{
  byte i;

  for( i = 0; i < TPMS_433_ID_LENGTH; i++) {
    if( b[i] != 0) {
      return false;
    }
  }

  return true;  
}

#endif
