/*
 * protocol.h
 * 
 * Communication protocol.
 * 
 */

#include <usb.h>

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


char *receiveLine( usbDevice *device, char *commandChar);

returnCode sendMoreData( usbDevice *device, const char *data);

returnCode sendEOT( usbDevice *device);

returnCode sendError( usbDevice *device, const char *message);

returnCode sendCommand( usbDevice *device, char command, const char *data);

