/*
 * ftdi.h
 *
 * FTDI USB => serial converter specific initialization.
 *
 * File History
 * ============
 *   wolfix      24-Jul-2019  Disable flow control.
 *
 */

#include "ftdi.h"

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
#define FTDI_INTERFACE_OUT_REQTYPE (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS  | LIBUSB_RECIPIENT_INTERFACE)

/*
 *  LIBUSB_ENDPOINT_OUT          0
 *  LIBUSB_REQUEST_TYPE_VENDOR    10
 *  LIBUSB_RECIPIENT_DEVICE         0 0000
 *  --------------------------------------
 *                               0100 0000 = 0x40
 */
#define FTDI_DEVICE_OUT_REQTYPE (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)

/* Request types */
#define SIO_RESET          ((uint8_t)0)
#define SIO_MODEM_CTRL     ((uint8_t)1)
#define SIO_SET_FLOW_CTRL  ((uint8_t)2)
#define SIO_SET_BAUDRATE   ((uint8_t)3)
#define SIO_SET_DATA       ((uint8_t)4)

#define SIO_SET_EVENT_CHAR ((uint8_t)6)
#define SIO_SET_ERROR_CHAR ((uint8_t)7)

/* Reset request */
#define SIO_RESET_SIO      ((uint16_t)0)

/* Flow control request */
#define SIO_DISABLE_FLOW_CTRL 0x0
#define SIO_RTS_CTS_HS (0x1 << 8)
#define SIO_DTR_DSR_HS (0x2 << 8)
#define SIO_XON_XOFF_HS (0x4 << 8)


/* FTDI specific libusb initialization code.
 * Sets communication parameters like baud rate, number of data bits,
 * parity...
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initFTDI( struct libusb_device_handle *devH, uint32_t bd)
{
    int rc;

    uint16_t value = (uint16_t)0;
    uint16_t index = (uint16_t)1;

    printfDebug( "FTDI: opening with %d baud\n", bd);

    /* Reset FTDI device.
     */
    if( (rc = libusb_control_transfer(devH,
                                FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET,
                                SIO_RESET_SIO,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to reset USB: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }


    /* Set communication parameters: 8N1, break off
     *
     * Bits   7                              0x0007
     *        8                              0x0008
     *
     * Parity N         0 0000 0000
     *        O         1 0000 0000          0x0100
     *        E        10 0000 0000          0x0200
     *        M        11 0000 0000          0x0300
     *        S       100 0000 0000          0x0400
     *
     * Stop   1      0000 0000 0000
     *       15      1000 0000 0000          0x0800
     *        2    1 0000 0000 0000          0x1000
     *
     * Break Off  00 0000 0000 0000
     *       On   10 0000 0000 0000          0x2000
     */
    value = (uint16_t)0x0008;
    index = (uint16_t)1;

    if( (rc = libusb_control_transfer(devH,
                                FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_DATA,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to set line properties: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }


    /* Set baud rate: 19200
     *
     * value = 3000000 / baud rate
     *
     *  9600 baud
     *    value = 0x38;
     *    index = 0x41;
     * 19200 baud
     *    value = 0x9c;
     *    index = 0x80;
     * 115384 baud
     *    value = 0x1a;
     *    index = 0x00;
     */
    if( (bd / 100) == 96) {
        value = (uint16_t)0x38;
        index = (uint16_t)0x41;

    } else if( (bd / 100) == 192) {
        value = (uint16_t)0x9c;
        index = (uint16_t)0x80;
        
    } else if( (bd / 1000) == 115) {
        value = (uint16_t)0x1a;
        index = (uint16_t)0x00;
        
    } else {
        printfLog( "Unknown baudrate: Use 9600,19200,115200\n");
        return RC_ERROR;
    }

    if( (rc = libusb_control_transfer(devH,
                                FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_BAUDRATE,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC) < 0)) {
        printfLog( "Failed to set baudrate: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }


    /* Flow control
     */
    value = (uint16_t)0;
    index = (uint16_t)1;

    unsigned short flowctrl = SIO_DISABLE_FLOW_CTRL;

    if( (rc = libusb_control_transfer(devH,
                                FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_FLOW_CTRL,
                                0,
                                flowctrl | index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC) < 0)) {
        printfLog( "Failed to set flow control: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }


    /* Disable event character
     */
    value = (uint16_t)0;

    if( (rc = libusb_control_transfer(devH,
                                FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_EVENT_CHAR,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to disable event char: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    return RC_OK;
}
