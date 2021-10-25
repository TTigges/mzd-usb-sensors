/*
 *
 */

#ifdef DISPLAY_SUPPORT

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

/* Display positions:
 *
 * For single height characters ( display.set1X() )
 * Display: 128x64 pixel
 * Font   : 5x7 pixel  (6x8 including gap)
 * 
 * 8 Lines, 21 characters
 * 
 *    Y (pixel)
 *    0 ............... 127
 * X 0
 *   1
 *   2
 *   3
 *   4
 *   5
 *   6
 *   7
 * 
 */
static const byte x_pos[] = { 1, 65, 1, 65};
static const byte y_pos[] = { 2,  2, 5,  5};

SSD1306AsciiWire display;

/*
 * This function is called once when the MC boots up.
 * It reads configuration data from EEPROM and returns the size of the data read.
 * 
 * It should also perform any one time initialization of the display.
 * 
 * IT IS IMPORTANT TO RETURN THE CORRECT SIZE OF THE CONFIGURATION DATA STORED 
 * IN EEPROM AS THE VALUE IS USED TO COMPUTE THE EEPROM START LOCATION OF THE 
 * NEXT MODULE!
 */
size_t Display::setup(unsigned int eepromLocation)
{
  size_t sizeOfConfig = sizeof(displayConfig);
  
  /* Remember where the configuration is stored in EEPROM */
  configLocation = eepromLocation;
  
  EEPROM.get( eepromLocation, displayConfig);

  if( displayConfig.checksum != computeChecksum( &displayConfig, sizeOfConfig)) {
    
    /* Wrong checksum => No valid configuration found, save default config */
    
    displayConfig.mode   = DISPLAY_DEFAULT_MODE;
    displayConfig.screen = DISPLAY_DEFAULT_SCREEN;
    displayConfig.device = DISPLAY_DEFAULT_DEVICE;
    displayConfig.last_indicator = DISPLAY_DEFAULT_LAST_UPD_sec;
    
    displayConfig.checksum = computeChecksum( &displayConfig, sizeOfConfig);

    EEPROM.put( eepromLocation, displayConfig);
  }
  
  Wire.begin();
  Wire.setClock(400000L);

  Wire.beginTransmission( DISPLAY_I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    
    /* Display found */
    display_present = true;

    switch( displayConfig.device) {
      case DISPLAY_DEVICE_SH1106:
        display.begin(&SH1106_128x64, DISPLAY_I2C_ADDRESS);
        break;

      case DISPLAY_DEVICE_SDD1306:
        display.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
        break;

      default:
        display.begin(&SH1106_128x64, DISPLAY_I2C_ADDRESS);  
    }
    
    display.setFont(Adafruit5x7);
    display.clear();
  
    show_title();
  }

  return (size_t)sizeOfConfig;
}

const char *Display::getName()
{
  return "DISP";
}

/* This is called every 10 msec.
 *
 * This is the only function called from outside world to control the display.
 */
void Display::timeout()
{
  unsigned long now = millis();
  bool full_refresh = false;

  if( !display_present) {
    return;
  }

  if( now >= last_update + DISPLAY_UPDATE_msec) {

    switch( displayConfig.mode) {
      case DISPLAY_MODE_SINGLE:
        /*
         * Display a single screen only.
         */
        current_screen = displayConfig.screen;
        break;
        
      case DISPLAY_MODE_AUTOSWITCH:
        /* 
         * Switch between two screens
         * One screen is DISPLAY_SCREEN_PRESSURE
         * The second screen is defined by value of
         *   displayConfig.display_screen
         */
        change++;
        if( change > DISPLAY_SWITCHTIME) {
          change = 0;
          if( current_screen == displayConfig.screen) {
            current_screen = DISPLAY_SCREEN_PRESSURE;
          } else {
            current_screen = displayConfig.screen;
          }
        }
        break;

      case DISPLAY_MODE_ALL:
        /* 
         *  Switch between all available screens.
         */
        change++;
        if( change > DISPLAY_SWITCHTIME) {
          change = 0;
          current_screen++;
          if( current_screen > DISPLAY_SCREEN_MAX) {
            current_screen=0;
          }
        }
    }
    
    if( current_screen != last_screen)
    {
      display.clear();
      last_screen = current_screen;
      full_refresh = true;
    }
    
    update_display( tpmsReceiver.getSensors(), full_refresh);
    
    /* Update last_update last because it is used in update_display() */
    last_update = now;
  }
}

