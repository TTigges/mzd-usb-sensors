/*
 * Interface Mazda MZD to various data sources and actions.
 * 
 * jayrock     26-June-2019  Port to Nano - stabilized serial protocol - optional output to OLED
 * wolfix      12-June-2019  Fixed protocol syntax
 * wolfix      09-June-2019  Initial Version
 * 
 */

/*
 * jayrock: ReadBear Stuff not needed on Nano
#if defined(ARDUINO) 
SYSTEM_MODE(SEMI_AUTOMATIC); 
#endif
*/

#include <Arduino.h>
#include <U8x8lib.h>

#define SERIAL_READ_DELAY 100

#define USE_OLED 1 //set 0 to disable

#define MAX_BUF_LEN 255
char buf[MAX_BUF_LEN]; 
int bufPtr = 0;

//jayrock - port to Nano
//const int blueLed = D7;
const int blueLed = 13; //onboad LED

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

/*OLED constructor and setup*/
#define SCL 2
#define SDA 0
U8X8_SH1106_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display
void setup_OLED(void)
{
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_px437wyse700b_2x2_f); // more fonts at https://github.com/olikraus/u8g2/wiki/fntlist8x8
  u8x8.drawString(0,0,"FL:");
  u8x8.drawString(0,2,"FR:");
  u8x8.drawString(0,4,"RL:");
  u8x8.drawString(0,6,"RR:");
}

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
#define tPin 0 //@ToDo change to correct pin
#define pPin 5 //@ToDo change to correct pin

/*  */
char currentCommand = NO_COMMAND;
#define MAX_FUNNAME_SIZE 40
char currentFunction[MAX_FUNNAME_SIZE];

/* Disable simulation to get real data */
//jayrock - port to Nano
//boolean simulate = FALSE;
boolean simulate = true;


/* Setup LED and Serial port (USB)
 */
void setup() {

  resetState();

  pinMode( blueLed, OUTPUT);
  blinkLed( 500);

  Serial.begin( 115200);

  setup_OLED();
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
    /*Serial.println("loop");
    Serial.print("commandChar: ");
    Serial.println(commandChar);
    Serial.print("currentFunction: ");
    Serial.println(currentFunction);
    Serial.print("buf: ");
    Serial.println(buf);*/
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

    /*Serial.print("Serial.available() = ");
    Serial.println(Serial.available());*/
    commandChar = Serial.read();
    delay(SERIAL_READ_DELAY);

    /*Serial.print("commandChar :");
    Serial.println(commandChar);
    Serial.println(String(Serial.available()));*/
    
    while( Serial.available() > 0 ) {
      ch = Serial.read();
      /*Serial.print("bufPtr: ");
      Serial.println(bufPtr);
      Serial.print("ch: ");
      Serial.println(ch);*/
      if( ch == '\r') { 
        //Serial.println("rr");
        continue; 
        }
      if( ch == '\n') { 
        //Serial.println("nn");
        break; 
        }
      buf[bufPtr++] = ch;
      //Serial.print("buf :");
      //Serial.println(buf);
      if( bufPtr >= (MAX_BUF_LEN-1)) { 
        //Serial.println("MAX_BUF_LEN");
        break; 
        }
      delay(SERIAL_READ_DELAY);
    }
    /*Serial.println("nix mehr available");*/
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
  // jayrock - port to Nano
  /*sendMoreData( String::format(
    "FL: %.1f %.2f FR: %.1f %.2f RL: %.1f %.2f RR: %.1f %.2f",
    tpmsTemp[FRONT_LEFT], tpmsPress[FRONT_LEFT],
    tpmsTemp[FRONT_RIGHT], tpmsPress[FRONT_RIGHT],
    tpmsTemp[REAR_LEFT], tpmsPress[REAR_LEFT],
    tpmsTemp[REAR_RIGHT], tpmsPress[REAR_RIGHT])*/
  String buf;
  char charBuf[52];


  buf += F("FL: ");
  buf += String(tpmsTemp[FRONT_LEFT], 1);
  buf += F(" ");
  buf += String(tpmsPress[FRONT_LEFT], 2);
 
  buf += F(" FR: ");
  buf += String(tpmsTemp[FRONT_RIGHT], 1);
  buf += F(" ");
  buf += String(tpmsPress[FRONT_RIGHT], 2);

  buf += F(" RL: ");
  buf += String(tpmsTemp[REAR_LEFT], 1);
  buf += F(" ");
  buf += String(tpmsPress[REAR_LEFT], 2);

  buf += F(" RR: ");
  buf += String(tpmsTemp[REAR_RIGHT], 1);
  buf += F(" ");
  buf += String(tpmsPress[REAR_RIGHT], 2);
 
  sendMoreData(buf);

  if(USE_OLED) {
      /*String(tpmsTemp[FRONT_LEFT], 1).toCharArray(charBuf, 52);
      u8x8.drawString(4,0,charBuf);
      String(tpmsTemp[FRONT_RIGHT], 1).toCharArray(charBuf, 52);
      u8x8.drawString(4,1,charBuf);
      String(tpmsTemp[REAR_LEFT], 1).toCharArray(charBuf, 52);
      u8x8.drawString(4,2,charBuf);
      String(tpmsTemp[REAR_RIGHT], 1).toCharArray(charBuf, 52);
      u8x8.drawString(4,3,charBuf);*/
      String(tpmsPress[FRONT_LEFT], 2).toCharArray(charBuf, 52);
      u8x8.drawString(8,0,charBuf);
      String(tpmsPress[FRONT_RIGHT], 2).toCharArray(charBuf, 52);
      u8x8.drawString(8,2,charBuf);
      String(tpmsPress[REAR_LEFT], 2).toCharArray(charBuf, 52);
      u8x8.drawString(8,4,charBuf);
      String(tpmsPress[REAR_RIGHT], 2).toCharArray(charBuf, 52);
      u8x8.drawString(8,6,charBuf);
    }
  
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
  // jayrock - port to Nano
  //sendMoreData( String::format("oiltemp: %.1f oilpres: %.2f", oilTemp, oilPres));
  String buf;

  buf += F("oiltemp: ");
  buf += String(oilTemp, 1);
  buf += F(" oilpres: ");
  buf += String(oilPres, 2);

  sendMoreData(buf);
}
