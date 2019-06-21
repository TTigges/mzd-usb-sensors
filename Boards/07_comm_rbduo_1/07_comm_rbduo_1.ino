/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 * wolfix      12-June-2019  Fixed protocol syntax
 * wolfix      09-June-2019  Initial Version
 * 
 */

#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif

#define MAX_BUF_LEN 255
char buf[MAX_BUF_LEN]; 
int bufPtr = 0;

const int blueLed = D7;

/* Protocol command characters */
const char NO_COMMAND = '\0';
const char LIST_FUNCTIONS = 'L';
const char QUERY_FUNCTION = 'Q';
const char SET_FUNCTION = 'S';
const char MORE_DATA = '+';
const char END_OF_TRANSMISSION = '.';
const char NACK_OR_ERROR = '/';

/* BLE scan parameters */
#define BLE_SCAN_TYPE        0x00   // Passive scanning
#define BLE_SCAN_INTERVAL    0x0060 // 60 ms
#define BLE_SCAN_WINDOW      0x0030 // 30 ms

/* Only accept these addresses */
char SMAC1[16] = "80EACA100326"; // Front left
char SMAC2[16] = "81EACA2002C0"; // Front right
char SMAC3[16] = "82EACA300633"; // Rear left
char SMAC4[16] = "83EACA400619"; // Rear right

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
float oilPres = 0.0;

/* OIL Data */
#define ABSZERO 273.15
#define MAXANALOGREAD 4095.0
#define tPin A0
#define pPin A5

/*  */
char currentCommand = NO_COMMAND;
#define MAX_FUNNAME_SIZE 40
char currentFunction[MAX_FUNNAME_SIZE];

/* Disable simulation to get real data */
boolean simulate = FALSE;

/* Setup LED and Serial port (USB)
 */
