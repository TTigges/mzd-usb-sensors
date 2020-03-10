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


/* Automatically switch display to next mode after 10 seconds */
#define DISPLAY_AUTOSWITCH       true
/* Display mode switch time in units of DISPLAY_UPDATE_msec
 *  20 x 500 msec = 10 sec
 */
#define DISPLAY_SWITCHTIME       20


/* Display update frequency in msec. */
#define DISPLAY_UPDATE_msec      500

#define DISPLAYMODE_SETUP        0 //displays pressure (large) and ID (small)
#define DISPLAYMODE_TEMPERATURE  1 //displays temperature (large) and pressure (small)
#define DISPLAYMODE_PRESSURE     2 //displays pressure (large) and temperatur (small)
#define DISPLAYMODE_STATISTICS1  3
#define DISPLAYMODE_STATISTICS2  4

/* Initial display mode */
byte display_mode = DISPLAYMODE_SETUP;
//byte display_mode = DISPLAYMODE_TEMPERATURE;
//byte display_mode = DISPLAYMODE_PRESSURE;


byte last_display_mode = display_mode;
byte change = 0;
unsigned long last_update = 0;

SSD1306AsciiWire display;


/* ********** forward declarations ********** */
void show_title();
void UpdateDisplay(tpms433_sensor_t sensor[], bool full);
void display_setup( tpms433_sensor_t sensor[], bool full);
void display_temperature( tpms433_sensor_t sensor[], bool full);
void display_pressure( tpms433_sensor_t sensor[], bool full);
void display_statistics1();
void display_statistics2();


void display_init()
{
  Wire.begin();
  Wire.setClock(400000L);
  
  display.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
  display.setFont(Adafruit5x7);
  display.clear();
  
  show_title();
}

/* This is called every 10 msec */
void display_handler()
{
  unsigned long now = millis();
  bool full = false;

  if( now >= last_update + DISPLAY_UPDATE_msec) {

    if( DISPLAY_AUTOSWITCH ) {
      change++;
      if( change > DISPLAY_SWITCHTIME) {
        change = 0;
        display_mode++;
        if( display_mode > 4) {
          display_mode=0;
        }
      }
    }
    
    if( display_mode != last_display_mode)
    {
      display.clear();
      last_display_mode = display_mode;
      full = true;
    }
    
    UpdateDisplay( tpmsReceiver.getSensors(), full);
    
    /* Update last_update last because it is used in UpdateDisplay() */
    last_update = now;
  }
}

void show_title()
{
  display.set1X();             // Normal 1:1 pixel scale
  display.setCursor(0, 0);
  display.println(" Abarth TPMS Monitor");
  display.println("   (TWJ Solutions)");
}

void UpdateDisplay( tpms433_sensor_t sensor[], bool full)
{
  switch( display_mode)
  {
  case DISPLAYMODE_SETUP:
    display_setup( sensor, full);
    break;
    
  case DISPLAYMODE_TEMPERATURE:
    display_temperature( sensor, full);
    break;
    
  case DISPLAYMODE_PRESSURE:
    display_pressure( sensor, full);
    break;

  case DISPLAYMODE_STATISTICS1:
    display_statistics1();
    break;

  case DISPLAYMODE_STATISTICS2:
    display_statistics2();
    break;
  }
}

void display_setup(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];
  char hexstr[ 2 * TPMS_433_ID_LENGTH +1];
  
  show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {
      
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
        
    //display.setFont(System5x7);          
    //display.print(DisplayTimeoutBar(millis() - sensor[i].lastupdated));
    }
  }
}

void display_temperature(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];

  show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {
      
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

       //in bar...
      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

//      display.setFont(System5x7);          
//      display.print(DisplayTimeoutBar(millis() - sensor[i].lastupdated));
    }
  }
}

void display_pressure(tpms433_sensor_t sensor[], bool full)
{
  byte i;
  int x;
  int y;
  char s[6];
  
  show_title();
  
  for (i = 0; i < 4; i++)
  {
    if( full || sensor[i].last_update >= last_update) {

      x = x_pos[i];
      y = y_pos[i];
 
      display.setFont(Adafruit5x7);
      display.set2X();      
      display.clear(x, x+62, y, y+1);

      //in bar...
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

void display_statistics1()
{
  display.setFont(Adafruit5x7);
  display.set1X();
  
  display.setCursor(0, 0);
  display.print(F("Statistics"));

  display.setCursor(0, 2);
  display.print(F("cs intr.    "));
  display.println(statistics.cs_interrupts);
  display.setCursor(0, 3);
  display.print(F("data intr.  "));
  display.println(statistics.data_interrupts);
  display.setCursor(0, 4);
  display.print(F("max carr us "));
  display.println(statistics.carrier_len);  
  display.setCursor(0, 5);
  display.print(F("carr detect "));
  display.println(statistics.carrier_detected);
  display.setCursor(0, 6);
  display.print(F("data avail. "));
  display.println(statistics.data_available);
  display.setCursor(0, 7);
  display.print(F("max timings "));
  display.println(statistics.max_timings);
}

void display_statistics2()
{
  display.setFont(Adafruit5x7);
  display.set1X();

  display.setCursor(0, 0);
  display.print(F("Statistics"));

  display.setCursor(0, 2);
  display.print(F("preamble ok "));
  display.println(statistics.preamble_found);
  display.setCursor(0, 3);
  display.print(F("cksum ok    "));
  display.println(statistics.checksum_ok);
  display.setCursor(0, 4);
  display.print(F("cksum fails "));
  display.println(statistics.checksum_fails);
}

#endif
