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
 * L<nl>                     =>
 * .<nl>                     =>
 *                          <=             +action1<nl>
 *                          <=             +action2<nl>
 *                          <=             .<nl>
 *
 * Execute and query action
 * ------------------------
 * Qaction1<nl>              =>
 * .<nl>                     =>
 *                          <=             +result line 1<nl>
 *                          <=             +result line 2<nl>
 *                          <=             .<nl>
 *
 * Variablen setzen
 * ----------------
 * Saction1<nl>              =>
 * +variable1=value1<nl>     =>
 * .<nl>                     =>
 *
 * 
 * In case of an error
 * -------------------
 * Qblabla<nl>               =>
 * .<nl>                     =>
 *                          <=             /Unknown function.
 * 
 * 
 * USB code based on mazda_tpms.c from Mazdaracerdude
 * 
 * wolfix      13-Jun-2019  Minor fixes and code cleanup
 * wolfix      12-Jun-2019  Initial version
 */
 
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <libusb.h>


#ifndef FALSE
#define FALSE (0)
#define TRUE  (!FALSE)
#endif

#ifndef boolean
typedef int boolean;
#endif

/* RedBear Duo */
static const uint16_t VENDOR_ID = 0x2B04;
static const uint16_t PRODUCT_ID = 0xC058;

/* The Endpoint address are hard coded.
 * Use lsusb -v to find the values corresponding to your device.
 */
static const unsigned char IN_ENDPOINT  = 0x81;
static const unsigned char OUT_ENDPOINT = 0x01;

#define OUTPUT_PATH    "/tmp/mnt/data_persist/dev/bin"
#define OUTPUT_EXT     ".out"
#define FILE_SEPARATOR "/"


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

/* USB receive timeout */
#define RECEIVE_TIMEOUT_MS 2000

/* Transfer buffers */
#define MAX_BUFFER_SIZE 256

static char sendBuffer[MAX_BUFFER_SIZE];
static int sendBufferPtr;

static char receiveBuffer[MAX_BUFFER_SIZE];
static int receiveBufferPtr;
static int receiveBufferEnd;

/* received line without command char */
static char bufferedLine[MAX_BUFFER_SIZE];

/* @TODO current limit of 10 actions */
#define MAX_ACTIONS 10
static char *actions[MAX_ACTIONS];
static int actionCount;

static struct libusb_device_handle *usbDevH;

static FILE *debug = NULL;


/****************** static foreward declarations *******************/

static void resetBuffers();

static void listActions();
static void queryAction( char *action);

static FILE *openFileForWrite( char *action);
static void strcatToLower( char *target, char *source);

static char *receiveLine( char *commandChar);

static void sendMoreData( char *data);
static void sendEOT();
static void sendError( char *message);
static void sendCommand(char command, char *data);

static libusb_device_handle* usbOpen( uint16_t vendor_id, uint16_t product_id);
static void usbClose( libusb_device_handle* usbDevH);
static boolean usbSendBuffer(char *buf);
static char usbGetChar();


/*******************************************************************/

int main( int argc, char **argv)
{
	/* @TODO command line option ? */
	// debug = stdout;
	
	usbDevH = usbOpen( VENDOR_ID, PRODUCT_ID);
	if( !usbDevH) {
		exit(-1);
	}

	resetBuffers();
	listActions();

	for( int action=0; action < actionCount; action++) {
		queryAction( actions[action]);
	}
	
	usbClose( usbDevH);
}

/*******************************************************************/

/* Clear all buffers.
 */
static void resetBuffers()
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
	
	actionCount = 0;
	for( int i=0; i<MAX_ACTIONS; i++) {
		actions[i] = NULL;
	}
	
	sendCommand( LIST_ACTIONS, NULL);
	sendEOT();
	
	line = receiveLine( &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			
			if( actionCount >= MAX_ACTIONS) { continue; }
			
			actions[actionCount] = (char*)calloc( 1, strlen(line)+1);
			strcpy( actions[actionCount], line);
			actionCount++;
		}
		else if( isNACK( commandChar)) {
			fprintf( stderr, "Error from USB device: %s\n", line);
			break;
		}

		line = receiveLine( &commandChar);
	}
}

/* Run actions and collect the result.
 */
static void queryAction( char *action)
{
	char *line;
	char commandChar;
	FILE *fp = NULL;

	sendCommand( QUERY_ACTION, action);
	sendEOT();
	
	line = receiveLine( &commandChar);
	
	while( !isNoCommand( commandChar)) {
		
		if( isEOT( commandChar)) {
			 break;
		}
		else if( isMoreData( commandChar)) {
			if( fp == NULL) {
				fp = openFileForWrite( action);
			}
			
			if (fp != NULL)	{
				fprintf(fp, "%s\n", line);
			}
		}
		else if( isNACK( commandChar)) {
			fprintf( stderr, "Error from USB device: %s\n", line);
			break;
		}
		
		line = receiveLine( &commandChar);
	}
	
	if( fp != NULL) {
		fclose( fp);
	}
}