void setup() {

  resetState();

  pinMode( blueLed, OUTPUT);
  blinkLed( 500);

  Serial.begin( 115200);
  
  delay(5000);
  Serial.println("BLE scan demo.");
  
  ble.init();
  ble.onScanReportCallback(reportCallback);

  // Set scan parameters.
  ble.setScanParams(BLE_SCAN_TYPE, BLE_SCAN_INTERVAL, BLE_SCAN_WINDOW);
  
  ble.startScanning();
  Serial.println("BLE scan start.");
 
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

/* Blink blue LED once for time_ms.
 */
void blinkLed( int time_ms) {

  digitalWrite(blueLed, HIGH);
  delay(time_ms);
  digitalWrite(blueLed, LOW);
  delay(time_ms);
}

/********************** BLE TPMS PRESSURE AND TEMPERATURE *************************/

uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    while (*hex) {
        // get current character then increment
        char byte = *hex++; 
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

void reportCallback(advertisementReport_t *report) {
  
  char addr[16];
  sprintf(addr, "%02X%02X%02X%02X%02X%02X", (unsigned char)report->advData[11], (unsigned char)report->advData[12], (unsigned char)report->advData[13], (unsigned char)report->advData[14], (unsigned char)report->advData[15], (unsigned char)report->advData[16]);
  Serial.print("Addr.: ");Serial.println(addr);
  Serial.println(" ");
  char pres[10];
  char temp[10];
  if (strcmp (SMAC1, addr) == 0) {
    Serial.print("Sensor ");Serial.print("FRONT_LEFT");Serial.print(": ");Serial.println(addr);
    
    sprintf(pres, "%02X%02X%02X%02X", (unsigned char)report->advData[20], (unsigned char)report->advData[19], (unsigned char)report->advData[18], (unsigned char)report->advData[17]);
    uint32_t y = hex2int(pres);
    float pressure = y/100000.00;
    //Serial.print("Pres: ");Serial.print(pressure);Serial.println(" bar");
    tpmsPress[0] = pressure;

    sprintf(temp, "%02X%02X%02X%02X", (unsigned char)report->advData[24], (unsigned char)report->advData[23], (unsigned char)report->advData[22], (unsigned char)report->advData[21]);
    uint32_t x = hex2int(temp);
    float tmpr = x/100.00;
    //Serial.print("Temp: ");Serial.print(tmpr);Serial.println("°C");
    tpmsTemp[0] = tmpr;

    Serial.println(" ");
  }
  else if (strcmp (SMAC2, addr) == 0) {
    Serial.print("Sensor ");Serial.print("FRONT_RIGHT");Serial.print(": ");Serial.println(addr);
    
    sprintf(pres, "%02X%02X%02X%02X", (unsigned char)report->advData[20], (unsigned char)report->advData[19], (unsigned char)report->advData[18], (unsigned char)report->advData[17]);
    uint32_t y = hex2int(pres);
    float pressure = y/100000.00;
    //Serial.print("Pres: ");Serial.print(pressure);Serial.println(" bar");
    tpmsPress[1] = pressure;

    sprintf(temp, "%02X%02X%02X%02X", (unsigned char)report->advData[24], (unsigned char)report->advData[23], (unsigned char)report->advData[22], (unsigned char)report->advData[21]);
    uint32_t x = hex2int(temp);
    float tmpr = x/100.00;
    //Serial.print("Temp: ");Serial.print(tmpr);Serial.println("°C");
    tpmsTemp[1] = tmpr;

    Serial.println(" ");
  }
  else if (strcmp (SMAC3, addr) == 0) {
    Serial.print("Sensor ");Serial.print("REAR_LEFT");Serial.print(": ");Serial.println(addr);
    
    sprintf(pres, "%02X%02X%02X%02X", (unsigned char)report->advData[20], (unsigned char)report->advData[19], (unsigned char)report->advData[18], (unsigned char)report->advData[17]);
    uint32_t y = hex2int(pres);
    float pressure = y/100000.00;
    //Serial.print("Pres: ");Serial.print(pressure);Serial.println(" bar");
    tpmsPress[2] = pressure;

    sprintf(temp, "%02X%02X%02X%02X", (unsigned char)report->advData[24], (unsigned char)report->advData[23], (unsigned char)report->advData[22], (unsigned char)report->advData[21]);
    uint32_t x = hex2int(temp);
    float tmpr = x/100.00;
    //Serial.print("Temp: ");Serial.print(tmpr);Serial.println("°C");
    tpmsTemp[2] = tmpr;

    Serial.println(" ");
  }
  else if (strcmp (SMAC4, addr) == 0) {
    Serial.print("Sensor ");Serial.print("REAR_RIGHT");Serial.print(": ");Serial.println(addr);
    
    sprintf(pres, "%02X%02X%02X%02X", (unsigned char)report->advData[20], (unsigned char)report->advData[19], (unsigned char)report->advData[18], (unsigned char)report->advData[17]);
    uint32_t y = hex2int(pres);
    float pressure = y/100000.00;
    //Serial.print("Pres: ");Serial.print(pressure);Serial.println(" bar");
    tpmsPress[3] = pressure;

    sprintf(temp, "%02X%02X%02X%02X", (unsigned char)report->advData[24], (unsigned char)report->advData[23], (unsigned char)report->advData[22], (unsigned char)report->advData[21]);
    uint32_t x = hex2int(temp);
    float tmpr = x/100.00;
    //Serial.print("Temp: ");Serial.print(tmpr);Serial.println("°C");
    tpmsTemp[3] = tmpr;

    Serial.println(" ");
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
    oilPres = (0 + random(1000)) / 100.0; // 0.00 ... 10.00 bar
  }
  else 
  {
    /* Real implementation goes here */
    float tPinValue = analogRead(tPin);
    oilTemp = calcTemp(tPinValue);

    float pPinValue = analogRead(pPin);
    oilPres = calcPres(pPinValue);
  }
}

float calcTemp(float tPinValue) {
  float T0 = 40 + ABSZERO;  // K
  float T1 = 150 + ABSZERO; // K
  float R0 = 5830;
  float R1 = 316;
  float RV = 1000;
  float VA_VB = tPinValue/MAXANALOGREAD;
  float B = (T0 * T1)/ (T1-T0) * log(R0/R1);
  float RN = RV*VA_VB / (1-VA_VB);
  return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}

float calcPres(float pPinValue) {
  if (pPinValue < 410) {
    pPinValue = 410;
  }
  else if (pPinValue > 3689) {
    pPinValue = 3689;
  }
  return pPinValue * 0.00305250305250305 - 1.25;
}

void sendOil()
{
  /* oiltemp: xx oilpres: yy */
  sendMoreData( String::format("oiltemp: %.1f oilpres: %.2f", oilTemp, oilPres));
}
