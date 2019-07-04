/*
 * ftdi.h
 * 
 * FTDI USB => serial converter specific initialization.
 * 
 */

#include <ftdi.h>

#define TRANSMIT_TIMEOUT_MSEC  5000

#define FTDI_DEVICE_OUT_REQTYPE (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT)
#define FTDI_DEVICE_IN_REQTYPE (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN)

/* Request types */
#define SIO_RESET          0
#define SIO_MODEM_CTRL     1 
#define SIO_SET_FLOW_CTRL  2
#define SIO_SET_BAUDRATE   3
#define SIO_SET_DATA       4

#define SIO_SET_EVENT_CHAR 6
#define SIO_SET_ERROR_CHAR 7

/* Reset request */
#define SIO_RESET_SIO 0

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
returnCode initFTDI( struct libusb_device_handle *devH) 
{
	unsigned short value = 0;
	unsigned short index = 1;


	/* Reset FTDI device.
	 */ 
	if (libusb_control_transfer(devH,
								FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET,
                                SIO_RESET_SIO,
                                index,
                                NULL,
                                0,
                                TRANSMIT_TIMEOUT_MSEC) < 0) {
		printfLog( "Failed to reset USB.");
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
	value = 0x0008;
	index = 1;
	
    if (libusb_control_transfer(devH,
								FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_DATA,
                                value,
                                index,
                                NULL,
                                0,
                                TRANSMIT_TIMEOUT_MSEC) < 0) {
		printfLog( "Failed to set line properties.");
		return RC_ERROR;
	}


	/* Set baud rate: 19200
	 * 
	 * value = 3000000 / baud rate
	 *
	 *  9600 baud
	 *	  value = 0x38;
	 *    index = 0x41;
	 * 19200 baud
	 *    value = 0x9c;
	 *    index = 0x80;
	 * 115384 baud 
	 *    value = 0x1a;
	 *    index = 0x00;
	 */
	value = 0x9c;
	index = 0x80;

	if (libusb_control_transfer(devH,
								FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_BAUDRATE,
                                value,
                                index,
                                NULL,
                                0,
                                TRANSMIT_TIMEOUT_MSEC) < 0) {
		printfLog( "Failed to set baudrate.");
		return RC_ERROR;
	}


	/* Flow control
	 */
	value = 0;
	index = 1;

	unsigned short flowctrl = SIO_XON_XOFF_HS;

	if (libusb_control_transfer(devH,
								FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_FLOW_CTRL,
                                0,
                                flowctrl | index,
								NULL,
								0,
								TRANSMIT_TIMEOUT_MSEC) < 0) {
		printfLog( "Failed to set flow control.");
		return RC_ERROR;
	}


	/* Disable event character
	 */
	value = 0;

    if (libusb_control_transfer(devH,
								FTDI_DEVICE_OUT_REQTYPE,
								SIO_SET_EVENT_CHAR,
								value,
								index,
								NULL,
								0,
								TRANSMIT_TIMEOUT_MSEC) < 0) {
		printfLog( "Failed to disable event char.");
		return RC_ERROR;
	}
	
	return RC_OK;
}
