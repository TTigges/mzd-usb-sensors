/*
 * Transparently fetch and push data from/to MZD to/from USB device.
 * 
 * When run the program connects to a micro controller connected to
 * the USB bus, fetches data and stores the result in a file in 
 * /tmp/mnt/data_persist/dev/bin
 * 
 * The filename is the action name converted to lower case with 
 * extension ".out".
 * 
 * 
 * Command line options
 * ====================
 * 
 * usage: usb_rbduo_get [options] [command ... ]
 * 
 *	 Options:
 *     -d device_name             Specify device to use.
 *     -v                         Verbose. Enable debug output.
 *     -?                         Print usage
 * 
 *   Commands:
 *     -l                         List supported actions
 *     -q action [-p param ... ]  Query action
 *     -s action [-p param ... ]  Set action
 * 
 *   In case no command is specified all supported actions are queried.
 * 
 * 
 * Line Protocol:
 * ==============
 * 
 * The protocol supports the following functions:
 * 
 *   + Query action list
 *       The micro controller supports a list of actions to be 
 *       performed on behalf of the MZD. The list may vary depending
 *       on the software release running on the micro controller.
 *       This function queries the list of supported actions.
 * 
 *   + Query action
 *       Execute an action on the micro controller and query the
 *       result state.
 * 
 *   + Set variable
 *       Modify the value of a variable on the micro controler or
 *       perform an action without fetching any results.
 * 
 * The protocol is character based (No binary data).
 * Every command starts with a single command character.
 * Every command is terminated by a newline character.
 * 
 * Supported command characters:
 * =============================
 *	 L     Query action list
 *   Q     Execute and query action
 *   S     Set variable
 *   +     Additional data
 *   .     End of transfer
 *   /     NACK or error response
 *   <nl>  Newline
 *
 * Examples
 * ========
 * 
 *      MZD         Transfer direction        Micro controller
 * ----------------------------------------------------------------
 *
 * Query action list
 * -----------------
 * L                         =>
 * .                         =>
 *                          <=             +action1
 *                          <=             +action2
 *                          <=             .
 *
 * Execute and query action
 * ------------------------
 * Qaction1                  =>
 * .                         =>
 *                          <=             +result line 1
 *                          <=             +result line 2
 *                          <=             .
 *
 * Execute and query action with optional parameter
 * ------------------------------------------------
 * Qaction1                  =>
 * +variable1=value1         =>
 * .                         =>
 *                          <=             +result line 1
 *                          <=             +result line 2
 *                          <=             .
 *
 * Variablen setzen
 * ----------------
 * Saction1                  =>
 * +variable1=value1         =>
 * .                         =>
 *                          <=             .
 * 
 * In case of an error
 * -------------------
 * Qblabla                   =>
 * .                         =>
 *                          <=             /Unknown function.
 * 
 * 
 * Credits
 * =======
 *   usbOpen() code based on mazda_tpms.c from Mazdaracerdude.
 * 
 * 
 * TODOs
 * =====
 * 
 * 
 * File History
 * ============
 *   wolfix                   Obey lock file
 *   wolfix                   Support set command
 *   wolfix                   Support command line options
 *   wolfix      13-Jun-2019  Minor fixes and code cleanup
 *   wolfix      12-Jun-2019  Initial version
 * 
 */
 
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/file.h>

#include <libusb.h>


#ifndef FALSE
#define FALSE (0)
#define TRUE  (!FALSE)
#endif

#ifndef boolean
typedef int boolean;
#endif


/* USB device info */

typedef struct device_info_t {
	char *name;
	uint16_t vendor_id;
	uint16_t product_id;
	unsigned char in_endp;
	unsigned char out_endp;
	int if_count;
} device_info_t;

static const device_info_t device_info[] = {
	{"redbear_duo",   0x2B04, 0xC058, 0x81, 0x01, 2},
	{"arduino_pro",   0x0403, 0x6001, 0x81, 0x02, 1},
	{"arduino_nano",  0x1a86, 0x7523, 0x81, 0x02, 1},

	/* END MARKER. Do not remove! */
	{NULL, 0x0000, 0x0000, 0x00, 0x00, 0}
};

