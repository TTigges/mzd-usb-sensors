/*
 * usb.h
 * 
 * USB support.
 * 
 */


/* USB receive timeout */
#define COMMAND_TIMEOUT_MSEC   5000
#define RECEIVE_TIMEOUT_MSEC   1000
#define TRANSMIT_TIMEOUT_MSEC  5000
#define DRAIN_TIMEOUT_MSEC        1

#define RECEIVE_BUFFER_SIZE      64


typedef struct usbDevice usbDevice;

const char *usbEnumDeviceNames( unsigned int *idx);

usbDevice* usbOpen( const char *devName);

void usbClose( usbDevice **device);

returnCode usbSendBuffer( usbDevice *device, char *buf);

char usbGetChar( usbDevice *device);

void usbDrainInput( usbDevice *device);

void usbResetBuffers( usbDevice *device);
