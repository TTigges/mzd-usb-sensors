/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 */

#include "config.h"
#include "util.h"
#include "protocol.h"
#include "action.h"

#include "tpms_ble.h"
#include "cc1101.h"
#include "tpms_433.h"
#include "oil_sensor.h"
#include "rgb_analog.h"
#include "ws2801.h"

#include "display.h"


String versionInfo = "0.1.5";


#define ENABLE_MEMDEBUG


#ifdef ENABLE_MEMDEBUG

/* ATMega 328 p
 *
 *    0 -   31 Register File (32)
 *   32 -   95 Standard IO Memory (64)
 *   96 -  255 Extended IO Memory (160)
 *  256 - 2303 SRAM (2048)
 */

/* Start of free memory in SRAM */
#define freeStart memdebug[0]
/* Stackpointer */
#define stackLow memdebug[1]
/* Gap size or initial free bytes */
#define gapSize memdebug[2]
/* Free bytes determined by free space check */
#define gapFree memdebug[3]

/* Initialize every byte within the gap between
 * freeStart and stackLow with bit pattern 01010101
 */
#define MEMDEBUG_INIT()                       \
do {                                          \
    uint16_t i;                               \
    cli();                                    \
    gapFree = 0;                              \
    freeStart = (uint16_t)malloc( 1);         \
    stackLow = SPH << 8 | SPL;                \
    gapSize = stackLow - freeStart - 2;       \
    for( i=0; i<gapSize; i++) {               \
        *(byte*)(freeStart+i) = 0x55;         \
    }                                         \
    sei();                                    \
} while( false)

/* Check how many bytes are still free.
 * (Unchanged pattern 01010101)
 */
#define MEMDEBUG_CHECK()                      \
do {                                          \
    uint16_t i;                               \
    gapFree = 0;                              \
    for( i=0; i<gapSize; i++) {               \
        if( *(byte*)(freeStart+i) != 0x55) {  \
            break;                            \
        } else {                              \
            gapFree++;                        \
        }                                     \
    }                                         \
} while( false)

uint16_t memdebug[4];

#endif



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
#define MAX_FUNNAME_LEN 20
char currentFunction[MAX_FUNNAME_LEN +1]; /* Keep the +1 !! */



/* Parameter storage */
#define MAX_PARAMETER 5
#define MAX_PARAMETER_KEY_LEN 8

typedef struct parameter_t {
  int keyPtr;
  int valuePtr;
} parameter_t;

parameter_t parameter[MAX_PARAMETER];
byte paramIdx;

#define PARAMDATA_MAX_SIZE 128

char paramData[PARAMDATA_MAX_SIZE];
int paramDataPtr;



void setup() {
  
  resetState();

  pinMode( LED_BUILTIN, OUTPUT);

  Serial.begin( 19200);
  Serial.setTimeout( 20);

  while( !Serial) {
    ;
  }

#ifdef DISPLAY_SUPPORT
  display_init();
#endif

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

#ifdef ENABLE_MEMDEBUG
  MEMDEBUG_INIT();
#endif    
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
    /* Timeout 100msec */
    runTimeout();
    break;

  default:
    sendError("Unknown command.");
    resetState();
  }
}  

/*** PUBLIC ***/

/*
 * Called from action modules to fetch a numeric parameter passed in
 * by usbget -d <device> -s <module> -p "<key>=<param>"
 */
int getIntParam( const char *key, int missingValue) {

  for( int i=0; i<paramIdx; i++) {
    if( strcmp(&paramData[parameter[i].keyPtr], key) == 0) {
      return atoi(&paramData[parameter[i].valuePtr]);
    }
  }

  return missingValue;
}

/*
 * Called from action modules to fetch a string parameter passed in
 * by usbget -d <device> -s <module> -p "<key>=<param>"
 */
const char *getStringParam( const char *key) {

  for( int i=0; i<paramIdx; i++) {
    if( strcmp(&paramData[parameter[i].keyPtr], key) == 0) {
      return &paramData[parameter[i].valuePtr];
    }
  }

  return NULL;
}

/*** PRIVATE ***/

static void setParam( const char *key, const char *value, int valLen) {

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

static void resetState()
{
  currentCommand = NO_COMMAND;
  currentFunction[0] = '\0';

  paramIdx = 0;
  paramDataPtr = 0;
}

static void handleMoreData()
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
static void handleEOT()
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

static void handleError()
{
  /* Nothing to do for now, just reset everything. */
  resetState();
}

static int mapToInfo( char info[])
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

static void infoCommand()
{
  sendMoreDataStart();
  Serial.print( F("version     = "));
  Serial.print(versionInfo);
  sendMoreDataEnd();

  sendMoreDataStart();
  Serial.print( F("simulate    = "));
  if( actionSimulate) {
    Serial.print( F("true"));
  } else {
    Serial.print( F("false"));  
  }
  sendMoreDataEnd();

  dump_statistics();
      
#ifdef ENABLE_MEMDEBUG     
  MEMDEBUG_CHECK();

  sendMoreDataStart();
  Serial.print(F("memory      = "));
  Serial.print( gapFree);
  Serial.print(F("/"));
  Serial.print( gapSize);
  sendMoreDataEnd();
  
/*
      Serial.print(F("+"));
      size_t i;
      for( byte i=0; i<80; i++) {
        Serial.print( *(byte*)(freeStart+i), HEX); 
        Serial.print(F(" "));   
      }
      Serial.println();
*/
      
#endif
  
  sendEOT();
}
