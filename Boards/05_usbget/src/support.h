/*
 * support.h
 * 
 * Generic defines and utilities.
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef FALSE
#define FALSE (0)
#define TRUE  (!FALSE)
#endif

#ifndef boolean
typedef int boolean;
#endif


/* Return codes */
typedef int returnCode;
#define RC_OK            ((returnCode)0)
#define RC_ERROR         ((returnCode)-1)


#define MAX_FILE_PATH_LEN 80

#define OUTPUT_PATH    "/tmp/mnt/data_persist/dev/bin"
#define OUTPUT_EXT     ".out"
#define LOG_FILE_NAME  "usbget"
#define LOG_EXT        ".log"
#define FILE_SEPARATOR "/"
#define LOCK_FILE      "usbget.lck"

/* microsecond to millisecond conversion */
#define USEC_TO_MSEC           1000
#define LOCK_SLEEP_MSEC         100
#define LOCK_SLEEP_LIMIT_MSEC  2000

void printfLog( const char *format, ...);

void printfDebug( const char *format, ...);

void setDebugStream( FILE *stream);

FILE *openFileForWrite( const char *name, const char *ext);

void strcatToLower( char *target, const char *source);

long timeMSec();

returnCode acquireLock();

void releaseLock();
