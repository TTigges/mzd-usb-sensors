/*
 * support.c
 *
 * Generic defines and utilities.
 *
 */

#include <support.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <sys/file.h>


static FILE *debug = NULL;
static FILE *logFile = NULL;
static int lockFile = -1;


static void strcatToLower( char *target, const char *source);


/* ******************* export functions ********************* */

/* Open log file if it hasn't been opened before and write to it.
 * Note that this function also prints to stderr.
 */
void printfLog( const char *format, ...)
{
    long fileSize;
    FILE *backupLogFile;
    char buf;
#define TS_MAX_LEN 20
    char ts[TS_MAX_LEN];

    if( logFile == NULL) {
        logFile = openFileForWrite( LOG_FILE_NAME, LOG_EXT, "a+");
    }

    if( logFile != NULL) {
        fseek(logFile, 0L, SEEK_END);
        fileSize = ftell( logFile);

        if( fileSize >= LOG_FILE_MAX_SIZE) {
            backupLogFile =
                openFileForWrite( LOG_FILE_NAME, LOG_BACKUP_EXT, "w");

            if( backupLogFile != NULL) {
                /* Copy logfile to backup */
                fseek(logFile, 0L, SEEK_SET);
                while( fread( &buf,(size_t)1,(size_t)1,logFile) == 1) {
                    fwrite( &buf,(size_t)1,(size_t)1,backupLogFile);
                }

                fclose( backupLogFile);
            }

            /* Reopen original log file (truncate) */
            fclose( logFile);
            logFile = openFileForWrite( LOG_FILE_NAME, LOG_EXT, "w");
        }
    }

    if( logFile != NULL) {
        timestamp( ts, TS_MAX_LEN);
        fprintf( logFile, "%s: ", ts);

        va_list vargs;
        va_start( vargs, format);
        vfprintf( logFile, format, vargs);
        va_end( vargs);
    }

    /* Print also to stderr */
    va_list vargs;
    va_start( vargs, format);
    vfprintf( stderr, format, vargs);
    va_end( vargs);
}

/* If debug output is enabled print to debug stream.
 * Debug output is enabled by setDebugStream() with a non-null argument.
 */
void printfDebug( const char *format, ...)
{
    if( debug != NULL) {
        va_list vargs;
        va_start( vargs, format);

        vfprintf( debug, format, vargs);

        va_end( vargs);
    }
}

/* Set debug output stream.
 * stream == null disables debug output.
 */
void setDebugStream( FILE *stream)
{
    debug = stream;
}

/* Timestamp in milliseconds.
 * Note: The returned value may have no relation to wall clock time
 * if we are running on a micro controller.
 */
long timeMSec()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (long)tp.tv_sec * 1000 + (long)tp.tv_usec / 1000;
}

/* Return a timestamp with format "YYYY-DD-MM HH:MM:SS".
 * Space available in ts should be at least 20 char.
 */
void timestamp( char *ts, size_t len)
{
    time_t now;
    struct tm *timeinfo;

    if( ts) {
        time( &now);
        timeinfo = localtime( &now);

        strftime( ts, len, "%F %T", timeinfo);
    }
}

/* Open a file in the OUTPUT_PATH directory.
 * If the resulting path is to long or there was an error during
 * file open this function returns NULL.
 */
FILE *openFileForWrite( const char *name,
                        const char *ext,
                        const char *mode)
{
    FILE *fp;
    char filePath[MAX_FILE_PATH_LEN];

    if( strlen(OUTPUT_PATH)+strlen(FILE_SEPARATOR)+strlen(name)
        +strlen(ext) < MAX_FILE_PATH_LEN) {

        strcpy( filePath, OUTPUT_PATH);
        strcat( filePath, FILE_SEPARATOR);
        strcatToLower( filePath, name);
        strcat( filePath, ext);
    }
    else {
        fprintf( stderr, "openFileForWrite(): File path to long.\n");
        return NULL;
    }

    fp = fopen( filePath, mode);

    if (fp == NULL) {
        fprintf( stderr, "Failed to open file: %s\n", filePath);
    }

    return fp;
}

/* Acquire an exclusive lock on the lock file.
 * The lock file is created in OUTPUT_PATH.
 */
returnCode acquireLock()
{
    char filePath[MAX_FILE_PATH_LEN];
    int rcFlock;
    int sleepMsec = 0;

    if( strlen(OUTPUT_PATH)+strlen(FILE_SEPARATOR)+strlen(LOCK_FILE)
        < MAX_FILE_PATH_LEN) {

        strcpy( filePath, OUTPUT_PATH);
        strcat( filePath, FILE_SEPARATOR);
        strcat( filePath, LOCK_FILE);
    }
    else {
        printfLog( "acquireLock(): File path to long.\n");
        return RC_ERROR;
    }

    lockFile = open( filePath, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if( lockFile < 0) {
        printfLog( "Error opening lock file [%s]: %d\n",
            filePath, lockFile);
        /* @TODO: rethink, continue without lock */
        return RC_ERROR;
    }
    else {
        do {
            /* Try to acquire lock with non blocking option */
            rcFlock = flock( lockFile, LOCK_EX | LOCK_NB);

            /* We try a couple of times to get the lock.
             * If that fails for LOCK_SLEEP_LIMIT
             * we raise an error and quit.
             */
            if( rcFlock && errno == EWOULDBLOCK) {
                usleep( LOCK_SLEEP_MSEC * USEC_TO_MSEC);
                sleepMsec += LOCK_SLEEP_MSEC;
                if( sleepMsec >= LOCK_LIMIT_MSEC) {
                    break;
                }
            }
            else { /* Fail for any other error. */
                break;
            }

        } while( rcFlock);

        if( rcFlock) {
            printfLog( "Error locking file: %d\n", errno);
            return RC_ERROR;
        }
    }

    return RC_OK;
}

/* Release previously acquired lock.
 */
void releaseLock()
{
    if( lockFile >= 0) {
        flock( lockFile, LOCK_UN);
        close( lockFile);
        lockFile = -1;
    }
}

/* ******************* static functions ********************* */

/* Append source to target with toLower() conversion.
 * Target gets null terminated in any case.
 */
static void strcatToLower( char *target, const char *source)
{
    while( *target != '\0') { target++; }
    while( *source != '\0') {
        *target = isupper( *source) ? tolower( *source) : *source;
        target++;
        source++;
    }
    *target = '\0';
}