static const device_info_t *device = NULL;
static struct libusb_device_handle *usbDevH;


#define OUTPUT_PATH    "/tmp/mnt/data_persist/dev/bin"
#define OUTPUT_EXT     ".out"
#define LOG_EXT        ".log"
#define FILE_SEPARATOR "/"
#define LOCK_FILE      "usb_rbduo_get.lck"

/* Protocol command characters */
static const char NO_COMMAND          = '\0';
static const char LIST_ACTIONS        = 'L';
static const char QUERY_ACTION        = 'Q';
static const char SET_ACTION          = 'S';
static const char MORE_DATA           = '+';
static const char END_OF_TRANSMISSION = '.';
static const char NACK_OR_ERROR       = '/';


#define isNoCommand( cmd) (cmd == NO_COMMAND)
#define isEOT( cmd) (cmd == END_OF_TRANSMISSION)
#define isMoreData( cmd) (cmd == MORE_DATA)
#define isNACK( cmd) (cmd == NACK_OR_ERROR)

/* microsecond to millisecond conversion */
#define USEC_TO_MSEC           1000
#define LOCK_SLEEP_MSEC         100
#define LOCK_SLEEP_LIMIT_MSEC  2000

/* USB receive timeout */
#define RECEIVE_TIMEOUT_MSEC   2000
#define DRAIN_TIMEOUT_MSEC       10

/* Transfer buffers */
#define MAX_BUFFER_SIZE         256

static char sendBuffer[MAX_BUFFER_SIZE];
static int sendBufferPtr;

static char receiveBuffer[MAX_BUFFER_SIZE];
static int receiveBufferPtr;
static int receiveBufferEnd;

/* received line without command char */
static char bufferedLine[MAX_BUFFER_SIZE];

/* @TODO current limit of 10 actions */
#define MAX_ACTIONS           10
#define MAX_ACTION_NAME_LEN   20
static char *actions[MAX_ACTIONS];
static int actionCount;

/* @TODO current limit of 10 parameters per action */
#define MAX_PARAMETERS 10
static char *parameters[MAX_PARAMETERS];
static int parameterCount;


static FILE *debug = NULL;
static FILE *logFile = NULL;
static int lockFile = -1;

/* Run options selected via command line parameters */
enum RunOption {
    QUIT = 0,
    ERROR,
	LIST,
	QUERY,
	SET
};

#define MAX_OPTION_ACTION_SIZE 20
static char optionAction[MAX_OPTION_ACTION_SIZE];

/******************* static forward declarations *******************/

static void parseOptions( int argc, char **argv);
static enum RunOption parseArguments( int argc, char **argv);
static void usage();
static void resetTransferBuffers();

static void listActions();
static void printActions();
static void queryAction( const char *action);
static void setAction( const char *action);

static FILE *openFileForWrite( const char *action, const char *ext);
static int acquireLock();
static void releaseLock();

static void printfLog( const char *format, ...);
static void printfDebug( const char *format, ...);
static void strcatToLower( char *target, const char *source);

static char *receiveLine( char *commandChar);

static void sendMoreData( const char *data);
static void sendEOT();
static void sendError( const char *message);
static void sendCommand(char command, const char *data);

static libusb_device_handle* usbOpen( uint16_t vendor_id, uint16_t product_id);
static void usbClose( libusb_device_handle* usbDevH);
static boolean usbSendBuffer(char *buf);
static char usbGetChar();
static void drainInput();

/*******************************************************************/

