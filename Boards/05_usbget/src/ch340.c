/*
 * ch340.c
 *
 * CH340 USB => serial converter specific initialization.
 *
 * File History
 * ============
 *   wolfix      30-Oct-2019  Creation
 *
 */

#include "ch340.h"

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
 *  LIBUSB_REQUEST_TYPE_VENDOR    10
 *  LIBUSB_RECIPIENT_DEVICE         0 0000
 *  --------------------------------------
 *                               0100 0000 = 0x40
 */
#define CH340_DEVICE_OUT_REQTYPE (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)

/* Request types */

//Vendor define
#define VENDOR_WRITE_TYPE       0x40
#define VENDOR_READ_TYPE        0xC0

#define VENDOR_READ             0x95
#define VENDOR_WRITE            0x9A
#define VENDOR_SERIAL_INIT      0xA1
#define VENDOR_MODEM_OUT        0xA4
#define VENDOR_VERSION          0x5F

//For CMD 0xA4
#define UART_CTS        0x01
#define UART_DSR        0x02
#define UART_RING       0x04
#define UART_DCD        0x08
#define CONTROL_OUT     0x10
#define CONTROL_DTR     0x20
#define CONTROL_RTS     0x40

//Uart state
#define UART_STATE          0x00
#define UART_OVERRUN_ERROR  0x01
#define UART_BREAK_ERROR    //no define
#define UART_PARITY_ERROR   0x02
#define UART_FRAME_ERROR    0x06
#define UART_RECV_ERROR     0x02
#define UART_STATE_TRANSIENT_MASK   0x07

//Port state
#define PORTA_STATE     0x01
#define PORTB_STATE     0x02
#define PORTC_STATE     0x03


/* CH340 specific libusb initialization code.
 * Sets communication parameters like baud rate, number of data bits,
 * parity...
 *
 * Returns: RC_OK on success
 *          RC_ERROR on any error
 */
returnCode initCH340( struct libusb_device_handle *devH, uint32_t bd)
{
    int rc;

    uint8_t dtr = 0;
    uint8_t rts = 0;

    uint16_t value = (uint16_t)0;
    uint16_t index = (uint16_t)0;
    uint16_t index2 = (uint16_t)0;

    printfDebug( "CH340: opening with %d baud\n", bd);

    /* CH340 device serial init */

    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_SERIAL_INIT,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to init USB: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    /* Set communication parameters */

    value = (uint16_t)0x2518;
    index = (uint16_t)0x0050;

    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_WRITE,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to set line properties: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    value = (uint16_t)0x501f;
    index = (uint16_t)0xd90a;

    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_SERIAL_INIT,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC)) < 0) {
        printfLog( "Failed to set line properties: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    /* Set baud rate */

     switch (bd / 100) {
        case 24:
            index = (uint16_t)0xd901;
            index2 = (uint16_t)0x0038;
            break;
        case 48:
            index = (uint16_t)0x6402;
            index2 = (uint16_t)0x001f;
            break;
        case 96:
            index = (uint16_t)0xb202;
            index2 = (uint16_t)0x0013;
            break;
        case 192:
            index = (uint16_t)0xd902;
            index2 = (uint16_t)0x000d;
            break;
        case 384:
            index = (uint16_t)0x6403;
            index2 = (uint16_t)0x000a;
            break;
        case 1152:
        case 1153:
            index = (uint16_t)0xcc03;
            index2 = (uint16_t)0x0008;
            break;
        default:
            printfLog( "Unknown baudrate: Use 2400,4800,9600,19200,38400,115200\n");
            return RC_ERROR;
    }

    value = (uint16_t)0x1312;
    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_WRITE,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC) < 0)) {
        printfLog( "Failed to set baudrate (1): %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    value = (uint16_t)0x0f2c;
    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_WRITE,
                                value,
                                index2,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC) < 0)) {
        printfLog( "Failed to set baudrate (2): %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    /* Set handshake lines */

    value = (uint16_t)~((dtr?1<<5:0)|(rts?1<<6:0));
    index = (uint16_t)0;

    if( (rc = libusb_control_transfer(devH,
                                CH340_DEVICE_OUT_REQTYPE,
                                VENDOR_MODEM_OUT,
                                value,
                                index,
                                NULL,
                                (uint16_t)0,
                                TRANSMIT_TIMEOUT_MSEC) < 0)) {
        printfLog( "Failed to set handshake lines: %s\n",
                   libusb_error_name(rc));
        return RC_ERROR;
    }

    return RC_OK;
}