/*******************************************************************/

static FILE *openFileForWrite( char *action)
{
	FILE *fp;
	char filePath[80];

	/* @TODO: check for string length while copying */
	strcpy( filePath, OUTPUT_PATH);
	strcat( filePath, FILE_SEPARATOR);
	strcatToLower( filePath, action);
	strcat( filePath, OUTPUT_EXT);

	fp = fopen( filePath, "w");
	
	if (fp == NULL) {
		fprintf( stderr, "Failed to open file: %s\n", filePath);
	}

	return fp;
}

static void strcatToLower( char *target, char *source)
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
	
	*commandChar = NO_COMMAND;

	while( (ch = usbGetChar()) != '\0')	{
		
		if( ch == '\r') { continue; }
		if( ch == '\n') { break; }

	    if( *commandChar == NO_COMMAND) {
			*commandChar = ch;
		}
		else {
			bufferedLine[ptr++] = ch;
			if( ptr >= MAX_BUFFER_SIZE) { break; }
		}
	}
	
	bufferedLine[ptr] = '\0';
	
	if( debug) {
		fprintf( debug, "From device: cmd=%c '%s'\n",
			*commandChar, bufferedLine);
	}
	
	return bufferedLine;
}

static void sendMoreData( char *data)
{
	sendCommand( MORE_DATA, data);
}

static void sendEOT()
{
	sendCommand( END_OF_TRANSMISSION, NULL);
}

static void sendError( char *message)
{
	sendCommand( NACK_OR_ERROR, message);
}

static void sendCommand(char command, char *data)
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

	if( debug) {
		fprintf( debug, "Opening USB device\n");
	}
  
	rc = libusb_init( NULL);
	if( rc < 0) {
		fprintf( stderr, "Error initializing libusb: %s\n",
			libusb_error_name(rc));
		return NULL;
	}

	/* @TODO Deprecated
	libusb_set_debug( NULL, LIBUSB_LOG_LEVEL_INFO);
	*/
	
	usbDevH = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
	if (!usbDevH) {
		fprintf( stderr,
			"Error finding USB device: vendor=0x%x product=0x%x\n",
			vendor_id, product_id);
		usbClose( usbDevH);
		return NULL;
	}

	/* As we are dealing with a CDC-ACM device, it's highly probable
	 * that Linux already attached the cdc-acm driver to this device.
     * We need to detach the drivers from all the USB interfaces.
     * The CDC-ACM Class defines two interfaces: the Control interface
     * and the Data interface.
     */
	for (int if_num = 0; if_num < 2; if_num++) {
		if (libusb_kernel_driver_active(usbDevH, if_num)) {
			libusb_detach_kernel_driver(usbDevH, if_num);
		}
    
		rc = libusb_claim_interface(usbDevH, if_num);
		if (rc < 0) {
			fprintf( stderr, "Error claiming interface: %s\n",
				libusb_error_name(rc));
			usbClose( usbDevH);
			return NULL;
		}
	}

	if( debug) {
		fprintf( debug, "USB device opened\n");
	}

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
}

/* Send a char array via USB bulk transfer.
 */
static boolean usbSendBuffer(char *buf)
{
	int rc;
	int sentBytes;
	
	if( debug) {
		fprintf( debug, "Send: %s", buf);
	}
	
	rc = libusb_bulk_transfer(usbDevH,
	                          OUT_ENDPOINT,
	                          (unsigned char*)buf,
	                          strlen((char*)buf),
	                          &sentBytes,
	                          0);

	if( rc < 0)	{
		fprintf( stderr, "Error (rc=%d) while sending '%s'\n", rc, buf);
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
			IN_ENDPOINT,
			(unsigned char*)receiveBuffer,
			MAX_BUFFER_SIZE,
			&receiveBufferEnd,
			RECEIVE_TIMEOUT_MS);
  
		/* Only if the buffer is empty we can be sure that we run
		 * into a timeout.
		 */
		if (rc == LIBUSB_ERROR_TIMEOUT && receiveBufferEnd == 0) {
			fprintf( stderr, "Timeout\n");
		}
		else if (rc < 0) {
			fprintf( stderr, "Error waiting for char rc=%d\n", rc);
		}
	}

	if( receiveBufferPtr < receiveBufferEnd) {
		ch = receiveBuffer[receiveBufferPtr++];
	}
	
	return ch;
}
