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

typedef struct tpms433_config_t {
  /* TODO: not sure what we need here */
  char smac[TPMS_433_NUM_SENSORS][16];
  checksum_t checksum;
} tpms433_config_t;

tpms433_config_t tpms433Config;

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
