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

#include <sys/time.h>
#include <sys/file.h>

static FILE *debug = NULL;
static FILE *logFile = NULL;
static int lockFile = -1;

void printfLog( const char *format, ...)
{
	if( logFile == NULL) {
		logFile = openFileForWrite( LOG_FILE_NAME, LOG_EXT);
	}

	if( logFile != NULL) {
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

void printfDebug( const char *format, ...)
{
	if( debug != NULL) {
		va_list vargs;
		va_start( vargs, format);
		
		vfprintf( debug, format, vargs);
		
		va_end( vargs);
	}	
}

void setDebugStream( FILE *stream)
{
	debug = stream;
}

void strcatToLower( char *target, const char *source)
{
	while( *target != '\0') { target++; }
	while( *source != '\0') {
		*target = isupper( *source) ? tolower( *source) : *source;
		target++;
		source++;
	}
	*target = '\0';
}

long timeMSec()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);

	return (long)tp.tv_sec * 1000 + (long)tp.tv_usec / 1000;
}

FILE *openFileForWrite( const char *name, const char *ext)
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

	fp = fopen( filePath, "w");
	
	if (fp == NULL) {
		fprintf( stderr, "Failed to open file: %s\n", filePath);
	}

	return fp;
}

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
		fprintf( stderr, "acquireLock(): File path to long.\n");
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
				if( sleepMsec >= LOCK_SLEEP_LIMIT_MSEC) {
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

void releaseLock()
{
	if( lockFile >= 0) {
		flock( lockFile, LOCK_UN);
		close( lockFile);
		lockFile = -1;
	}
}
