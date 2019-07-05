/*
 * ftdi.h
 *
 * FTDI USB => serial converter specific initialization.
 *
 */

#include <support.h>
#include <libusb.h>

/* FTDI specific libusb initialization code.
 * Sets communication parameters like baud rate, number of data bits,
 * parity...
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initFTDI( struct libusb_device_handle *devH);
