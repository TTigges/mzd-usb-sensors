/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 * wolfix      09-June-2019  Initial Version
 */

#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define MAX_BUF_LEN 255
char buf[MAX_BUF_LEN]; 
int bufPtr = 0;

const int blueLed = D7;

/* Protocol command characters */
const char INVALID_COMMAND = '\0';
const char LIST_FUNCTIONS = 'L';
const char QUERY_FUNCTION = 'Q';
const char MORE_DATA = '+';
const char END_OF_TRANSMISSION = '.';
const char NACK_OR_ERROR = '/';

/* List of supported functions */
const int FUNCTION_COUNT = 1;
const String FUNCTION_LIST[] = {"TPMS"};

/* TPMS Data */
#define RIGHT_FRONT 0
#define RIGHT_REAR  1
#define LEFT_FRONT  2
#define LEFT_REAR   3
int tpmsData[4] = { 0, 0, 0, 0 };


/* Setup LED and Serial port (USB)
 */
void setup() {
  
  pinMode( blueLed, OUTPUT);
  blinkLed( 500);

  Serial.begin( 115200);
}

void loop() {

  char commandChar = readCommand();

  switch( commandChar) {
  case LIST_FUNCTIONS:
    listFunctions();
    break;
    
  case QUERY_FUNCTION:
    queryFunction();
    break;
    
  case END_OF_TRANSMISSION:
    handleEOT();
    break;

  case NACK_OR_ERROR:
    handleError();
    break;
    
  case INVALID_COMMAND:
    break;

  default:
    sendError("Unknown command.");
  }
}  

/* Read a single line from Serial.
 * The first char is stored in commandChar, the rest goes to the buffer.
 * If there is no data available, return INVALID_COMMAND.
 * Ignore \r characters.
 */
char readCommand() {

  char ch;
  char commandChar = '\0';

  bufPtr = 0;

  if( Serial.available()) {

    commandChar = Serial.read();
    
    while( Serial.available()) {
      ch = Serial.read();
      if( ch == '\r') { continue; }
      if( ch == '\n') { break; }
      buf[bufPtr++] = ch;
      if( bufPtr >= (MAX_BUF_LEN-1)) { break; }
    }

  }

  buf[bufPtr] = '\0';

  return commandChar;
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
  Serial.println(data);  
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

void queryFunction()
{
  switch( mapToFunction( buf)) {
    case 0: /* TPMS */
      getTPMSData();
      sendTPMSData();
      break;

    default:
      sendError("Unknown function.");
  }
}

void handleEOT()
{
  /** @TODO: ignore for now */
}

void handleError()
{
  /** @TODO: Ignore for now */
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

/************************ TPMS *****************************/

void getTPMSData()
{
  /* TESTING ONLY, GENERATE RANDOM TIRE PRESSURE BETWEEN 1.8 AND 2.2 */
  tpmsData[RIGHT_FRONT] = 1800 + random(400);
  tpmsData[RIGHT_REAR] = 1800 + random(400);
  tpmsData[LEFT_FRONT] = 1800 + random(400);
  tpmsData[LEFT_REAR] = 1800 + random(400);
}

void sendTPMSData()
{
  sendMoreData( String("RF=" + String(tpmsData[RIGHT_FRONT])));
  sendMoreData( String("RR=" + String(tpmsData[RIGHT_REAR])));
  sendMoreData( String("LF=" + String(tpmsData[LEFT_FRONT])));
  sendMoreData( String("LR=" + String(tpmsData[LEFT_REAR])));
  sendEOT();
}
