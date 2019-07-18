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

typedef uint16_t checksum_t;
