/*
 * tpms_ble.h
 * 
 * Read TMPS Bluetooth Low Energy sensors.
 * 
 */

#ifdef TPMS_BLE_SUPPORT

#define TPMS_BLE_NUM_SENSORS 4

#define FRONT_LEFT  0
#define FRONT_RIGHT 1
#define REAR_LEFT   2
#define REAR_RIGHT  3

typedef struct tpmsBLE_config_t {
  /* Adresses of BLE sensors */
  char smac[TPMS_BLE_NUM_SENSORS][16];

  /* checksum MUST BE LAST */
  checksum_t checksum;
} tpmsBLE_config_t;

/* The config structure and the sensor data variables should actually 
 * be part of the class but they cannot as we need to access it from the BLE callback.
 */
tpmsBLE_config_t tpmsBLEConfig;

/* Tire pressure */
volatile float tpmsPress[TPMS_BLE_NUM_SENSORS] = { 0.0, 0.0, 0.0, 0.0 };
/* Tire temperature */
volatile float tpmsTemp[TPMS_BLE_NUM_SENSORS] = { 0.0, 0.0, 0.0, 0.0 };


class TpmsBLE : public Action {

  private:
    unsigned int configLocation;

  public:
    size_t setup(unsigned int settingsLocation);
    const char *getName();
    void getData();
    void sendData();
    void sendConfig();
    void setConfig();

  private:
    boolean saveSMAC( const char *which, byte loc);
    boolean validSMAC( const char *smac);
};

#endif