int main( int argc, char **argv)
{
	boolean nothingToDo = TRUE;
	enum RunOption runOption;

	parseOptions( argc, argv);
		
	if( device == NULL) {
		printfLog( "No device specified.\n");
		printfDebug( "No device specified.\n");
		exit(-1);
	}
	
	/* Make sure only one instance accesses the USB port.
	 * Multiple transfers in parallel would fail because the 
	 * port can only be opened by a single process.
	 */
	if( acquireLock()) {
		printfLog("Failed to acquire lock.\n");
		printfDebug("Failed to acquire lock.\n");
		exit(-1);
	}
	
	usbDevH = usbOpen( device->vendor_id, device->product_id);
	if( !usbDevH) {
		releaseLock();
		exit(-1);
	}

	drainInput();

	while( (runOption = parseArguments( argc, argv)))
	{
		resetTransferBuffers();

		if( runOption == LIST) {
			nothingToDo = FALSE;
			listActions();
			printActions();

		} else if( runOption == QUERY) {
			nothingToDo = FALSE;
			queryAction( optionAction);

		} else if( runOption == SET) {
			nothingToDo = FALSE;
			setAction( optionAction);
			
		} else if( runOption == QUIT || runOption == ERROR) {
			break;
		}
		
		parameterCount = 0;
	}

	if( runOption == ERROR) {
		printfDebug("Exit with error.\n");

	} else if( nothingToDo) {
		printfDebug("Nothing to do, Querying all actions.\n");
		
		listActions();
		for( int action=0; action < actionCount; action++) {
			queryAction( actions[action]);
		}		
	}

	usbClose( usbDevH);
	
	releaseLock();
}

static void parseOptions( int argc, char **argv)
{
	char opt;
	const device_info_t *dev;

#define ALL_GETOPTS "lq:s:p:d:v?"

	while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {
		
		if( opt ==  'v') {
			debug = stdout;

	    } else if( opt == 'd') {
			device = NULL;
			dev = &(device_info[0]);
			while( dev->name != NULL) {
				if( strcmp( dev->name, optarg) == 0) {
					device = dev;
					break;
				}
				dev++;
			}

	    } else if( opt == '?') {
			usage();
			exit(0);
        }
	}
	
	optind = 1;
}

static enum RunOption parseArguments( int argc, char **argv)
{
	int state = 0;
	
#define CHECK_STATE( b) \
	if( state == 1) {   \
		optind -= b;    \
		break;          \
	}                   \
	state++
	
	char opt;
	enum RunOption runOption = QUIT;
	
	while((opt = getopt(argc, argv, ALL_GETOPTS)) != -1) {  

		/* This cannot be a switch() because the CHECK_STATE macro
		 * calls break to leave the loop.
		 */
        if(opt == 'l') {
			CHECK_STATE( 1);
			runOption = LIST;

		} else if( opt == 'q') {
			CHECK_STATE(2);
			runOption = QUERY;
			strncpy( optionAction, optarg, MAX_OPTION_ACTION_SIZE);
			
		} else if( opt == 's') {
			CHECK_STATE(2);
			runOption = SET;
			strncpy( optionAction, optarg, MAX_OPTION_ACTION_SIZE);

		} else if( opt == 'p') {
			if( state == 0) {
				printfLog( "No action for parameter: %s\n", optarg);
				runOption = ERROR;
				break;
			}
			/* @TODO check limit */
			parameters[parameterCount++] = optarg;
		}
    }

    return runOption;
}

static void usage()
{
	const device_info_t *dev;
	
	printf("\nusage: usb_rbduo_get [options] [command ... ] \n\n");
	printf("  Options:\n");	
	printf("    -d device_name             USB device type\n");
	
	printf("       Valid device names:\n");
	dev = &(device_info[0]);
	while( dev->name != NULL) {
		printf("         %s\n", dev->name);
		dev++;
	}
	
	printf("    -v                         Enable debug output\n");
	printf("    -?                         Print usage\n\n");
	printf("  Commands:\n");
	printf("    -l                         List supported actions\n");
	printf("    -q action [-p param ... ]  Query action\n");
	printf("    -s action [-p param ... ]  Set action\n\n");
	printf("  In case no command is specified all supported actions"
	        " are queried.\n\n");
}

/*******************************************************************/

/* Clear all buffers.
 */
static void resetTransferBuffers()
{
	sendBufferPtr = 0;
	sendBuffer[0] = '\0';
	receiveBufferPtr = 0;
	receiveBufferEnd = 0;
	receiveBuffer[0] = '\0';
	bufferedLine[0] = '\0';
}

/*******************************************************************/

/* Query the device for supported actions.
 */
