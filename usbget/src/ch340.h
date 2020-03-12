/*
 * ch340.h
 *
 * CH340 USB => serial converter specific initialization.
 *
 * File History
 * ============
 *   wolfix      30-Oct-2019  Creation
 */

#ifndef _USBGET_CH340_H
#define _USBGET_CH340_H

#include "support.h"
#include <libusb.h>

/* CH340 specific libusb initialization code.
 * Sets communication parameters like baud rate, number of data bits,
 * parity...
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initCH340( struct libusb_device_handle *devH, uint32_t bd);

#endif
