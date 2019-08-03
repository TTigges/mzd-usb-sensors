/*
 * usb.c
 *
 * USB support.
 *
 * File History
 * ============
 *   wolfix      21-Jul-2019  USB device open code.
 *                            usbList() added
 *                            Arduino Mico added.
 */

#include "usb.h"
#include "ftdi.h"
#include "atmega32u4.h"

typedef struct deviceInfo_t {
    const char *name;
    uint16_t vendorId;
    uint16_t productId;
    unsigned char inEndp;
    unsigned char outEndp;
    int ifCount;
    int special;               /* Which init code to run, see below */
} deviceInfo_t;

/* Run special init code */
#define INIT_NOTHING       0
#define INIT_FTDI          1
#define INIT_CH340         2
#define INIT_ATMEGA32U4    3

static const deviceInfo_t device_info[] = {
    {(const char*)"redbear_duo",
        0x2B04, 0xC058, 0x81, 0x01, 2, INIT_NOTHING},
    {(const char*)"arduino_pro",
        0x0403, 0x6001, 0x81, 0x02, 1, INIT_FTDI},
    {(const char*)"arduino_nano",
        0x0403, 0x6001, 0x81, 0x02, 1, INIT_FTDI},
    {(const char*)"arduino_nano_clone",
        0x1a86, 0x7523, 0x81, 0x02, 1, INIT_CH340},
    {(const char*)"arduino_micro",
        0x2341, 0x8037, 0x83, 0x02, 2, INIT_ATMEGA32U4},

    /* END MARKER. Do not remove! */
    {(const char*)NULL, 0x0000, 0x0000, 0x00, 0x00, 0, INIT_NOTHING}
};


/* Opaque structure returned to caller on initialization. */
struct usbDevice {
    struct libusb_device_handle *devHandle;
    const deviceInfo_t *devInfo;

    char receiveBuffer[RECEIVE_BUFFER_SIZE];
    int receiveBufferPtr;
    int receiveBufferEnd;
};


static const deviceInfo_t *deviceInfoByName( const char *devName);


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
const char *usbEnumDeviceNames( unsigned int *idx) {

    const deviceInfo_t *di;

    if( !idx) {
        return NULL;
    }

    if( (*idx) >= sizeof(device_info)/sizeof(deviceInfo_t))
    {
        return NULL;
    }

    di = &device_info[(*idx)];
    (*idx)++;

    return di->name;
}

/* List vendor ID and product ID of USB devices.
 */
void usbList( void)
{
    int rc;
    int i;
    ssize_t numDev;
    libusb_device **devList;
    libusb_device *dev;
    struct libusb_device_handle *devH;
    struct libusb_device_descriptor desc;

#define DESCRIPTOR_MAX_LEN  80
    unsigned char manufacturer[DESCRIPTOR_MAX_LEN];
    unsigned char product[DESCRIPTOR_MAX_LEN];

    rc = libusb_init( NULL);
    if( rc < 0) {
        printfLog( "Error initializing libusb: %s\n",
                   libusb_error_name( rc));
        return;
    }

    numDev = libusb_get_device_list( NULL, &devList);
    if( numDev > 0)
    {
        for( i=0; i < numDev; i++)
        {
            dev = devList[i];
            libusb_ref_device( dev);

            rc = libusb_get_device_descriptor( dev, &desc);
            if( rc < 0) {
                printfLog( "Error getting device descriptor: %s\n",
                           libusb_error_name( rc));
                continue;
            }

            manufacturer[0] = '\0';
            product[0] = '\0';

            rc = libusb_open( dev, &devH);
            if( rc == 0) {
                libusb_get_string_descriptor_ascii(
                    devH,
                    desc.iManufacturer,
                    manufacturer, DESCRIPTOR_MAX_LEN);

                libusb_get_string_descriptor_ascii(
                    devH,
                    desc.iProduct,
                    product, DESCRIPTOR_MAX_LEN);

                libusb_close( devH);

            } else { /* Most likely missing privileges */
                strncpy( (char*)manufacturer, (char*)"- no access -",
                         DESCRIPTOR_MAX_LEN);
            }

            printf( "VId=%04x PId=%04x [%s] %s\n",
                    desc.idVendor, desc.idProduct,
                    manufacturer, product);

            libusb_unref_device( dev);
        }
    }

    libusb_free_device_list( devList, 0);

    libusb_exit( NULL);
}

/* Open USB device by vendor and product id.
 * Returns NULL if the device was not found or we run into an error.
 */
