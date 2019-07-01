/*
 * protocol.c
 * 
 * Communication protocol.
 * 
 */
 
#include <support.h>

#include <protocol.h>

/* Transfer buffers */
#define MAX_BUFFER_SIZE         256

static char sendBuffer[MAX_BUFFER_SIZE];
static int sendBufferPtr;

/* received line without command char */
static char bufferedLine[MAX_BUFFER_SIZE];


/* Fetches a single line without command char from USB device.
 * Line termination character is NL.
 * CR characters will be silently dropped.
 * The command character (first character in line) is returned
 * separately.
 * It is NO_COMMAND when we run into a timeout.
 */
char *receiveLine( usbDevice *device, char *commandChar)
{
	char ch;
	int ptr = 0;
	boolean skip_next = FALSE;
	long startTimeMSec = timeMSec();
	
	*commandChar = NO_COMMAND;


	for(;;) {

		ch = usbGetChar( device);
	
		if( timeMSec() > startTimeMSec + COMMAND_TIMEOUT_MSEC) {
			printfLog( "receiveLine() timed out after %d msec.\n",
				       COMMAND_TIMEOUT_MSEC);
			break;
		}
		 
		/* @TODO HACK!!!
		 * Drop ascii 17 + 96 characters gotten from arduino... F*ck
		 * 17 is device control 1 aka CTRL-Q  =  Xon Handshake
		 */
		if( skip_next ) {
			skip_next = FALSE;
			continue;
		}
		
	    if( ch == '\0') { break; }
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

returnCode sendMoreData( usbDevice *device, const char *data)
{
	return sendCommand( device, MORE_DATA, data);
}

returnCode sendEOT( usbDevice *device)
{
	return sendCommand( device, END_OF_TRANSMISSION, NULL);
}

returnCode sendError( usbDevice *device, const char *message)
{
	return sendCommand( device, NACK_OR_ERROR, message);
}

returnCode sendCommand( usbDevice *device, char command, const char *data)
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
	
	return usbSendBuffer( device, sendBuffer);
}
