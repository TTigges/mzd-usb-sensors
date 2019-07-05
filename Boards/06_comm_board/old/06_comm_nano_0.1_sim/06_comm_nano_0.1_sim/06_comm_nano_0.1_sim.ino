/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 * wolfix      12-June-2019  Fixed protocol syntax
 * wolfix      09-June-2019  Initial Version
 */


#define MAX_BUF_LEN 255
char buf[MAX_BUF_LEN]; 
int bufPtr = 0;

/* Protocol command characters */
const char NO_COMMAND = '\0';
const char LIST_FUNCTIONS = 'L';
const char QUERY_FUNCTION = 'Q';
const char SET_FUNCTION = 'S';
const char MORE_DATA = '+';
const char END_OF_TRANSMISSION = '.';
const char NACK_OR_ERROR = '/';

/* List of supported functions */
const int FUNCTION_COUNT = 2;
const String FUNCTION_LIST[] = {"TPMS", "OIL"};
/* The numeric values of following defines have to be in the 
 * same order as in FUNCTION_LIST, starting with 0.
 */
#define TPMS 0
#define OIL  1

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

/* Disable simulation to get real data */
boolean simulate = true;

/* Setup LED and Serial port (USB)
 */
void setup() {

  resetState();

  Serial.begin( 115200);
}

void loop() {

  char commandChar = readCommand();

  switch( commandChar) {

  case LIST_FUNCTIONS:
  case QUERY_FUNCTION:
  case SET_FUNCTION:
    currentCommand = commandChar;
    strncpy( currentFunction, buf, MAX_FUNNAME_SIZE);
    currentFunction[MAX_FUNNAME_SIZE-1] = '\0';
    break;

  case MORE_DATA:
    /* @TODO: For future 'set' command. Ignore for now. */
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
}

/* Read a single line from Serial.
 * The first char is stored in commandChar, the rest goes to the buffer.
 * If there is no data available, return NO_COMMAND.
 * Ignore \r characters.
 */
char readCommand() {

  char ch;
  char commandChar = NO_COMMAND;

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

    default:
      sendError("Unknown function.");
  }
}

/* Set a variable or execute an action
 */
void setFunction()
{
  /* @TODO: Future extension */  
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
  sendMoreData( String::format(
    "FL: %.1f %.2f FR: %.1f %.2f RL: %.1f %.2f RR: %.1f %.2f",
    tpmsTemp[FRONT_LEFT], tpmsPress[FRONT_LEFT],
    tpmsTemp[FRONT_RIGHT], tpmsPress[FRONT_RIGHT],
    tpmsTemp[REAR_LEFT], tpmsPress[REAR_LEFT],
    tpmsTemp[REAR_RIGHT], tpmsPress[REAR_RIGHT])
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
  sendMoreData( String::format("oiltemp: %.1f oilpress: %.2f", oilTemp, oilPress));
}
