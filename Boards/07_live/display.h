/*
 *
 */

#ifdef DISPLAY_SUPPORT

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"


//#define DISPLAYMODE_SETUP 1 //displays pressure (large) and ID (small)
#define DISPLAYMODE_TEMPERATURE 1 //displays temperature (large) and pressure (small)
//#define DISPLAYMODE_PRESSURE 1 //displays pressure (large) and temperatur (small)


static const byte x_pos[] = { 2, 60, 2, 60};
static const byte y_pos[] = { 2,  2, 5,  5};


SSD1306AsciiWire display;

void display_init()
{
  Wire.begin();
  Wire.setClock(400000L);
  
  display.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
  display.setFont(Adafruit5x7);
}

void show_title()
{
  display.clear();
  display.set1X();             // Normal 1:1 pixel scale
  display.setCursor(0, 0);
  display.println(" Abarth TPMS Monitor");
  display.println("   (TWJ Solutions)");
}

/*
char DisplayTimeoutBar(unsigned long TimeSinceLastUpdate)
{
      int HowCloseToTimeout;
      HowCloseToTimeout = (int)(TimeSinceLastUpdate/(TPMS_TIMEOUT/5));

      switch(HowCloseToTimeout)
      {
        case 0: 
           return('5');
           break;
        case 1: 
           return('4');
           break;
        case 2: 
           return('3');
           break;
        case 3: 
           return('2');
           break;
        case 4: 
           return('1');
           break;
        default: 
           return('0');
           break;
                      
      }
}
*/

#if DISPLAYMODE_SETUP

void UpdateDisplay(tpms433_sensor_t sensor[])
{
  byte i;
  int x;
  int y;
  char s[6];
  
  show_title();
  
  for (i = 0; i < 4; i++)
  {
    x = x_pos[i];
    y = y_pos[i];
    
//    if (TPMS[i].TPMS_ID != 0)
//    {
      display.setCursor(x, y);
      display.setFont(Adafruit5x7);
      display.set2X();

      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

      display.setCursor(x, y+2);
      display.setFont(Adafruit5x7);
      display.set1X();
      
      display.print(sensor[i].TPMS_ID,HEX);
        
      //display.setFont(System5x7);          
      //display.print(DisplayTimeoutBar(millis() - sensor[i].lastupdated));
//    }
  }
}
  
#endif


#if DISPLAYMODE_TEMPERATURE

void UpdateDisplay(tpms433_sensor_t sensor[])
{
  byte i;
  int x;
  int y;
  char s[6];

  show_title();
  
  for (i = 0; i < 4; i++)
  {
    x = x_pos[i];
    y = y_pos[i];
  
//    if (TPMS[i].TPMS_ID != 0)
//    {
      display.setCursor(x, y);
      display.setFont(Adafruit5x7);
      display.set2X();
      
      dtostrf(sensor[i].temp_c, 2, 0, s);
      display.print(" ");
      display.print(s);      
      display.setFont(System5x7);
      display.print(char(128));  //degrees symbol
      display.setFont(Adafruit5x7);
      display.print("C");
      display.print("  ");

      display.setCursor(x, y+2);
      display.setFont(Adafruit5x7);
      display.set1X();
      //in bar...
      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

//      display.setFont(System5x7);          
//      display.print(DisplayTimeoutBar(millis() - sensor[i].lastupdated));
//    }
  }
}
  
#endif
  
#if DISPLAYMODE_PESSUUE

void UpdateDisplay(tpms433_sensor_t sensor[])
{
  byte i;
  int x;
  int y;
  char s[6];
  
  show_title();
  
  for (i = 0; i < 4; i++)
  {
    x = x_pos[i];
    y = y_pos[i];
 
//    if (TPMS[i].TPMS_ID != 0)
//    {
      display.setCursor(x, y);
      display.setFont(Adafruit5x7);
      display.set2X();      
      //in bar...
      dtostrf(sensor[i].press_bar, 3, 2, s);
      display.print(s);

      display.setCursor(x, y+2);
      display.setFont(Adafruit5x7);
      display.set1X();
      dtostrf(sensor[i].temp_c, 2, 0, s);
      display.print(" ");
      display.print(s);
      
      display.setFont(System5x7);
      display.print(char(128));  //degrees symbol
      
      display.setFont(Adafruit5x7);
      display.print("C");
      display.print("  ");

//      display.setFont(System5x7);          
//      display.print(DisplayTimeoutBar(millis() - sensor[i].lastupdated));
//    }
  }
}
  
#endif

#endif
