/*
 * tpms_ble.ino
 * 
 * Read TMPS Bluetooth Low Energy sensors.
 * 
 */

#ifdef TPMS_BLE_SUPPORT

/* BLE scan parameters */
#define BLE_SCAN_TYPE        0x00   // Passive scanning
#define BLE_SCAN_INTERVAL    0x0060 // 60 ms
#define BLE_SCAN_WINDOW      0x0030 // 30 ms

size_t TpmsBLE::setup(unsigned int eepromLocation) {

  size_t sizeOfConfig = sizeof(tpmsBLEConfig);
  
  /* Remember where the configuration is stored in EEPROM */
  configLocation = eepromLocation;
  
  EEPROM.get( eepromLocation, tpmsBLEConfig);

  if( tpmsBLEConfig.checksum == computeChecksum( &tpmsBLEConfig, sizeOfConfig)) {

    blinkLed( 200, 2);
  }
  else {
    /* Set default values in case the checksum is wrong and there is no valid date in the EEPROM */
    strcpy( tpmsBLEConfig.smac[FRONT_LEFT],  "80EACA100326");
    strcpy( tpmsBLEConfig.smac[FRONT_RIGHT], "81EACA2002C0");
    strcpy( tpmsBLEConfig.smac[REAR_LEFT],   "82EACA300633");
    strcpy( tpmsBLEConfig.smac[REAR_RIGHT],  "83EACA400619");

    tpmsBLEConfig.checksum = computeChecksum( &tpmsBLEConfig, sizeOfConfig);
    EEPROM.put( eepromLocation, tpmsBLEConfig);

    blinkLed( 200, 5);
  }

  ble.init();
  ble.onScanReportCallback(tpmsReportCallback);
  ble.setScanParams(BLE_SCAN_TYPE, BLE_SCAN_INTERVAL, BLE_SCAN_WINDOW);
  ble.startScanning();

  return sizeOfConfig;
}

const char *TpmsBLE::getName()
{

  return "TPMS";
}

void TpmsBLE::getData()
{
  if( actionSimulate)
  {
    /* TESTING ONLY, GENERATE RANDOM TIRE PRESSURE BETWEEN 1.8 AND 2.2 
     * Pressure: Two digits right of the dot.
     * Temperature: One digit right of the dot.
     */
    tpmsPress[FRONT_RIGHT] = (180 + random(40)) / 100.0;
    tpmsPress[FRONT_LEFT] = (180 + random(40)) / 100.0;
    tpmsPress[REAR_RIGHT] = (180 + random(40)) / 100.0;
    tpmsPress[REAR_LEFT] = (180 + random(40)) / 100.0;

    tpmsTemp[FRONT_RIGHT] = (-100 + random(900)) / 10.0;
    tpmsTemp[FRONT_LEFT] = (-100 + random(900)) / 10.0;
    tpmsTemp[REAR_RIGHT] = (-100 + random(900)) / 10.0;
    tpmsTemp[REAR_LEFT] = (-100 + random(900)) / 10.0;
  }
  else 
  {
    /* Real implementation uses: tpmsReportCallback() */
  }    
}

void TpmsBLE::sendData()
{
  String data;

  SINGLE_THREADED_BLOCK() {
    /* FL: temp press FR: temp press RL: temp press RR: temp press */
    data = "FL: "+String(tpmsTemp[FRONT_LEFT],1)+" "+String(tpmsPress[FRONT_LEFT],2)
         +" FR: "+String(tpmsTemp[FRONT_RIGHT],1)+" "+String(tpmsPress[FRONT_RIGHT],2)
         +" RL: "+String(tpmsTemp[REAR_LEFT],1)+" "+String(tpmsPress[REAR_LEFT],2)
         +" RR: "+String(tpmsTemp[REAR_RIGHT],1)+" "+String(tpmsPress[REAR_RIGHT],2);
  }
  
  sendMoreData( data);
}

void TpmsBLE::sendConfig()
{
  sendMoreData( "FL="+String(tpmsBLEConfig.smac[FRONT_LEFT]));
  sendMoreData( "FR="+String(tpmsBLEConfig.smac[FRONT_RIGHT]));
  sendMoreData( "RL="+String(tpmsBLEConfig.smac[REAR_LEFT]));
  sendMoreData( "RR="+String(tpmsBLEConfig.smac[REAR_RIGHT]));
}

void TpmsBLE::setConfig()
{
  boolean writeEEPROM = false;
  
  writeEEPROM |= saveSMAC( "FL", FRONT_LEFT);
  writeEEPROM |= saveSMAC( "FR", FRONT_RIGHT);
  writeEEPROM |= saveSMAC( "RL", REAR_LEFT);
  writeEEPROM |= saveSMAC( "RR", REAR_RIGHT);
  
  if( writeEEPROM) {
    tpmsBLEConfig.checksum = computeChecksum( &tpmsBLEConfig, sizeof(tpmsBLEConfig));
    EEPROM.put( configLocation, tpmsBLEConfig);
  }
}

boolean TpmsBLE::saveSMAC( const char *which, byte loc)
{
  const char *param;

  param = getStringParam( which);

  if( param ) {
    if( validSMAC( param)) {
      strcpy( tpmsBLEConfig.smac[loc], param);
      return true;
    } else {
      flagError( ERROR_INVALID_PARAM);
    }
  }

  return false;
}

boolean TpmsBLE::validSMAC( const char *smac) 
{
  return    (smac != NULL)
         && (strlen( smac) == 12);
}

void tpmsReportCallback(advertisementReport_t *report)
{  
  char addr[16];
  char val[10];
  int pos = -1;

  sprintf( addr, "%02X%02X%02X%02X%02X%02X",
                 (unsigned char)report->advData[11],
                 (unsigned char)report->advData[12],
                 (unsigned char)report->advData[13],
                 (unsigned char)report->advData[14],
                 (unsigned char)report->advData[15],
                 (unsigned char)report->advData[16]);

  if( strcmp (tpmsBLEConfig.smac[FRONT_LEFT], addr) == 0) {
    pos = FRONT_LEFT;
  }
  else if (strcmp (tpmsBLEConfig.smac[FRONT_RIGHT], addr) == 0) {
    pos = FRONT_RIGHT;
  }
  else if (strcmp (tpmsBLEConfig.smac[REAR_LEFT], addr) == 0) {
    pos = REAR_LEFT;
  }
  else if (strcmp (tpmsBLEConfig.smac[REAR_RIGHT], addr) == 0) {
    pos = REAR_RIGHT;
  }

  if( !actionSimulate) {

    if ((pos >= 0) && (pos <= 3)) {
      sprintf( val, "%02X%02X%02X%02X",
               (unsigned char)report->advData[20],
               (unsigned char)report->advData[19],
               (unsigned char)report->advData[18],
               (unsigned char)report->advData[17]);

      SINGLE_THREADED_BLOCK() {
        tpmsPress[pos] = (float)hex2int(val)/100000.00;
      }
    
      sprintf( val, "%02X%02X%02X%02X",
               (unsigned char)report->advData[24],
               (unsigned char)report->advData[23],
               (unsigned char)report->advData[22],
               (unsigned char)report->advData[21]);

      SINGLE_THREADED_BLOCK() {
        tpmsTemp[pos] = (float)hex2int(val)/100.00;
      }
    }
  }
}

#endif
