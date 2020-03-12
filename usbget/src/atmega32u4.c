/*
 * atmega32u4.h
 *
 * ATMEGA32U4 USB => serial converter specific initialization.
 *
 * File History
 * ============
 *   wolfix      27-Jul-2019  Creation
 *
 */

#include "atmega32u4.h"

#define TRANSMIT_TIMEOUT_MSEC  ((unsigned int)5000)

/* USB Request type: uint8_t
 *
 * 0000 0000
 * +---------- Direction
 *               0 Host to device    LIBUSB_ENDPOINT_OUT
 *               1 Device to Host    LIBUSB_ENDPOINT_IN
 *  ++-------- Type
 *               00 Standard         LIBUSB_REQUEST_TYPE_STANDARD
 *               01 Class            LIBUSB_REQUEST_TYPE_CLASS
 *               10 Vendor           LIBUSB_REQUEST_TYPE_VENDOR
 *               11 Reserved         LIBUSB_REQUEST_TYPE_RESERVED
 *    + ++++-- Recipient
 *               00000 Device        LIBUSB_RECIPIENT_DEVICE
 *               00001 Interface     LIBUSB_RECIPIENT_INTERFACE
 *               00010 Endpoint      LIBUSB_RECIPIENT_ENDPOINT
 *               00011 Other         LIBUSB_RECIPIENT_OTHER
 *                     Reserved
 *
 */

/*
 *  LIBUSB_ENDPOINT_OUT          0
 *  LIBUSB_REQUEST_TYPE_CLASS     01
 *  LIBUSB_RECIPIENT_INTERFACE      0 0001
 *  --------------------------------------
 *                               0010 0001 = 0x21
 */
#define ATMEGA32U4_INTERFACE_OUT_REQTYPE (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS  | LIBUSB_RECIPIENT_INTERFACE)

#define ACM_CTRL_DTR   0x01
#define ACM_CTRL_RTS   0x02


/* ATMEGA32U4 specific libusb initialization code.
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initATMEGA32U4( struct libusb_device_handle *devH)
{
    int rc;

    if ( (rc = libusb_control_transfer( devH,
                                 ATMEGA32U4_INTERFACE_OUT_REQTYPE,
                                 0x22,
                                 ACM_CTRL_DTR | ACM_CTRL_RTS,
                                 0,
                                 NULL,
                                 (uint16_t)0,
                                 TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to set USB: %s\n", libusb_error_name(rc));
        return RC_ERROR;
    }

    return RC_OK;
}
