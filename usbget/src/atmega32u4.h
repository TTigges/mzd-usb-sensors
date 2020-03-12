/*
 * atmega32u4.h
 *
 * ATMEGA32U4 USB => serial converter specific initialization.
 *
 */

#ifndef _USBGET_ATMEGA32U4_H
#define _USBGET_ATMEGA32U4_H

#include "support.h"
#include <libusb.h>

/* ATMEGA32U4 specific libusb initialization code.
 * Sets communication parameters like baud rate, number of data bits,
 * parity...
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initATMEGA32U4( struct libusb_device_handle *devH);

#endif
