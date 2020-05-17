/*
 * usb.h
 *
 * USB support.
 *
 * File History
 * ============
 *   wolfix      29-Feb-2020  (0.1.3) Support default device
 */

#ifndef _USBGET_USB_H
#define _USBGET_USB_H

#include "support.h"
#include <libusb.h>

/* USB receive timeout */
#define COMMAND_TIMEOUT_MSEC   2000
#define RECEIVE_TIMEOUT_MSEC    500
#define TRANSMIT_TIMEOUT_MSEC  2000

/* Must be < 16 msec. for FTDI !
 * The FTDI chip sends status bytes every 16 msec. even if there is
 * no data.
 */
#define DRAIN_TIMEOUT_MSEC        1

#define RECEIVE_BUFFER_SIZE      64

#define MAX_DEVICENAME_LEN       20

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

/* List vendor ID and product ID of USB devices.
 */
void usbList( void);

/* Try to find a device connected to USB and return its name.
 */
void usbGetDefaultDevice( char *devName, uint16_t maxLen);

/* Open USB device by vendor and product id.
 * Returns NULL if the device was not found or we run into an error.
 */
usbDevice* usbOpen( const char *devName, uint32_t bd);

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

#endif

