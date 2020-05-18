/*
 *
 */

#ifdef DISPLAY_SUPPORT

/***************/

#define DISPLAY_DEVICE_SH1106       0
#define DISPLAY_DEVICE_SDD1306      1
#define DISPLAY_DEVICE_MAX          1

#define DISPLAY_DEFAULT_DEVICE    DISPLAY_DEVICE_SH1106

/* Only display the screen defined in displayConfig.display_screen */
#define DISPLAY_MODE_SINGLE         0
/* Automatically toggle between two screens */
#define DISPLAY_MODE_AUTOSWITCH     1
/* Automatically switch to next screen, cycling through all screens */
#define DISPLAY_MODE_ALL            2
#define DISPLAY_MODE_MAX            2

#define DISPLAY_DEFAULT_MODE      DISPLAY_MODE_AUTOSWITCH

/* Display update frequency in msec. */
#define DISPLAY_UPDATE_msec       500

/* Display mode switch time in units of DISPLAY_UPDATE_msec
 *  20 x 500 msec = 10 sec
 */
#define DISPLAY_SWITCHTIME         20

/****************/

#define DISPLAY_SCREEN_PRESSURE     0 // displays pressure (large) and temperatur (small)
#define DISPLAY_SCREEN_TEMPERATURE  1 // displays temperature (large) and pressure (small)
#define DISPLAY_SCREEN_SETUP        2 // displays pressure (large) and ID (small)
#define DISPLAY_SCREEN_STATISTICS1  3
#define DISPLAY_SCREEN_STATISTICS2  4
#define DISPLAY_SCREEN_MAX          4

#define DISPLAY_DEFAULT_SCREEN     DISPLAY_SCREEN_SETUP

/****************/

#define DISPLAY_DEFAULT_LAST_UPD_sec  15 // Display last update indicator if time > this


/*
 * Configuration structure stored in EEPROM.
 * It holds the sensor IDs for all 4 tires.
 */
typedef struct display_config_t {

  byte mode;
  byte screen;
  byte device;
  unsigned int last_indicator;

  checksum_t checksum;

} display_config_t;


class Display : public Action {

  private:
    unsigned int configLocation;
    display_config_t displayConfig;
    
    bool display_present = false;
    byte current_screen = DISPLAY_DEFAULT_SCREEN;
    byte last_screen = 255;
    byte change = 0;
    unsigned int last_disp = 0;
    unsigned long last_update = 0;      // Last display update time
    unsigned long sensor_update_ts = 0; // Last time a sensor was updated

  public:
    size_t setup(unsigned int eepromLocation);
    const char *getName();
    void timeout();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

  private:
    void show_title();
    void show_last_update();
    void update_display( tpms433_sensor_t sensor[], bool full);
    void display_setup(tpms433_sensor_t sensor[], bool full);
    void display_temperature(tpms433_sensor_t sensor[], bool full);
    void display_pressure(tpms433_sensor_t sensor[], bool full);
    void display_statistics1( bool full);
    void display_statistics2( bool full);

};

#endif