static void listActions()
{
	char *line;
	char commandChar;
	
	printfDebug( "ListActions()\n");

	actionCount = 0;
	for( int i=0; i<MAX_ACTIONS; i++) {
		actions[i] = NULL;
	}
	
	sendCommand( LIST_ACTIONS, NULL);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData(parameters[i]);
	}
	sendEOT();
	
	line = receiveLine( &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			
			if( actionCount >= MAX_ACTIONS) { continue; }
			
			if( strlen(line) < MAX_ACTION_NAME_LEN) {
				actions[actionCount] = (char*)calloc( 1, strlen(line)+1);
				strcpy( actions[actionCount], line);
				actionCount++;
			} else {
				printfLog( "Action name exceeds length limit of %d: %s\n",
					MAX_ACTION_NAME_LEN, line);
			}
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}

		line = receiveLine( &commandChar);
	}
}

static void printActions()
{
	for( int action=0; action < actionCount; action++) {
		printf("%s\n",actions[action]);
	}	
}

/* Run actions and collect the result.
 */
static void queryAction( const char *action)
{
	char *line;
	char commandChar;
	FILE *fp = NULL;

	printfDebug( "QueryAction()\n");

	sendCommand( QUERY_ACTION, action);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData(parameters[i]);
	}
	sendEOT();
	
	line = receiveLine( &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			if( fp == NULL) {
				fp = openFileForWrite( action, OUTPUT_EXT);
			}
			
			if (fp != NULL)	{
				fprintf(fp, "%s\n", line);
			}
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}
		
		line = receiveLine( &commandChar);
	}
	
	if( fp != NULL) {
		fclose( fp);
	}
}

/* Run actions but do not expect a result.
 */
static void setAction( const char *action)
{
	char *line;
	char commandChar;

	printfDebug( "SetAction()\n");

	sendCommand( SET_ACTION, action);
	for( int i=0; i<parameterCount; i++) {
		sendMoreData(parameters[i]);
	}
	sendEOT();
	
	line = receiveLine( &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isNACK( commandChar)) {
			printfLog( "Error from USB device: %s\n", line);
			break;
		}
		
		line = receiveLine( &commandChar);
	}
}

/*******************************************************************/

static FILE *openFileForWrite( const char *action, const char *ext)
{
	FILE *fp;
	char filePath[80];

	/* @TODO: check for string length while copying */
	strcpy( filePath, OUTPUT_PATH);
	strcat( filePath, FILE_SEPARATOR);
	strcatToLower( filePath, action);
	strcat( filePath, ext);

	fp = fopen( filePath, "w");
	
	if (fp == NULL) {
		fprintf( stderr, "Failed to open file: %s\n", filePath);
	}

	return fp;
}

/*******************************************************************/

