/*
 * ws2801.ino
 * 
 * Control WS2801 LED strip.
 * 
 * Supported functions:
 * ====================
 * 
 * Query data: No
 *   
 * Query config: Yes
 *   send intensity per color
 *     Keys: R, G, B
 *   send number of LEDs of the strip
 *     Key: LEDS
 *   
 * Set config: Yes
 *   set intensity per color
 *     Keys: R, G, B
 *   set number of LEDs of the strip
 *     Key: LEDS
 * 
 */

#ifdef WS2801_SUPPORT

size_t WS2801::setup(unsigned int eepromLocation)
{
  size_t sizeOfConfig = sizeof(ws2801Config);
  
  /* Remember where the configuration is stored in EEPROM */
  configLocation = eepromLocation;
  
  EEPROM.get( eepromLocation, ws2801Config);

  if( ws2801Config.checksum != computeChecksum( &ws2801Config, sizeOfConfig)) {
    /* Set default values in case the checksum is wrong and there is no valid date in the EEPROM */
    ws2801Config.ledCount = WS2801_LED_COUNT;

    ws2801Config.checksum = computeChecksum( &ws2801Config, sizeOfConfig);
    EEPROM.put( eepromLocation, ws2801Config);
  }

  pinMode(WS2801_CLOCK_PIN, OUTPUT);
  pinMode(WS2801_DATA_PIN, OUTPUT);

  digitalWrite(WS2801_CLOCK_PIN, HIGH);
  digitalWrite(WS2801_DATA_PIN, LOW);

  setWS2801( 0, 0, 0);

  return (size_t)sizeOfConfig;
}

const char *WS2801::getName()
{
  return "WS2801";
}

void WS2801::getData()
{
  /* Nothing to do */
}

void WS2801::sendData()
{
  /* Not supported */
}

void WS2801::sendConfig()
{
  sendMoreData( "LEDS="+String(ws2801Config.ledCount));
  sendMoreData( "R="+String(red));
  sendMoreData( "G="+String(green));
  sendMoreData( "B="+String(blue));
}

void WS2801::setConfig()
{
  byte leds = getIntParam("LEDS", 0);

  if( leds > 0) {
    ws2801Config.ledCount = leds;

    ws2801Config.checksum = computeChecksum( &ws2801Config, sizeof(ws2801Config));
    EEPROM.put( configLocation, ws2801Config);
  }
  
  setWS2801( getIntParam("R", 0), getIntParam("G", 0), getIntParam("B", 0));
}


/* ***************** PRIVATE *************************************************/


void WS2801::setWS2801( byte r, byte g, byte b)
{
  red = r;
  green = g;
  blue = b;

  digitalWrite( WS2801_CLOCK_PIN, LOW);
  delayMicroseconds(1000); /* WS2801 requires at least 500usec. */

  for( int i=0; i<ws2801Config.ledCount; i++) {
    serialByteOut( r);
    serialByteOut( g);
    serialByteOut( b);
  }
}

/* Output one byte using DATA_BIT and CLOCK_BIT.
 */
void WS2801::serialByteOut( byte b) 
{
  for( int bn = 7; bn >= 0; bn--)
  {
    digitalWrite( WS2801_CLOCK_PIN, LOW);
    digitalWrite( WS2801_DATA_PIN, (b & bit(bn)) ? HIGH : LOW);
    delayMicroseconds( 10);
    digitalWrite( WS2801_CLOCK_PIN, HIGH);
    delayMicroseconds( 10);
  }
}

#endif
