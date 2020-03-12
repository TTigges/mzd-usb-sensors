/*
 * support.h
 *
 * Generic defines and utilities.
 *
 */

#ifndef _USBGET_SUPPORT_H
#define _USBGET_SUPPORT_H


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
#define LOG_BACKUP_EXT ".log.1"
/* We keep two log files of this size.
 * The current one and a backup of the previous one.
 */
#define LOG_FILE_MAX_SIZE ((long)1024*10)

#define FILE_SEPARATOR "/"

#define LOCK_FILE      "usbget.lck"

/* microsecond to millisecond conversion */
#define USEC_TO_MSEC           1000

/* Wait this long to acquire lock */
#define LOCK_LIMIT_MSEC        5000
#define LOCK_SLEEP_MSEC         100


#define SAFE_STRNCPY( target, source, maxlen)   \
    {                                           \
        strncpy( (target), (source), (maxlen)); \
        target[(maxlen)-1] = '\0';              \
    } while( FALSE)


/* Open log file if it hasn't been opened before and write to it.
 * Note that this function also prints to stderr.
 */
void printfLog( const char *format, ...);

/* If debug output is enabled print to debug stream.
 * Debug output is enabled by setDebugStream() with a non-null argument.
 */
void printfDebug( const char *format, ...);

/* Set debug output stream.
 * stream == null disables debug output.
 */
void setDebugStream( FILE *stream);

/* Open a file in the OUTPUT_PATH directory.
 * If the resulting path is to long or there was an error during
 * file open this function returns NULL.
 */
FILE *openFile( const char *name, const char *ext, const char *mode);

/* Timestamp in milliseconds.
 * Note: The returned value may have no relation to wall clock time
 * if we are running on a micro controller.
 */
long timeMSec( void);

/* Return a timestamp with format "YYYY-DD-MM HH:MM:SS".
 * Space available in ts should be at least 20 char.
 */
void timestamp( char *ts, size_t len);

/* Acquire an exclusive lock on the lock file.
 * The lock file is created in OUTPUT_PATH.
 */
returnCode acquireLock( void);

/* Release previously acquired lock.
 */
void releaseLock( void);

#endif
