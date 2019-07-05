/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 * wolfix      20-June-2019  Support Arduino (pro and nano)
 * wolfix      12-June-2019  Fixed protocol syntax
 * wolfix      09-June-2019  Initial Version
 */


/* Select one of... */

#define REDBEAR_DUO
/* #define ARDUINO_GENERIC */


#include <Adafruit_WS2801.h>


#ifdef REDBEAR_DUO

#if defined(ARDUINO) 
  SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define LED_BUILTIN D7

#ifndef true
# define true TRUE
# define false FALSE
#endif

#endif

/* End of port specific code */


#define MAX_BUF_LEN 64
char data[MAX_BUF_LEN]; 

char buf[MAX_BUF_LEN];
int bufIn = 0;
int bufEnd = 0;

const int blueLed = LED_BUILTIN;

/* Protocol command characters */
const char NO_COMMAND = '\0';
const char LIST_FUNCTIONS = 'L';
const char QUERY_FUNCTION = 'Q';
const char SET_FUNCTION = 'S';
const char MORE_DATA = '+';
const char END_OF_TRANSMISSION = '.';
const char NACK_OR_ERROR = '/';

/* List of supported functions */
const int FUNCTION_COUNT = 3;
const String FUNCTION_LIST[] = {"TPMS", "OIL", "WS2801"};
/* The numeric values of following defines have to be in the 
 * same order as in FUNCTION_LIST, starting with 0.
 */
#define TPMS   0
#define OIL    1
#define WS2801 2

/* TPMS Data */
#define FRONT_RIGHT 0
#define FRONT_LEFT  1
#define REAR_RIGHT  2
#define REAR_LEFT   3

/* Tire pressure */
float tpmsPress[4] = { 0.0, 0.0, 0.0, 0.0 };
/* Tire temperature */
float tpmsTemp[4] = { 0.0, 0.0, 0.0, 0.0 };
/* Oil temperature and pressure */
float oilTemp =  0.0;
float oilPress = 0.0;

/*  */
char currentCommand = NO_COMMAND;
#define MAX_FUNNAME_SIZE 40
char currentFunction[MAX_FUNNAME_SIZE];

#define MAX_PARAMETER 10
typedef struct parameter_t {
  char name;
  int intVal;
} parameter_t;

parameter_t parameter[MAX_PARAMETER];
byte paramIdx;

/* Disable simulation to get real data */
boolean simulate = true;


Adafruit_WS2801 LEDs;
int WS2801_ledCnt = 4;

/* Setup LED and Serial port (USB)
 */
void setup() {

  resetState();

  pinMode( blueLed, OUTPUT);
  blinkLed( 500);

  Serial.begin( 19200);
  Serial.setTimeout( 100);

  while( !Serial) {
    ;
  }

  LEDs = Adafruit_WS2801(WS2801_ledCnt, 2, 3);

  LEDs.begin();
  setWS2801( 0, 0, 0);
}