void Display::getData()
{
  /* Nothing to do */
}

void Display::sendData()
{
  /* Nothing to do */
}

void Display::sendConfig()
{
  sendMoreDataStart();
  Serial.print( F("D="));
  Serial.print( displayConfig.device);
  Serial.print( F(" M="));
  Serial.print( displayConfig.mode);
  Serial.print( F(" S="));
  Serial.print( displayConfig.screen);
  Serial.print( F(" L="));
  Serial.print( displayConfig.last_indicator);
  sendMoreDataEnd();
}

void Display::setConfig()
{    
  displayConfig.device = getIntParam( "D", displayConfig.device);
  displayConfig.mode = getIntParam( "M", displayConfig.mode);
  displayConfig.screen = getIntParam( "S", displayConfig.screen);
  displayConfig.last_indicator = getIntParam( "L", displayConfig.last_indicator);

  if( displayConfig.device > DISPLAY_DEVICE_MAX)
    displayConfig.device = DISPLAY_DEVICE_MAX;
  if( displayConfig.mode   > DISPLAY_MODE_MAX)
    displayConfig.mode   = DISPLAY_MODE_MAX;
  if( displayConfig.screen > DISPLAY_SCREEN_MAX)
    displayConfig.screen = DISPLAY_SCREEN_MAX;
    
  displayConfig.checksum = computeChecksum( &displayConfig, sizeof(displayConfig));
  EEPROM.put( configLocation, displayConfig);
}

/****************** PRIVATE ********************/

void Display::show_title()
{
  display.setFont(Adafruit5x7);
  display.set1X();             // Normal 1:1 pixel scale
  display.setCursor(0, 0);
  display.print( F(" Abarth TPMS Monitor"));
}

void Display::show_last_update()
{
  unsigned int disp = (unsigned int)((millis() - sensor_update_ts) / 1000);

  if( last_disp != disp) {
    last_disp = disp;
    /* Display last update indicator only if last update time > displayConfig.last_indicator
     */
    if( disp >= displayConfig.last_indicator) {
      display.setFont(Adafruit5x7);
      display.set1X();             // Normal 1:1 pixel scale
      display.setCursor( 0, 1);
      display.print( F("Last update"));
      display.clear( 72, 127, 1, 1);
      display.print( disp);
      display.print( F(" sec"));
    } else {      
      display.clear( 0, 127, 1, 1);
    }
  }
}

void Display::update_display( tpms433_sensor_t sensor[], bool full)
{  
  switch( current_screen)
  {
  case DISPLAY_SCREEN_PRESSURE:
    display_pressure( sensor, full);
    show_last_update();
    break;

  case DISPLAY_SCREEN_TEMPERATURE:
    display_temperature( sensor, full);
    show_last_update();
    break;
    
  case DISPLAY_SCREEN_SETUP:
    display_setup( sensor, full);
    show_last_update();
    break;

  case DISPLAY_SCREEN_STATISTICS1:
    display_statistics1( full);
    break;

  case DISPLAY_SCREEN_STATISTICS2:
    display_statistics2( full);
    break;
  }
}

