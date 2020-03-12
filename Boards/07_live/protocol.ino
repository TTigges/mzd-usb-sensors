/*
 * protocol.ino
 * 
 * Communication protocol.
 * 
 */

/* Protocol read buffer */
char protocolBuf[MAX_BUF_LEN];
int protocolBufIn = 0;
int protocolBufEnd = 0;

char data[MAX_BUF_LEN];

/* Read a single line from Serial.
 * The first char is stored in commandChar, the rest goes to the protocolBuffer.
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
 * If there is nothing on the local protocolBuffer read a chunk from USB port.
 * Serial.readBytes honors the timeout value set by Serial.setTimeout.
 */
char getChar() {

  char ch = '\0';
  
  if( protocolBufIn == protocolBufEnd) {
    protocolBufIn = protocolBufEnd = 0;
    protocolBufEnd = Serial.readBytes( protocolBuf, MAX_BUF_LEN);
  }

  if( protocolBufIn < protocolBufEnd) {
    ch = protocolBuf[protocolBufIn++];
  }

  return ch;
}

const char *getData() {

  return &data[0];
}


void sendCommand(char command, char *data)
{
  Serial.print(command);
  if( data != NULL || strlen(data) > 0) {
    Serial.print( data);
  }
  Serial.println();  
}

void sendMoreDataStart()
{
  Serial.print(MORE_DATA);
}

void sendMoreDataEnd()
{
  Serial.println();
}

void sendEOT()
{
  if( getError() ) {
    sendError( getErrorMsgAndClear());   
  } else {
    Serial.println(END_OF_TRANSMISSION);
    Serial.flush();
  }
}

void sendError( char *message)
{
  Serial.print(NACK_OR_ERROR);
  if( message != NULL || strlen(message) > 0) {
    Serial.print( message);
  }
  Serial.println();  
}
