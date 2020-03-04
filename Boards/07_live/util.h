/*
 * util.h
 * 
 * Some handy defines.
 * 
 */

#define ERROR_NONE              0
#define ERROR_UNKNOWN_ACTION    1
#define ERROR_UNKNOWN_INFO      2
#define ERROR_INVALID_PARAM     3
#define ERROR_TO_MANY_PARAMS    4
#define ERROR_EXCEED_PARAMDATA  5
#define ERROR_KEY_TO_LONG       6

#define ERROR_COUNT             7

/* This is a global error variable that can be set by flagError() in the action specific code.
 * If this variable is set the corresponding error message is sent to the peer when 
 * sendEOT() is called.
 */
uint8_t globalError = ERROR_NONE;

const char *errorMsg[] = {

  "",                              // ERROR_NONE
  "Unknown action request.",       // ERROR_UNKNOWN_ACTION
  "Unknown info request.",         // ERROR_UNKNOWN_INFO
  "Invalid parameter.",            // ERROR_INVALID_PARAM
  "To many parameters.",           // ERROR_TO_MANY_PARAMS
  "Parameter data exceeds limit.", // ERROR_EXCEED_PARAMDATA
  "Parameter key to long."         // ERROR_KEY_TO_LONG

};

/* Checksum type to checksum EEPROM configuration data.
 */
typedef uint16_t checksum_t;


/************** STATISTICS *********************/

typedef struct statistics_t {
  unsigned long cs_interrupts;
  unsigned long data_interrupts;
  unsigned long carrier_detected;
  unsigned long data_available;
  unsigned int carrier_len;
  unsigned int max_timings;
  unsigned int preamble_found;
  unsigned int checksum_ok;
  unsigned int checksum_fails;
} statistics_t;

static volatile statistics_t statistics;

/* Note: dump happens unlatched. 
 * Wrong data may be printed (sometimes).
 */
void dump_statistics()
{
  Serial.print(F("+cs intr.    = "));
  Serial.println(statistics.cs_interrupts);
  Serial.print(F("+data intr.  = "));
  Serial.println(statistics.data_interrupts);
  Serial.print(F("+max carr us = "));
  Serial.println(statistics.carrier_len);  
  Serial.print(F("+carr detect = "));
  Serial.println(statistics.carrier_detected);
  Serial.print(F("+data avail. = "));
  Serial.println(statistics.data_available);
  Serial.print(F("+max timings = "));
  Serial.println(statistics.max_timings);
  Serial.print(F("+preamble ok = "));
  Serial.println(statistics.preamble_found);
  Serial.print(F("+cksum ok    = "));
  Serial.println(statistics.checksum_ok);
  Serial.print(F("+cksum fails = "));
  Serial.println(statistics.checksum_fails);
}

void clear_statistics()
{
  memset( (void*)&statistics, 0, sizeof(statistics));
}