usbDevice *usbOpen( const char *devName)
{
    int rc;
    int i;
    ssize_t numDev;
    libusb_device **devList;
    libusb_device *dev;
    libusb_device *devFound = NULL;
    struct libusb_device_descriptor desc;
    int if_num;
    usbDevice *device;
    const deviceInfo_t *devInfo = NULL;
    struct libusb_device_handle *devH = NULL;

    devInfo = deviceInfoByName( devName);

    if( devInfo == NULL) {
        printfLog( "No device info for device named: %s\n", devName);
        return NULL;
    }

    printfDebug( "Initializing libusb.\n");

    rc = libusb_init( NULL);
    if( rc < 0) {
        printfLog( "Error initializing libusb: %s\n",
                   libusb_error_name( rc));
        return NULL;
    }

    printfDebug( "Searching for USB device: [vendor=%p,product=%p]\n",
                 devInfo->vendorId, devInfo->productId);

    numDev = libusb_get_device_list( NULL, &devList);
    if( numDev > 0)
    {
        for( i=0; i < numDev; i++)
        {
            dev = devList[i];
            libusb_ref_device( dev);

            rc = libusb_get_device_descriptor( dev, &desc);
            if( rc < 0) {
                printfLog( "Error getting device descriptor: %s\n",
                           libusb_error_name( rc));
                continue;
            }

            if(    desc.idVendor  == devInfo->vendorId
                && desc.idProduct == devInfo->productId)
            {
                devFound = dev;
                break;
            }

            libusb_unref_device( dev);
        }
    }
    else
    {
        printfLog( "No USB devices.\n");
        usbClose( NULL);
        return NULL;
    }

    if( devFound == NULL) {
        printfLog( "No suitable USB device found.\n");
        usbClose( NULL);
        return NULL;
    }

    printfDebug( "Opening USB device.\n");

    rc = libusb_open( devFound, &devH);

    libusb_unref_device( devFound);
    libusb_free_device_list( devList, 0);

    if( rc < 0) {
        printfLog( "Error opening USB device: %s\n",
                   libusb_error_name( rc));
        usbClose( NULL);
        return NULL;
    }

    printfDebug( "Claiming USB interface.\n");

    /* Detach kernel driver and claim interfaces. */
    for (if_num = 0; if_num < devInfo->ifCount; if_num++) {
        if (libusb_kernel_driver_active(devH, if_num)) {
            libusb_detach_kernel_driver(devH, if_num);
        }

        rc = libusb_claim_interface(devH, if_num);
        if (rc < 0) {
            printfLog( "Error claiming interface [%d]: %s\n",
                if_num, libusb_error_name(rc));
            usbClose( NULL);
            return NULL;
        }
    }

    printfDebug( "USB interface(s) claimed.\n");

    /* Optionally run special init code */

    switch( devInfo->special) {

    case INIT_FTDI:
        printfDebug( "Running FTDI initialization code.\n");
        if( initFTDI( devH) != RC_OK) {
            printfLog( "Failed to run init code for FTDI.\n");
            usbClose( NULL);
            return NULL;
        }
        break;

    case INIT_CH340:

        printfLog( "CH340 is not supported yet.\n");
        usbClose( NULL);
        return NULL;

    case INIT_ATMEGA32U4:
        printfDebug( "Running ATMEGA32U4 initialization code.\n");
        if( initATMEGA32U4( devH) != RC_OK) {
            printfLog( "Failed to run init code for ATMEGA32U4.\n");
            usbClose( NULL);
            return NULL;
        }
        break;
        
    default:
        break;
    }

    printfDebug( "USB device opened.\n");

    device = (usbDevice*)malloc( sizeof( usbDevice));

    device->devHandle = devH;
    device->devInfo = devInfo;

    return device;
}

/* Release all interfaces and close USB port.
 *
 * The usbDevice structure will be deallocated.
 * It is ok to pass NULL.
 */
void usbClose( usbDevice **device)
{
    int rc;
    int ifNum;

    if( device && (*device)) {
        if( (*device)->devHandle) {
            for (ifNum = 0; ifNum < (*device)->devInfo->ifCount; ifNum++) {
                rc = libusb_release_interface((*device)->devHandle, ifNum);
                if (rc < 0) {
                    printfLog( "Error releasing interface [%d]: %s\n",
                               ifNum, libusb_error_name(rc));
                }
            }

            libusb_close( (*device)->devHandle);
        }

        free( *device);
        *device = NULL;
    }

    libusb_exit( NULL);

    printfDebug( "USB device closed.\n");
}

/* Send a char array via USB bulk transfer.
 */
