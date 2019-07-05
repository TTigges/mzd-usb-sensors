/*
 * usb.h
 *
 * USB support.
 *
 */


/* USB receive timeout */
#define COMMAND_TIMEOUT_MSEC   4000
#define RECEIVE_TIMEOUT_MSEC   1000
#define TRANSMIT_TIMEOUT_MSEC  4000

/* Must be < 16 msec. for FTDI !
 * The FTDI chip sends status bytes every 16 msec. even if there is
 * no data.
 */
#define DRAIN_TIMEOUT_MSEC        1

#define RECEIVE_BUFFER_SIZE      64


/* Opaque device structure */
typedef struct usbDevice usbDevice;


/* Enumerate all device names.
 *
 * Example:
 *
 * unsigned int idx = 0;
 * const char *name;
 *
 * while( (name = usbEnumDeviceNames( &idx)) ) {
 *   printf( "device=%s\n", name);
 * }
 *
 */
const char *usbEnumDeviceNames( unsigned int *idx);

/* Open USB device by vendor and product id.
 * Returns NULL if the device was not found or we run into an error.
 */
usbDevice* usbOpen( const char *devName);

/* Release all interfaces and close USB port.
 *
 * The usbDevice structure will be deallocated.
 * It is ok to pass NULL.
 */
void usbClose( usbDevice **device);

/* Send a char array via USB bulk transfer.
 */
returnCode usbSendBuffer( usbDevice *device, char *buf);

/* Fetch the next character from USB device.
 * Returns NUL if we run into timeout or error.
 */
char usbGetChar( usbDevice *device);

/* Drain left over input from USB device.
 * This is a rare condition, but may happen if a process crashes.
 */
void usbDrainInput( usbDevice *device);

/* Reset receive buffer.
 */
void usbResetBuffers( usbDevice *device);
