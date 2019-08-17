/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 */

#include "config.h"
#include "util.h"
#include "protocol.h"
#include "action.h"

#include "tpms_ble.h"
#include "tpms_433.h"
#include "oil_sensor.h"
#include "rgb_analog.h"
#include "ws2801.h"



String versionInfo = "0.1.4";



/* List of supported info requests */
const char *INFO_LIST[] = {
  "ALL",
  "VERSION",
  "SIMULATE",
  NULL /* Do not remove end marker */
};

/* The numeric values of following defines have to be in the 
 * same order as in INFO_LIST, starting with 0.
 */
#define ALL_INFO       0
#define VERSION_INFO   1
#define SIMULATE_INFO  2



/* Last command gotten */
char currentCommand = NO_COMMAND;
#define MAX_FUNNAME_LEN 30
char currentFunction[MAX_FUNNAME_LEN +1]; /* Keep the +1 !! */



/* Parameter storage */
#define MAX_PARAMETER 10
#define MAX_PARAMETER_KEY_LEN 8

typedef struct parameter_t {
  int keyPtr;
  int valuePtr;
} parameter_t;

parameter_t parameter[MAX_PARAMETER];
byte paramIdx;

#define PARAMDATA_MAX_SIZE 256

char paramData[PARAMDATA_MAX_SIZE];
int paramDataPtr;



void setup() {
  
  resetState();

  pinMode( LED_BUILTIN, OUTPUT);

  Serial.begin( 19200);
  Serial.setTimeout( 100);

  while( !Serial) {
    ;
  }


  /* Register all supported actions.
   */
#ifdef TPMS_BLE_SUPPORT
  addAction( new TpmsBLE);
#endif
#ifdef TPMS_433_SUPPORT
  addAction( new Tpms433);
#endif
#ifdef OIL_SUPPORT
  addAction( new OilSensor);
#endif
#ifdef RGB_SUPPORT
  addAction( new RgbAnalog);
#endif
#ifdef WS2801_SUPPORT
  addAction( new WS2801);
#endif
  
  setupActions();

  blinkLed( 500, 1);
}

void loop() {

  char commandChar = readCommand();

  switch( commandChar) {

  case INFO_COMMAND:
  case QUERY_CONFIG:
  case LIST_FUNCTIONS:
  case QUERY_FUNCTION:
  case SET_FUNCTION:
    currentCommand = commandChar;
    strncpy( currentFunction, getData(), MAX_FUNNAME_LEN);
    /* Make sure it is null terminated in any case */
    currentFunction[MAX_FUNNAME_LEN] = '\0';
    break;

  case MORE_DATA:
    handleMoreData();
    break;

  case END_OF_TRANSMISSION:
    handleEOT();
    break;

  case NACK_OR_ERROR:
    handleError();
    break;
    
  case NO_COMMAND:
    /* Nothing to do */
    break;

  default:
    sendError("Unknown command.");
    resetState();
  }
}  

void resetState()
{
  currentCommand = NO_COMMAND;
  currentFunction[0] = '\0';

  paramIdx = 0;
  paramDataPtr = 0;
}

int getIntParam( const char *key, int missingValue) {

  for( int i=0; i<paramIdx; i++) {
    if( strcmp(&paramData[parameter[i].keyPtr], key) == 0) {
      return atoi(&paramData[parameter[i].valuePtr]);
    }
  }

  return missingValue;
}

const char *getStringParam( const char *key) {

  for( int i=0; i<paramIdx; i++) {
    if( strcmp(&paramData[parameter[i].keyPtr], key) == 0) {
      return &paramData[parameter[i].valuePtr];
    }
  }

  return NULL;
}

void setParam( const char *key, const char *value, int valLen) {

  size_t keyLen = strlen(key);
  
  if( paramIdx >= MAX_PARAMETER) {
    flagError( ERROR_TO_MANY_PARAMS);
    return;
  }

  if( paramDataPtr + keyLen + valLen + 2 > PARAMDATA_MAX_SIZE) {
    flagError( ERROR_EXCEED_PARAMDATA);
    return;
  }

  parameter[paramIdx].keyPtr = paramDataPtr;
  strcpy( &paramData[paramDataPtr], key);
  paramDataPtr += keyLen;
  paramData[paramDataPtr++] = '\0';

  parameter[paramIdx].valuePtr = paramDataPtr;
  strncpy( &paramData[paramDataPtr], value, valLen);
  paramDataPtr += valLen;
  paramData[paramDataPtr++] = '\0';  

  paramIdx++;
}

void handleMoreData()
{
  const char *moreData;
  char ch;
  char key[MAX_PARAMETER_KEY_LEN+1];
  byte keyPtr = 0;
  byte valuePtr = 0;
  byte i = 0;
  byte state = 0; 

  moreData = getData();
  
  while( (ch = moreData[i]) != '\0') {
    
    switch( state) {
    case 0: /* save parameter name */
      key[keyPtr++] = ch;
      state++;
      break;

    case 1: /* search for '=' */
      if( ch == '=') {
        key[keyPtr++] = '\0';
        state++;
      }
      else if( ch == ';') {
        keyPtr = 0;
        valuePtr = 0;
        state = 0;        
      }
      else {
        if( keyPtr >= MAX_PARAMETER_KEY_LEN) {
          flagError( ERROR_KEY_TO_LONG);    
        } else {
          key[keyPtr++] = ch;
        }
      }
      break;
      
    case 2: /* save value start index */
      valuePtr = i;
      state++;
      break;

    case 3: /* search for ';' */
      if( ch == ';') {
        setParam( key, &moreData[valuePtr], i-valuePtr);
        keyPtr = 0;
        valuePtr = 0;
        state = 0;
      }
    }
   
    i++;
  }

  if( valuePtr > 0) {
    setParam( key, &moreData[valuePtr], i-valuePtr);
  }
}

/* We got EOT.
 * That means all transfer of data is done and we can start executing the function.
 */
void handleEOT()
{
  switch( currentCommand) {

  case INFO_COMMAND:
    infoCommand();
    break;

  case QUERY_CONFIG:
    queryConfig(currentFunction);
    break;
    
  case LIST_FUNCTIONS:
    listFunctions();
    break;
    
  case QUERY_FUNCTION:
    queryFunction(currentFunction);
    break;
    
  case SET_FUNCTION:
    setFunction(currentFunction);
    break;

  default:
    sendError("Unknown command.");
  }

  resetState();
}

void handleError()
{
  /* Nothing to do for now, just reset everything. */
  resetState();
}

int mapToInfo( char info[])
{
  int infoId = -1;

  for( int i=0; INFO_LIST[i] != NULL; i++) {
    if( strcmp( info, INFO_LIST[i]) == 0) {
      infoId = i;
      break;
    }
  }

  return infoId;
}

void infoCommand()
{
  switch( mapToInfo( currentFunction)) {
    case ALL_INFO:
      sendMoreData( "VERSION=" + versionInfo);
      sendMoreData( actionSimulate ? "SIMULATE=true" : "SIMULATE=false");
      break;
      
    case VERSION_INFO:
      sendMoreData( "VERSION=" + versionInfo);
      break;

    case SIMULATE_INFO:
      sendMoreData( actionSimulate ? "SIMULATE=true" : "SIMULATE=false");
      break;
      
    default:
      flagError( ERROR_UNKNOWN_INFO);
  }
  
  sendEOT();
}
