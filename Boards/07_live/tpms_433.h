/*
 * tmps_433.h
 * 
 * Reading 433Mhz TPMS sensors.
 * 
 */

#ifdef TPMS_433_SUPPORT

#include "tpms_decode.h"


/* Reserved space for our sensors */
#define TPMS_433_NUM_SENSORS    4
/* We keep some extra space for sensors that are not ours */
#define TPMS_433_EXTRA_SENSORS  8

#define TPMS_433_ID_LENGTH      4 // in bytes

#define TPMS_433_SCORE_MAX       250
#define TPMS_433_SCORE_ADD        10
#define TPMS_433_SCORE_TIMEOUT_s  20

/*
 * Configuration structure stored in EEPROM.
 * It holds the sensor IDs for all 4 tires.
 */
typedef struct tpms433_config_t {

  byte sensorId[TPMS_433_NUM_SENSORS][TPMS_433_ID_LENGTH];
  
  checksum_t checksum;
  
} tpms433_config_t;

typedef struct tpms433_sensor_t {

  byte sensorId[TPMS_433_ID_LENGTH];
  // unsigned long lastupdated;
  float press_bar;
  float temp_c;
  byte score;
  
} tpms433_sensor_t;

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
    unsigned long next_score_adj;
    tpms433_sensor_t sensor[TPMS_433_NUM_SENSORS + TPMS_433_EXTRA_SENSORS];
    
  public:
    size_t setup(unsigned int settingsLocation);
    const char *getName();
    void timeout();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

    void static id2hex( byte b[], char hex[]);
    void static hex2id( char hex[], byte b[]);
    
  private:
    int find_sensor( byteArray_t *data);
    bool match_id( byte id, byteArray_t *data);
    bool is_empty( byte id);
    bool empty_config();
    void copy_sensor( tpms433_sensor_t *t, tpms433_sensor_t *s);
    void set_sensor_IDs_from_config();
    void sort_sensors( byte top);
};

#endif
