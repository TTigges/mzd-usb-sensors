/*
 * tmps_433.h
 * 
 * Reading 433Mhz TPMS sensors.
 * 
 */

#ifdef TPMS_433_SUPPORT

#define TPMS_433_NUM_SENSORS 4

#define FRONT_LEFT  0
#define FRONT_RIGHT 1
#define REAR_LEFT   2
#define REAR_RIGHT  3

/*
 * Configuration structure stored in EEPROM.
 * It holds the sensor IDs for all 4 tires.
 */
typedef struct tpms433_config_t {
  
  /* TODO: Adjust length and data type */
  char smac[TPMS_433_NUM_SENSORS][16];
  
  checksum_t checksum;
  
} tpms433_config_t;

/*
 * This modules configuration data.
 */
tpms433_config_t tpms433Config;

/*
 * 
 */
class Tpms433 : public Action {

  private:
    unsigned int configLocation;
    float tpmsPress[TPMS_433_NUM_SENSORS] = { 0.0, 0.0, 0.0, 0.0 };
    float tpmsTemp[TPMS_433_NUM_SENSORS] = { 0.0, 0.0, 0.0, 0.0 };

  public:
    size_t setup(unsigned int settingsLocation);
    const char *getName();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();
};

#endif
