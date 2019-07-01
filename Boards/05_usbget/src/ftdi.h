/*
 * ftdi.h
 * 
 * FTDI USB => serial converter specific initialization.
 * 
 */

#include <support.h>
#include <libusb.h>

returnCode initFTDI( struct libusb_device_handle *devH);
