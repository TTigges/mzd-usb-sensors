/*
 * util.h
 * 
 * Some handy utilities.
 * 
 */

/* Compute checksum of a structure.
 * NOTE: The checksum is part of the structure and MUST
 *       be the last variable in the structure.
 */
static checksum_t computeChecksum( void *str, size_t sz)
{
  checksum_t checksum = 0;
  uint8_t *p = (uint8_t*)str;
  
  for( uint16_t i = 0; i < (sz - sizeof(checksum_t)); i++)
  {
    checksum = rotate(checksum);
    checksum ^= ((i+1) ^ ((uint8_t)*(p+i)));
  }

  return checksum;
}

/* Left rotate unsigned int value.
 */
static checksum_t rotate( checksum_t v)
{
  int bits = sizeof( checksum_t) * 8 -1;
  
  return ((v >> bits) & 1) | (v << 1);
}

/* Blink builtin LED once for time_ms.
 */
void blinkLed( int time_ms, uint8_t n)
{

  for( uint8_t i=0; i<n; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(time_ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(time_ms);
  }
}

/* Convert hex string to unsigned 32 bit value */
uint32_t hex2int(char *hex)
{
  uint32_t val = 0;
  char ch;

  while ( (ch = *(hex++)) ) {

    // transform hex character to the 4bit equivalent number, using the ascii table indexes
    if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (ch >= 'a' && ch <='f') ch = ch - 'a' + 10;
    else if (ch >= 'A' && ch <='F') ch = ch - 'A' + 10;    
    else break;
    
    // shift 4 to make space for new digit, and add the 4 bits of the new digit 
    val = (val << 4) | (ch & 0xF);
  }

  return val;
}

void clearError() {
  
  globalError = ERROR_NONE;
}

void flagError( uint8_t errNo) {

  /* We are interrest in the first error only */
  if( globalError == ERROR_NONE && errNo < ERROR_COUNT) {
    globalError = errNo;
  }
}

uint8_t getError() {

  return globalError;
}

const char *getErrorMsgAndClear() {

  uint8_t errNo = globalError;
  clearError();

  return errorMsg[errNo];
}