void Display::display_setup(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];
  char hexstr[ 2 * TPMS_433_ID_LENGTH +1];
  
  if( full) show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {

      if( sensor_update_ts == 0 || sensor_update_ts < sensor[i].last_update) {
        sensor_update_ts = sensor[i].last_update;
      }
      
      x = x_pos[i];
      y = y_pos[i];

      display.setFont(Adafruit5x7);
      display.set2X();
      display.clear(x, x+62, y, y+1);

      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

      display.set1X();
      display.clear(x, x+62, y+2, y+2);
    
      Tpms433::id2hex( sensor[i].sensorId, hexstr );   
      hexstr[ 2 * TPMS_433_ID_LENGTH ] = '\0';
      display.print(hexstr);
    }
  }
}

void Display::display_temperature(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];

  if( full) show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {

      if( sensor_update_ts == 0 || sensor_update_ts < sensor[i].last_update) {
        sensor_update_ts = sensor[i].last_update;
      }

      x = x_pos[i];
      y = y_pos[i];
  
      display.setFont(Adafruit5x7);
      display.set2X();
      display.clear(x, x+62, y, y+1);
      
      dtostrf(sensor[i].temp_c, 2, 0, s);
      display.print(" ");
      display.print(s);      
      display.setFont(System5x7);
      display.print(char(128));  //degrees symbol
      display.setFont(Adafruit5x7);
      display.print("C");

      display.set1X();
      display.clear(x, x+62, y+2, y+2);

      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);
    }
  }
}

void Display::display_pressure(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];
  
  if( full) show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {

      if( sensor_update_ts == 0 || sensor_update_ts < sensor[i].last_update) {
        sensor_update_ts = sensor[i].last_update;
      }

      x = x_pos[i];
      y = y_pos[i];
 
      display.setFont(Adafruit5x7);
      display.set2X();      
      display.clear(x, x+62, y, y+1);

      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

      display.set1X();
      display.clear(x, x+62, y+2, y+2);
 
      dtostrf(sensor[i].temp_c, 2, 0, s);
      display.print(" ");
      display.print(s);
      
      display.setFont(System5x7);
      display.print(char(128));  //degrees symbol
      
      display.setFont(Adafruit5x7);
      display.print("C");
    }
  }
}

void Display::display_statistics1( bool full)
{
  display.setFont(Adafruit5x7);
  display.set1X();

  if( full) {
    display.setCursor(0, 0);
    display.print(F("Statistics"));

    display.setCursor(0, 2);
    display.print(F("version"));
    display.setCursor(0, 3);
    display.print(F("cs intr."));
    display.setCursor(0, 4);
    display.print(F("data intr."));
    display.setCursor(0, 5);
    display.print(F("max carr us"));
    display.setCursor(0, 6);
    display.print(F("carr detect"));
    display.setCursor(0, 7);
    display.print(F("data avail."));
  }

  display.setCursor(72, 2);
  display.print(versionInfo);
  display.setCursor(72, 3);
  display.print(statistics.cs_interrupts);
  display.setCursor(72, 4);
  display.print(statistics.data_interrupts);
  display.setCursor(72, 5);
  display.print(statistics.carrier_len);  
  display.setCursor(72, 6);
  display.print(statistics.carrier_detected);
  display.setCursor(72, 7);
  display.print(statistics.data_available);
}

void Display::display_statistics2( bool full)
{
  display.setFont(Adafruit5x7);
  display.set1X();

  if( full) {
    display.setCursor(0, 0);
    display.print(F("Statistics"));

    display.setCursor(0, 2);
    display.print(F("max timings"));
    display.setCursor(0, 3);
    display.print(F("bit errors"));
    display.setCursor(0, 4);
    display.print(F("preamble ok"));
    display.setCursor(0, 5);    
    display.print(F("cksum ok"));
    display.setCursor(0, 6);
    display.print(F("cksum fails"));
  }

  display.setCursor(72, 2);
  display.print(statistics.max_timings);
  display.setCursor(72, 3);
  display.print(statistics.bit_errors);
  display.setCursor(72, 4);
  display.print(statistics.preamble_found);
  display.setCursor(72, 5);
  display.print(statistics.checksum_ok);
  display.setCursor(72, 6);
  display.print(statistics.checksum_fails);
}

#endif
