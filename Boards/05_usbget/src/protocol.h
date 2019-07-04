/*
 * protocol.h
 * 
 * Communication protocol.
 * 
 */

#include <usb.h>

/* Protocol command characters */
#define NO_COMMAND           '\0'
#define QUERY_CONFIG         'C'
#define INFO_COMMAND         'I'
#define LIST_ACTIONS         'L'
#define QUERY_ACTION         'Q'
#define SET_ACTION           'S'
#define MORE_DATA            '+'
#define END_OF_TRANSMISSION  '.'
#define NACK_OR_ERROR        '/'

/*
typedef enum {
	NO_COMMAND         = '\0',
	QUERY_CONFIG        = 'C',
	INFO_COMMAND        = 'I',
    LIST_ACTIONS =        'L',
    QUERY_ACTION =        'Q',
    SET_ACTION =          'S',
    MORE_DATA =           '+',
    END_OF_TRANSMISSION = '.',
    NACK_OR_ERROR =       '/'
} ProtocolChars;
*/

#define isNoCommand( cmd) (cmd == NO_COMMAND)
#define isEOT( cmd) (cmd == END_OF_TRANSMISSION)
#define isMoreData( cmd) (cmd == MORE_DATA)
#define isNACK( cmd) (cmd == NACK_OR_ERROR)


char *receiveLine( usbDevice *device, char *commandChar);

returnCode sendMoreData( usbDevice *device, const char *data);

returnCode sendEOT( usbDevice *device);

returnCode sendError( usbDevice *device, const char *message);

returnCode sendCommand( usbDevice *device, char command, const char *data);