returnCode usbSendBuffer( usbDevice *device, char *buf)
{
    int rc;
    int sentBytes;

    if( device == NULL) {
        return RC_ERROR;
    }

    printfDebug( "USB Send (%d) : %s", strlen(buf), buf);

    rc = libusb_bulk_transfer(device->devHandle,
                              device->devInfo->outEndp,
                              (unsigned char*)buf,
                              strlen((char*)buf),
                              &sentBytes,
                              TRANSMIT_TIMEOUT_MSEC);

    if( rc < 0) {
        printfLog( "Error (rc=%d) while sending '%s'\n", rc, buf);
        return RC_ERROR;
    }

    return RC_OK;
}

/* Fetch the next character from USB device.
 * Returns NUL if we run into timeout or error.
 */
char usbGetChar( usbDevice *device)
{
    int rc;
    int i;
    char ch = '\0';
    long startTimeMSec = timeMSec();

    if( device == NULL) {
        return ch;
    }

    /* We cannot assume that usb bulk transfer returns a single line
     * or even a single packet as a whole.
     * We need to fetch data in junks and look for our marker
     * characters which is done in the calling function.
     */
    if( device->receiveBufferPtr == device->receiveBufferEnd) {
        while( TRUE) {
            usbResetBuffers( device);

            if( timeMSec() > startTimeMSec + COMMAND_TIMEOUT_MSEC) {
                printfLog( "receiveLine() timed out after %d msec.\n",
                           COMMAND_TIMEOUT_MSEC);
                break;
            }

            rc = libusb_bulk_transfer( device->devHandle,
                                    device->devInfo->inEndp,
                                   (unsigned char*)device->receiveBuffer,
                                   RECEIVE_BUFFER_SIZE,
                                   &device->receiveBufferEnd,
                                   RECEIVE_TIMEOUT_MSEC);

            printfDebug( "usbGetChar %d chars rc=%d: ",
                         device->receiveBufferEnd, rc);

            for( i=0; i < device->receiveBufferEnd; i++) {
                printfDebug( "%d ", device->receiveBuffer[i]);
            }

            printfDebug( "\n");

            /* Only if the buffer is empty we can be sure that we run
             * into a timeout.
             */
            if (rc == LIBUSB_ERROR_TIMEOUT && device->receiveBufferEnd == 0) {
                continue;
            }
            else if (rc < 0) {
                printfLog(
                    "Error waiting for USB bulk transfer. rc=%d\n",
                    rc);
                break;
            }

            if( device->devInfo->special == INIT_FTDI) {
                /* First two characters are status bytes.
                 * We can skip them.
                 */
                if( device->receiveBufferEnd <= 2) {
                    continue;
                } else {
                    device->receiveBufferPtr = 2;
                }
            }
            break;
        }
    }

    if( device->receiveBufferPtr < device->receiveBufferEnd) {
        ch = device->receiveBuffer[device->receiveBufferPtr++];
    }

    return ch;
}

/* Drain left over input from USB device.
 * This is a rare condition, but may happen if a process crashes.
 */
void usbDrainInput( usbDevice *device)
{
    int rc;
    int i;

    if( device == NULL) {
        return;
    }

    do {
        rc = libusb_bulk_transfer( device->devHandle,
                                   device->devInfo->inEndp,
                                   (unsigned char*)device->receiveBuffer,
                                   RECEIVE_BUFFER_SIZE,
                                   &device->receiveBufferEnd,
                                   DRAIN_TIMEOUT_MSEC);

        printfDebug( "DrainInput %d chars rc=%d: ",
                     device->receiveBufferEnd, rc);

        for( i=0; i < device->receiveBufferEnd; i++) {
            printfDebug( "%d ", device->receiveBuffer[i]);
        }

        printfDebug("\n");

        if (rc == LIBUSB_ERROR_TIMEOUT && device->receiveBufferEnd == 0) {
            break;
        }
        else if (rc < 0) {
            printfLog( "Error waiting for USB bulk transfer. rc=%d\n",
                       rc);
            break;
        }
    } while( rc == 0);

    usbResetBuffers( device);
}

/* Reset receive buffer.
 */
void usbResetBuffers( usbDevice *device)
{
    if( device) {
        device->receiveBuffer[0] = '\0';
        device->receiveBufferPtr = 0;
        device->receiveBufferEnd = 0;
    }
}

/* ****************** static functions *************************** */

/* Find device info structure by name
 */
static const deviceInfo_t *deviceInfoByName( const char *devName)
{
    const deviceInfo_t *device;

    if( devName == NULL) {
        return NULL;
    }

    device = &(device_info[0]);
    while( device->name != NULL) {
        if( strcmp( device->name, devName) == 0) {
            return device;
        }
        device++;
    }

    return NULL;
}
