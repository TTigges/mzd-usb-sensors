/*
 * protocol.h
 * 
 * Communication protocol defines.
 * 
 */

/* Input buffer length */
#define MAX_BUF_LEN 64

/* Protocol command characters */
const char  NO_COMMAND          = '\0';
const char  QUERY_CONFIG        = 'C';
const char  INFO_COMMAND        = 'I';
const char  LIST_FUNCTIONS      = 'L';
const char  QUERY_FUNCTION      = 'Q';
const char  SET_FUNCTION        = 'S';
const char  MORE_DATA           = '+';
const char  END_OF_TRANSMISSION = '.';
const char  NACK_OR_ERROR       = '/';
