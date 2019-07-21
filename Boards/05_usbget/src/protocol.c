/*
 * protocol.c
 *
 * A simple communication protocol.
 *
 */

#include <support.h>
#include <protocol.h>


/* Transfer buffers */
#define MAX_BUFFER_SIZE         256

static char sendBuffer[MAX_BUFFER_SIZE];
static int sendBufferPtr;

/* Received line without command char */
static char bufferedLine[MAX_BUFFER_SIZE];


/* Fetches a single line without command char from USB device.
 * Line termination character is NL.
 * CR characters will be silently dropped.
 * The command character (first character in line) is returned
 * separately.
 * It is NO_COMMAND when we run into a timeout.
 */
char *receiveLine( usbDevice *device, ProtocolChar *commandChar)
{
    char ch;
    int ptr = 0;

    *commandChar = NO_COMMAND;

    for(;;) {

        ch = usbGetChar( device);

        if( ch == '\0') { break; }
        if( ch == '\n') { break; }
        if( ch < ' ') { continue; }

        if( *commandChar == NO_COMMAND) {
            *commandChar = TO_ProtocolChar( ch);
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

/* Send "more data" protocol line to device.
 */
returnCode sendMoreData( usbDevice *device, const char *data)
{
    return sendCommand( device, MORE_DATA, data);
}

/* Send "end of transmission" protocol line to device.
 */
returnCode sendEOT( usbDevice *device)
{
    return sendCommand( device, END_OF_TRANSMISSION, NULL);
}

/* Send "error" protocol line to device.
 */
returnCode sendError( usbDevice *device, const char *message)
{
    return sendCommand( device, NACK_OR_ERROR, message);
}

/* Send command to device.
 *
 * data can be NULL.
 */
returnCode sendCommand( usbDevice *device,
                        ProtocolChar command,
                        const char *data)
{
    int copied;

    if( !device) {
        printfLog( "Failed to send command '%c'. No device.", command);
        return RC_ERROR;
    }

    sendBufferPtr = 0;
    sendBuffer[sendBufferPtr++] = TO_char(command);

    if( data != NULL && strlen(data) > 0) {
        copied = strlen(data);
        strncpy( &(sendBuffer[sendBufferPtr]), data,
                 MAX_BUFFER_SIZE-sendBufferPtr-1);

        sendBufferPtr += copied;
    }

    sendBuffer[sendBufferPtr++] = '\n';
    sendBuffer[sendBufferPtr++] = '\0';

    return usbSendBuffer( device, sendBuffer);
}
