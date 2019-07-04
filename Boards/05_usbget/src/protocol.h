/*
 * protocol.h
 * 
 * Communication protocol.
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
 * 
 * Supported command characters:
 * =============================
 *   I     Query info (Version number... )
 *	 L     Query action list
 *   Q     Execute and query action
 *   S     Set variable
 *   C     Query action configuration
 *   +     Additional data
 *   .     End of transfer
 *   /     NACK or error response
 *   <nl>  Newline
 *
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
 * Set configuration
 * -----------------
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
 */

#include <usb.h>


/* Protocol command characters */
typedef enum {
	NO_COMMAND          = '\0',
	QUERY_CONFIG        = 'C',
	INFO_COMMAND        = 'I',
    LIST_ACTIONS        = 'L',
    QUERY_ACTION        = 'Q',
    SET_ACTION          = 'S',
    MORE_DATA           = '+',
    END_OF_TRANSMISSION = '.',
    NACK_OR_ERROR       = '/'
} ProtocolChar;

#define TO_char( p) ((char)p)
#define TO_ProtocolChar( c) ((ProtocolChar)c)

#define isNoCommand( cmd) (cmd == NO_COMMAND)
#define isEOT( cmd) (cmd == END_OF_TRANSMISSION)
#define isMoreData( cmd) (cmd == MORE_DATA)
#define isNACK( cmd) (cmd == NACK_OR_ERROR)


/* Fetches a single line without command char from USB device.
 * Line termination character is NL.
 * CR characters will be silently dropped.
 * The command character (first character in line) is returned
 * separately.
 * It is NO_COMMAND when we run into a timeout.
 */
char *receiveLine( usbDevice *device, ProtocolChar *commandChar);

/* Send "more data" protocol line to device.
 */
returnCode sendMoreData( usbDevice *device, const char *data);

/* Send "end of transmission" protocol line to device.
 */
returnCode sendEOT( usbDevice *device);

/* Send "error" protocol line to device.
 */
returnCode sendError( usbDevice *device, const char *message);

/* Send command to device.
 * 
 * data can be NULL.
 */
returnCode sendCommand( usbDevice *device,
                        ProtocolChar command,
                        const char *data);