void loop() {

  char commandChar = readCommand();

  switch( commandChar) {

  case LIST_FUNCTIONS:
  case QUERY_FUNCTION:
  case SET_FUNCTION:
    currentCommand = commandChar;
    strncpy( currentFunction, data, MAX_FUNNAME_SIZE);
    currentFunction[MAX_FUNNAME_SIZE-1] = '\0';
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

  for( int i=0; i<MAX_PARAMETER; i++) {
    parameter[i].name = '\0';
  }
  paramIdx = 0;
}

int getIntParam( char cmd) {

  for( int i=0; i<paramIdx; i++) {
    if( parameter[i].name == cmd) {
      return parameter[i].intVal;
    }
  }

  return 0;
}

void setIntParam( char cmd, int value) {

  if( paramIdx < MAX_PARAMETER) {
    parameter[paramIdx].name = cmd;
    parameter[paramIdx++].intVal = value;
  }
}

/* Read a single line from Serial.
 * The first char is stored in commandChar, the rest goes to the buffer.
 * If there is no data available, return NO_COMMAND.
 * Ignore any control characters. ASCII(1) - ASCII(31)
 */
char readCommand() {

  char ch;
  int ptr = 0;
  char commandChar = NO_COMMAND;

  while( (ch = getChar()) != '\0') {

    if( ch == '\n') { break; }
    if( ch < ' ') { continue; }

    if( commandChar == NO_COMMAND) {
      commandChar = ch;
    }
    else {
      data[ptr++] = ch;
      if( ptr >= (MAX_BUF_LEN-1)) { break; }
    }
  }

  data[ptr] = '\0';

  return commandChar;
}

/* Get one character from USB.
 *  
 * If there is nothing on the local buffer read a chunk from USB port.
 * Serial.readBytes honors the timeout value set by Serial.setTimeout.
 */
char getChar() {

  char ch = '\0';
  
  if( bufIn == bufEnd) {
    bufIn = bufEnd = 0;
    bufEnd = Serial.readBytes( buf, MAX_BUF_LEN);
  }

  if( bufIn < bufEnd) {
    ch = buf[bufIn++];
  }

  return ch;
}

void sendCommand(char command, String data)
{
  Serial.print(command);
  if( data != NULL || data.length() > 0) {
    Serial.print( data);
  }
  Serial.println();  
}

void sendMoreData( String data)
{
  Serial.print(MORE_DATA);
  Serial.println( data); 
}

void sendEOT()
{
  Serial.println(END_OF_TRANSMISSION);
  Serial.flush();
}

void sendError( String message)
{
  Serial.print(NACK_OR_ERROR);
  if( message != NULL || message.length() > 0) {
    Serial.print( message);
  }
  Serial.println();  
}

/* Send list of supported functions.
 */
void listFunctions()
{
  for( int i=0; i<FUNCTION_COUNT; i++) {
    sendMoreData(FUNCTION_LIST[i]);
  }

  sendEOT();
}

/* Query for data
 */
void queryFunction()
{
  switch( mapToFunction( currentFunction)) {
    case TPMS:
      getTpms();
      sendTpms();
      sendEOT();
      break;

    case OIL:
      getOil();
      sendOil();
      sendEOT();
      break;

    case WS2801:
      sendEOT();
      break;

    default:
      sendError("Unknown function.");
  }
}

/* Set a variable or execute an action
 */
void setFunction()
{
  switch( mapToFunction( currentFunction)) {
    case TPMS:
      sendEOT();
      break;

    case OIL:
      sendEOT();
      break;

    case WS2801:
      setWS2801( getIntParam('R'), getIntParam('G'),getIntParam('B'));

      /* DEBUG
      sendMoreData( String(paramIdx));
      for( int i=0; i<paramIdx; i++) {
        sendMoreData( String(parameter[i].name));
      }
      */

      sendEOT();
      break;

    default:
      sendError("Unknown function.");
  }
}

void handleMoreData()
{
  char ch;
  char cmd = '\0';
  byte valueIdx = 0;
  byte i = 0;
 
  byte state = 0; 
  
  while( (ch = data[i]) != '\0') {
    
    switch( state) {
    case 0: /* save parameter name */
      cmd = ch;
      state++;
      break;

    case 1: /* search for '=' */
      if( ch == '=') {
        state++;
      }
      else if( ch == ';') {
        cmd = '\0';
        valueIdx = 0;
        state = 0;        
      }
      break;
      
    case 2: /* save value start index */
      if( ch == '=' || ch == ';') {
        cmd = '\0';
        valueIdx = 0;
        state = 0;        
      }
      valueIdx = i;
      state++;
      break;

    case 3: /* search for ';' */
      if( ch == ';') {
        setIntParam( cmd, toInt( &data[valueIdx]));
        cmd = '\0';
        valueIdx = 0;
        state = 0;
      }
    }
   
    i++;
  }

  if( cmd != '\0' && valueIdx > 0) {
    setIntParam( cmd, toInt( &data[valueIdx]));
  }
}

int toInt( char *str) {

  char ch;
  byte neg = 0;
  int val = 0;

  while( (ch = *(str++))) {
    if( ch == ' ') {
      continue;
    }
    else if( ch == '-') {
      neg = 1;
    }
    else if( isDigit(ch)) {
      val = val * 10 + (ch - '0');
    }
    else {
      break;
    }
  }
  
  return (neg) ? -val : val;
}

/* We got EOT.
 * That means all transfer of data is done and we can start executing the function.
 */
void handleEOT()
{
  switch( currentCommand) {

  case LIST_FUNCTIONS:
    listFunctions();
    break;
    
  case QUERY_FUNCTION:
    queryFunction();
    break;
    
  case SET_FUNCTION:
    setFunction();
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

/* Map the function name to function index number.
 * If there is no function with this name return index -1.
 */
int mapToFunction( char funName[])
{
  int functionId = -1;

  for( int i=0; i<FUNCTION_COUNT; i++) {
    if( strcmp( funName, FUNCTION_LIST[i].c_str()) == 0) {
      functionId = i;
      break;
    }
  }

  return functionId;
}

/* Blink blue LED once for time_ms.
 */
void blinkLed( int time_ms) {

  digitalWrite(blueLed, HIGH);
  delay(time_ms);
  digitalWrite(blueLed, LOW);
  delay(time_ms);
}

/********************** TPMS PRESSURE AND TEMPERATURE *************************/

void getTpms()
{
  if( simulate)
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
    /* @TODO: Real implementation goes here */
  }    
}

void sendTpms()
{
  /* FL: temp press FR: temp press RL: temp press RR: temp press */
  sendMoreData(
    "FL: "+String(tpmsTemp[FRONT_LEFT],1)+" "+String(tpmsPress[FRONT_LEFT],2)
    +" FR: "+String(tpmsTemp[FRONT_RIGHT],1)+" "+String(tpmsPress[FRONT_RIGHT],2)
    +" RL: "+String(tpmsTemp[REAR_LEFT],1)+" "+String(tpmsPress[REAR_LEFT],2)
    +" RR: "+String(tpmsTemp[REAR_RIGHT],1)+" "+String(tpmsPress[REAR_RIGHT],2)
  );
}

void getOil()
{
  if( simulate)
  {
    /* TESTING ONLY, GENERATE RANDOM OIL PRESSURE AND TEMPERATURE
     * Pressure: Two digits right of the dot.
     * Temperature: One digit right of the dot.
     */
    oilTemp = (-100 + random(1200)) / 10.0; // -10.0 ... 120.0 C
    oilPress = (0 + random(1000)) / 100.0; // 0.00 ... 10.00 bar
  }
  else 
  {
    /* @TODO: Real implementation goes here */
  }
}

void sendOil()
{
  /* oiltemp: xx oilpress: yy */
  sendMoreData( "oiltemp: "+String(oilTemp,1)+" oilpress: "+String(oilPress,2));
}

/*************************** WS2801 LED Strip *********************************/

void setWS2801( byte R, byte G, byte B)
{  
  for( int i=0; i<WS2801_ledCnt; i++) {
    LEDs.setPixelColor( i, R, G, B);
  }
  LEDs.show();
}