static int acquireLock()
{
	char filePath[80];
	int rc = -1;
	int sleepMsec = 0;
	
	/* @TODO: check for string length while copying */
	strcpy( filePath, OUTPUT_PATH);
	strcat( filePath, FILE_SEPARATOR);
	strcat( filePath, LOCK_FILE);

	lockFile = open( filePath, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
	
	if( lockFile < 0) {
		printfLog( "Error opening lock file [%s]: %d\n",
			filePath, lockFile);
		/* @TODO: rethink, continue without lock */
	}
	else {
		do {
			/* Try to acquire lock with non blocking option */
			rc = flock( lockFile, LOCK_EX | LOCK_NB);
			
			/* We try a couple of times to get the lock.
			 * If that fails for LOCK_SLEEP_LIMIT
			 * we raise an error and quit.
			 */
			if( rc && errno == EWOULDBLOCK) {
				usleep( LOCK_SLEEP_MSEC * USEC_TO_MSEC);
				sleepMsec += LOCK_SLEEP_MSEC;
				if( sleepMsec >= LOCK_SLEEP_LIMIT_MSEC) {
					break;
				}
			}
			else { /* Fail for any other error. */
				break;
			}
			
		} while( rc);
			
		if( rc) {
			printfLog( "Error locking file: %d\n", errno);
		}
	}

	return rc;
}

static void releaseLock()
{
	if( lockFile >= 0) {
		flock( lockFile, LOCK_UN);
		close( lockFile);
		lockFile = -1;
	}
}

/*******************************************************************/

static void printfLog( const char *format, ...)
{
	if( logFile == NULL) {
		logFile = openFileForWrite( "usb_rbduo_get", LOG_EXT);
	}
	
	if( logFile == NULL) {
		logFile = stderr;
	}

	if( logFile != NULL) {
		va_list vargs;
		va_start( vargs, format);
		
		vfprintf( logFile, format, vargs);
		
		va_end( vargs);
	}	
}

static void printfDebug( const char *format, ...)
{
	if( debug != NULL) {
		va_list vargs;
		va_start( vargs, format);
		
		vfprintf( debug, format, vargs);
		
		va_end( vargs);
	}	
}

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

/*******************************************************************/

/* Fetches a single line without command char from USB device.
 * Line termination character is NL.
 * CR characters will be silently dropped.
 * The command character (first character in line) is returned
 * separately.
 * It is NO_COMMAND when we run into a timeout.
 */
static char *receiveLine( char *commandChar)
{
	char ch;
	int ptr = 0;
	boolean skip_next = FALSE;
	
	*commandChar = NO_COMMAND;

	while( (ch = usbGetChar()) != '\0')	{

		/* @TODO HACK!!!
		 * Dump ascii 17 + 96 characters gotten from arduino... F*ck
		 * 17 is device control 1 aka CTRL-Q  =  Xon Handshake
		 */
		if( skip_next ) {
			skip_next = FALSE;
			continue;
		}
		
		if( ch == '\n') { break; }
		
		/* @TODO HACK !!! */
		if( ch == 17) { skip_next = TRUE; continue; }

		if( ch < ' ') { continue; }
		
	    if( *commandChar == NO_COMMAND) {
			*commandChar = ch;
		}
		else {
			bufferedLine[ptr++] = ch;
			if( ptr >= (MAX_BUFFER_SIZE-1)) { break; }
		}
	}
	
	bufferedLine[ptr] = '\0';

	printfDebug( "USB Recv: cmd=%c '%s'\n", *commandChar, bufferedLine);
	
	return bufferedLine;
}

static void sendMoreData( const char *data)
{
	sendCommand( MORE_DATA, data);
}

static void sendEOT()
{
	sendCommand( END_OF_TRANSMISSION, NULL);
}

static void sendError( const char *message)
{
	sendCommand( NACK_OR_ERROR, message);
}

static void sendCommand(char command, const char *data)
{
	int copied;
	
	sendBufferPtr = 0;
    sendBuffer[sendBufferPtr++] = command;
  
    if( data != NULL && strlen(data) > 0) {
		copied = strlen(data);
		strncpy( &(sendBuffer[sendBufferPtr]), data, MAX_BUFFER_SIZE-3);
		sendBufferPtr += copied;
	}

	sendBuffer[sendBufferPtr++] = '\n';
	sendBuffer[sendBufferPtr++] = '\0';
	
	usbSendBuffer( sendBuffer);
}

/*******************************************************************/

/* Open USB device by vendor and product id.
 * Returns NULL if the device was not found or we run into an error.
 */
static libusb_device_handle* usbOpen( uint16_t vendor_id, uint16_t product_id)
{
	int rc;
	
	libusb_device_handle *usbDevH = NULL;

	printfDebug( "Initializing libusb.\n");
	
	rc = libusb_init( NULL);
	if( rc < 0) {
		printfLog( "Error initializing libusb: %s\n",
			libusb_error_name(rc));
		return NULL;
	}

	printfDebug( "Opening USB device. [vendor=%p,product=%p]\n",
		vendor_id, product_id);
	
	usbDevH = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
	if (!usbDevH) {
		printfLog( 
			"Error finding USB device: vendor=0x%x product=0x%x\n",
			vendor_id, product_id);
		usbClose( usbDevH);
		return NULL;
	}

	printfDebug( "Claiming USB interface.\n");

	/* As we are dealing with a CDC-ACM device, it's highly probable
	 * that Linux already attached the cdc-acm driver to this device.
     * We need to detach the drivers from all the USB interfaces.
     * The CDC-ACM Class defines two interfaces: the Control interface
     * and the Data interface.
     */ 
	for (int if_num = 0; if_num < device->if_count; if_num++) {
		if (libusb_kernel_driver_active(usbDevH, if_num)) {
			libusb_detach_kernel_driver(usbDevH, if_num);
		}
    
		rc = libusb_claim_interface(usbDevH, if_num);
		if (rc < 0) {
			printfLog( "Error claiming interface [%d]: %s\n",
				if_num, libusb_error_name(rc));
			usbClose( usbDevH);
			return NULL;
		}
	}

	printfDebug( "USB interface(s) claimed.\n");

	printfDebug( "USB device opened.\n");

	return usbDevH;
}

/* Close USB port.
 * It is ok to pass NULL.
 */
static void usbClose( libusb_device_handle* usbDevH)
{
	if( usbDevH) {
	    libusb_close( usbDevH);
	}

	libusb_exit( NULL);
	
	printfDebug( "USB device closed.\n");	
}

/* Send a char array via USB bulk transfer.
 */
static boolean usbSendBuffer(char *buf)
{
	int rc;
	int sentBytes;
	
	printfDebug( "USB Send: %s", buf);
	
	rc = libusb_bulk_transfer(usbDevH,
	                          device->out_endp,
	                          (unsigned char*)buf,
	                          strlen((char*)buf),
	                          &sentBytes,
	                          0);

	if( rc < 0)	{
		printfLog( "Error (rc=%d) while sending '%s'\n", rc, buf);
		return FALSE;
	}
	
	return TRUE;
}

/* Fetch the next character from USB device.
 * Returns NUL if we run into timeout.
 */
static char usbGetChar()
{
	int rc;
	char ch = '\0';

	/* We cannot assume that usb bulk transfer returns a single line 
	 * or even a single packet as a whole.
	 * We need to fetch data in junks and look for our marker 
	 * characters which is done in the calling function.
	 */
	if( receiveBufferPtr == receiveBufferEnd) {
		
		receiveBufferPtr = receiveBufferEnd = 0;
		
		rc = libusb_bulk_transfer(
			usbDevH,
			device->in_endp,
			(unsigned char*)receiveBuffer,
			MAX_BUFFER_SIZE,
			&receiveBufferEnd,
			RECEIVE_TIMEOUT_MSEC);
		
		printfDebug("usbGetChar %d chars rc=%d: ", receiveBufferEnd, rc);
		
		for( int i=0; i<receiveBufferEnd; i++) {
			printfDebug("%d ", receiveBuffer[i]);
		}
		printfDebug("\n");
		
		/* Only if the buffer is empty we can be sure that we run
		 * into a timeout.
		 */
		if (rc == LIBUSB_ERROR_TIMEOUT && receiveBufferEnd == 0) {
			printfLog( "Timeout waiting for USB bulk transfer.\n");
		}
		else if (rc < 0) {
			printfLog( "Error waiting for USB bulk transfer. rc=%d\n", rc);
		}
	}

	if( receiveBufferPtr < receiveBufferEnd) {
		ch = receiveBuffer[receiveBufferPtr++];
	}
	
	return ch;
}

/* Drain left over input from USB bus.
 * This is a rare condition, but may happen if a process crashes.
 */
static void drainInput()
{
	int rc;
	
	do {
		rc = libusb_bulk_transfer(
			usbDevH,
			device->in_endp,
			(unsigned char*)receiveBuffer,
			MAX_BUFFER_SIZE,
			&receiveBufferEnd,
			DRAIN_TIMEOUT_MSEC);
	
		printfDebug("DrainInput %d chars rc=%d: ", receiveBufferEnd, rc);
		
		for( int i=0; i<receiveBufferEnd; i++) {
			printfDebug("%d ", receiveBuffer[i]);
		}
		printfDebug("\n");
				
		if (rc == LIBUSB_ERROR_TIMEOUT && receiveBufferEnd == 0) {
			break;
		}
		else if (rc < 0) {
			printfLog( "Error waiting for USB bulk transfer. rc=%d\n", rc);
			break;
		}
	} while( rc == 0);

	receiveBufferEnd = 0;
}
