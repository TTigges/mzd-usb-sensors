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
    /* No valid configuration found, clear and save empty config */
    memset( (void*)&tpms433Config, 0, sizeof(tpms433Config));
    tpms433Config.checksum = computeChecksum( &tpms433Config, sizeOfConfig);

    EEPROM.put( eepromLocation, tpms433Config);
  }

  next_score_adj = millis() + 1000 * TPMS_433_SCORE_TIMEOUT_s;

  /* Clear sensor array */
  set_sensor_IDs_from_config();
  
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
  unsigned long now = millis();
  byteArray_t data;
  byte id;
  byte i;

  /* Lower score of all sensors periodically */
  if( now > next_score_adj) {
    next_score_adj = now + 1000 * TPMS_433_SCORE_TIMEOUT_s;

    for( id = 0; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {
      if( sensor[id].score > 0) {
        sensor[id].score--;
      }
    }
  }

  if ( receiver_state == STATE_DATA_AVAILABLE)
  {
    /* Returns true is decoding went fine and checksum was ok. */

    if( decode_tpms( timings, timings_len, first_edge_state, &data)) {

      /* We can enable receiver state here because the timings buffer
       * is not used anymore.
       */
      init_buffer();
      receiver_state = STATE_IDLE;

      id = find_sensor( &data);
      
      /* find_sensor may return -1 if there are no extra slots available */
      if( id >= 0) {
        sensor[id].press_bar = (float)data.bytes[5] * 1.38 / 100; //pressure in bar
        sensor[id].temp_c    = (float)data.bytes[6] - 50;
        //sensor[id].lastupdated = now;

        if( sensor[id].score >= TPMS_433_SCORE_MAX - TPMS_433_SCORE_ADD) {
          sensor[id].score = TPMS_433_SCORE_MAX;
        } else {
          sensor[id].score += TPMS_433_SCORE_ADD;
        }

        /* We need to sort only after new data was inserted */
        sort_sensors( id);
        
#ifdef DISPLAY_SUPPORT
        UpdateDisplay( sensor);
#endif
      }
    } else {
      init_buffer();
      receiver_state = STATE_IDLE;
    }
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
      sensor[i].temp_c    = (-100 + random(900)) / 10.0;
      //sensor[i].lastupdated = millis();
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
    
    sendMoreDataStart();
    Serial.print(F("ID="));
    Serial.print(hexstr);
    Serial.print(F(" T="));
    Serial.print(sensor[i].temp_c,1);
    Serial.print(F(" P="));
    Serial.print(sensor[i].press_bar,2);
    sendMoreDataEnd();
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

  for( byte i = 0; i < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; i++) {
    //id2hex( tpms433Config.sensorId[i], hexstr );
    id2hex( sensor[i].sensorId, hexstr );
    hexstr[ 2 * TPMS_433_ID_LENGTH ] = '\0';
    
    sendMoreDataStart();
    Serial.print(i);
    //Serial.print(F("="));
    //Serial.print(hexstr);
    Serial.print(F(" ID="));
    Serial.print(hexstr);
    Serial.print(F(" T="));
    Serial.print(sensor[i].temp_c,1);
    Serial.print(F(" P="));
    Serial.print(sensor[i].press_bar,2);
    //Serial.print(F(" U="));
    //Serial.print(sensor[i].lastupdated);
    Serial.print(F(" S="));
    Serial.print(sensor[i].score);
    sendMoreDataEnd();
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
      set_sensor_IDs_from_config();
  }  
}

/* ***************** PRIVATE ******************* */

/*
 * Convert 4 bytes sensor ID to 8 hex chars
 */
void static Tpms433::id2hex( byte b[], char hex[]) {

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
void static Tpms433::hex2id( char hex[], byte b[]) {

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

/* 
 *  Find a sensor that matched id in 'data'
 *  
 *  First try to find a slot with the same sensor ID.
 *  If none was found, try to find an empty slot.
 *  If that fails as well, replace the last sensor.
 */
int Tpms433::find_sensor( byteArray_t *data)
{
  byte id;

  /* First check whether we have seen this sensors before */
  for( id = 0; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {
    if( match_id( id, data)) {
      return id;
    }
  }
  
  if( TPMS_433_EXTRA_SENSORS > 0) {

    /* Check for empty slot */
    for( id = 0; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {
      if( is_empty( id)) {

        /* Copy sensor id */
        for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
          sensor[id].sensorId[i] = data->bytes[i];
        }

        return id;
      }
    }
  }

  if( TPMS_433_EXTRA_SENSORS == 0) {
    return -1;
    
  } else { /* Replace last sensor */
    id = TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS -1;

    /* Clear sensor data */
    memset( (void*)&sensor[id], 0, sizeof(tpms433_sensor_t));

    /* Copy sensor id */
    for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
      sensor[id].sensorId[i] = data->bytes[i];
    }
    
    return id;
  }
}

/*
 * Check if a sensor matches id (first bytes in 'data')
 */
bool Tpms433::match_id( byte id, byteArray_t *data) 
{
  byte i;

  for( i = 0; i < TPMS_433_ID_LENGTH; i++) {
    if( sensor[id].sensorId[i] != get_byte( data, i)) {
      return false;
    }
  }

  return true;
}

/* 
 * Check if a sensor slot is empty.
 * ID = 0x00000000
 */
bool Tpms433::is_empty( byte id)
{
  byte i;

  for( i = 0; i < TPMS_433_ID_LENGTH; i++) {
    if( sensor[id].sensorId[i] != 0) {
      return false;
    }
  }

  return true;  
}

/* 
 * Check for empty config 
 */
bool Tpms433::empty_config()
{
  for( byte id = 0; id < TPMS_433_NUM_SENSORS; id++) {
    for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
      if( tpms433Config.sensorId[id][i] != 0) {
        return false;
      }
    }
  }
  
  return true;
}

/*
 * Copy sensor s(ource) to t(arget)
 */
void Tpms433::copy_sensor( tpms433_sensor_t *t, tpms433_sensor_t *s)
{
  memcpy( t, s, sizeof(tpms433_sensor_t));
}

/*
 * Clear all sensors and copy sensor IDs from EEPROM config.
 */
void Tpms433::set_sensor_IDs_from_config()
{
  byte id;
  
  for( id = 0; id < TPMS_433_NUM_SENSORS; id++) {

    /* Clear sensor data */
    memset( (void*)&sensor[id], 0, sizeof(tpms433_sensor_t));

    /* Copy sensor id */
    for( byte i = 0; i < TPMS_433_ID_LENGTH; i++) {
      sensor[id].sensorId[i] = tpms433Config.sensorId[id][i];
    } 
  }

  for( id = TPMS_433_NUM_SENSORS; id < TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS; id++) {

    /* Clear sensor data */
    memset( (void*)&sensor[id], 0, sizeof(tpms433_sensor_t));
  }
}

void Tpms433::sort_sensors( byte top)
{
  tpms433_sensor_t temp;
  byte id;
  byte score;

  /* Where to start sorting */
  byte bottom = empty_config() ? 0 : TPMS_433_NUM_SENSORS;
  byte pos = bottom;
  
  /* A new identified sensor always replaces the sensor with highest slot id.
   * We simply need to move this sensor up to the right place.
   */
  score = sensor[top].score;
  
  for( id = top; id > bottom; id--) {
    if( sensor[id-1].score > score) {
      pos = id;
      break; 
    }
  }

  if( pos < top) {
    /* Move all sensors from top to pos down one slot. */
    copy_sensor( &temp, &sensor[ top]);

    for( id = top; id > pos; id--) {
      copy_sensor( &sensor[id], &sensor[id-1]);
    }

    copy_sensor( &sensor[pos], &temp);
  }
}

#endif
