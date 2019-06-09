/*
 * This is a sample code to poll bluetooth based VC601 use the Readbears BLE Mini configure in HCI Mode
 * using libusb.
 *
 * Author: Mazdaracerdude
 */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <libusb-1.0/libusb.h>

char fbuf[] = "S[0]:FL: 30.00C 29.00psi ;S[1]:FL: 30.00C 29.00psi ;S[2]:RL: 30.00C 29.00psi ;S[3]:RR: 30.00C 29.00psi ;\n  "; 

// Redbear LABS BLE Mini product and vendor ID

#define VENDOR_ID      0x0451   
#define PRODUCT_ID     0x16aa  

#define ACM_CTRL_DTR   0x01
#define ACM_CTRL_RTS   0x02

#define TRUE 1
#define FALSE 0

// Update your Tire Sensors MAC Address here

#define SMAC1 0xaaaaaa		// Front Left Tire Bluetooth MAC Address
#define SMAC2 0xbbbbbb		// Front Right Tire Bluetooth MAC Address
#define SMAC3 0xcccccc		// Rear Left Tire Bluetooth MAC Address
#define SMAC4 0xdddddd		// Rear Right Tire Bluetooth MAC Address

// These were derived from the BLE_HCI github arduino codes of Redbear labs

#define GAP_PROFILE_BROADCASTER   0x01 //!< A device that sends advertising events only.
#define GAP_PROFILE_OBSERVER      0x02 //!< A device that receives advertising events only.
#define GAP_PROFILE_PERIPHERAL    0x04 //!< A device that accepts the establishment of an LE physical link using the connection establishment procedure
#define GAP_PROFILE_CENTRAL       0x08 //!< A device that supports the Central role initiates the establishment of a physical connection

#define DEVDISC_MODE_NONDISCOVERABLE  0x00    //!< No discoverable setting
#define DEVDISC_MODE_GENERAL          0x01    //!< General Discoverable devices
#define DEVDISC_MODE_LIMITED          0x02    //!< Limited Discoverable devices
#define DEVDISC_MODE_ALL              0x03    //!< Not filtered


#define GAP_MAX_SCAN_RESPONSE	      8 

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE                DEVDISC_MODE_ALL
// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN         TRUE
// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST          FALSE
// TRUE to use high scan duty cycle when creating link
#define DEFAULT_LINK_HIGH_DUTY_CYCLE          FALSE

// TRUE to use white list when creating link
#define DEFAULT_LINK_WHITE_LIST               FALSE



/* USB device handle
 */
static struct libusb_device_handle *devh = NULL;

/* The Endpoint address are hard coded. You should use lsusb -v to find
 * the values corresponding to your device.
 */
static int ep_in_addr  = 0x84;
static int ep_out_addr = 0x04;


int read_chars(unsigned char * data, int size)
{
    /* To receive characters from the device initiate a bulk_transfer to the
     * Endpoint with address ep_in_addr.
     */
    int actual_length;
    int rc = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actual_length,
                                  5000);
    if (rc == LIBUSB_ERROR_TIMEOUT) {
        return -1;
    } else if (rc < 0) {
        fprintf(stderr, "Error while waiting for char\n");
        return -1;
    }

    return actual_length;
}

// Function to initialize the BLE Mini board

void ble_init()
{
	int actual_length;

	unsigned char buf[] = {0x01,0x00,0xfe,0x26,GAP_PROFILE_CENTRAL,GAP_MAX_SCAN_RESPONSE,
			 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			 1,0,0,0};


    if (libusb_bulk_transfer(devh, ep_out_addr, buf, 42,
                             &actual_length, 0) < 0) 
        fprintf(stderr, "Error while sending char\n");

}

// Function to put the BLE Mini board in BLE Discovery mode

void ble_start_discovery()
{

	int actual_length;
	unsigned char buf[] = {0x01,0x04,0xfe,0x03,DEFAULT_DISCOVERY_MODE,DEFAULT_DISCOVERY_ACTIVE_SCAN,DEFAULT_LINK_WHITE_LIST};

	if (libusb_bulk_transfer(devh, ep_out_addr, buf, 7,
                             &actual_length, 0) < 0)
        fprintf(stderr, "Error while sending char\n");

} 



int main(int argc, char **argv)
{
    int i,rc;
	FILE *fp;

	unsigned char buf[65];
	char tbuf[30];

	long tmpval,tmpval2;
	long long event,smac;
	double pressure;
	double temperature;
	int data_len;
	unsigned char sid;
	
	/* Get the old TPMS data if it exists if not use the one in fbuf */
    
	fp =fopen("/tmp/mnt/data_persist/dev/bin/tpms.out", "r");
	if (fp != NULL)
	{
		if (fgets(fbuf, 107, fp) == NULL) sid=0;
		fclose(fp);
	}

	/* Initialize libusb
     */

    rc = libusb_init(NULL);
    if (rc < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
        exit(1);
    }

    /* Set debugging output to max level.
     */
    libusb_set_debug(NULL, 3);

    /* Look for a specific device and open it.
     */
    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!devh) {
        fprintf(stderr, "Error finding USB device\n");
        return -1;
    }

    /* As we are dealing with a CDC-ACM device, it's highly probable that
     * Linux already attached the cdc-acm driver to this device.
     * We need to detach the drivers from all the USB interfaces. The CDC-ACM
     * Class defines two interfaces: the Control interface and the
     * Data interface.
     */
    for (int if_num = 0; if_num < 2; if_num++) {
        if (libusb_kernel_driver_active(devh, if_num)) {
            libusb_detach_kernel_driver(devh, if_num);
        }
        rc = libusb_claim_interface(devh, if_num);
        if (rc < 0) {
            fprintf(stderr, "Error claiming interface: %s\n",
                    libusb_error_name(rc));
	return -1;
        }
    }

    /* Start configuring the device:
     * - set line state
     */
    rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS,
                                0, NULL, 0, 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }

    /* - set line encoding: here 9600 8N1
     * 9600 = 0x2580 ~> 0x80, 0x25 in little endian
     */


    unsigned char encoding[] = { 0x00, 0xc2, 0x01, 0x00, 0x00, 0x00, 0x08 };
    rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding,
                                sizeof(encoding), 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }

    /* We can now start sending or receiving data to the device
     */
	 
	 
	/* Read the results of the previous BLE command if there is any */

    data_len = read_chars(buf, 64);

    if( data_len == -1)
	{

		/* No data ? thus initialize the board for discovery mode */

		ble_init();
		ble_start_discovery();
		data_len = read_chars(buf, 64);

		/* Just and indicator that the last read was not a timeout, set to -1 if timeout */
		
		data_len = -2;
	} 

	/* re adjust buffer because the codes below was based on the arduino code offset values (to fix later) */
	for(i=0;i< data_len;i++)
		buf[i] = buf[i+3]; 

	event = buf[1]*256 + buf[0];
 
    /* just block the switch case if there is no read data - too lazy to put if statements :D (to fix later)  */
 
	if ((data_len == -1) ||  (data_len == -2))  event = -1;
 
	switch (event)
	{
     case 0x060d: // Process Advertisement Data

		if (data_len > 14)
		{
			// Validate Event Code
			if ((buf[21] == 0xff) && (buf[8] == 0xca) && (buf[9] == 0xea) && ((buf[10] & 0xf0) == 0x80) && (buf[26] == 0xca) && (buf[25] == 0xea))   // Validate BLE MAC
            { 
            
            
                sid =  buf[24] & 0x03;    // Get Sensor ID
                //pressure =    double (( (double)buf[30] +  (double)(buf[31] * 256 ) + (double)(buf[32] * 65536) +   (double)(buf[33] *16277216)) / (double)100000.00) * 14.5;
                
                tmpval = 0;
                tmpval2 = 0;
                
                memcpy(&tmpval, buf+30, 4);
                memcpy(&tmpval2, buf+34, 4);
                
                pressure =    (double) ((tmpval / 100000.00) * 14.5);
                temperature = (double) (tmpval2 / 100.00);

                smac = buf[29] +  (buf[28] * 256) +  (buf[27] * 65536);
                
                /* Block output if ID temp and pressure data is invalid */
				
                if ((temperature > 100) || (pressure > 200)) smac = -1;
                
                /* To fix : remove hardcoded file locations. Use possibly separate files per senoro ? */
				
                switch (smac)
                {
                    case SMAC1:
                                     
                        sprintf(tbuf,"S[%d]:FL: %.2fC %.2fpsi ;",sid,temperature,pressure);
						memcpy(&fbuf[0],tbuf,25);
						fp =fopen("/tmp/mnt/data_persist/dev/bin/tpms.out", "w");
						fbuf[104] = 0;
						fprintf(fp,"%s\n",fbuf);
						fclose(fp);
                        break;

                    case SMAC2:
					
						sprintf(tbuf,"S[%d]:FR: %.2fC %.2fpsi ;",sid,temperature,pressure);
						memcpy(&fbuf[26],tbuf,25);
                        fp =fopen("/tmp/mnt/data_persist/dev/bin/tpms.out", "w");
                        fbuf[104] = 0;
                        fprintf(fp,"%s\n",fbuf);
                        fclose(fp);
                        break;

                    case SMAC3:
					
						sprintf(tbuf,"S[%d]:RL: %.2fC %.2fpsi ;",sid,temperature,pressure);
						memcpy(&fbuf[52],tbuf,25);
                        fp =fopen("/tmp/mnt/data_persist/dev/bin/tpms.out", "w");
                        fbuf[104] = 0;
                        fprintf(fp,"%s\n",fbuf);
                        fclose(fp);
                        break;

					case SMAC4:
						
						sprintf(tbuf,"S[%d]:RR: %.2fC %.2fpsi ;",sid,temperature,pressure);
						memcpy(&fbuf[78],tbuf,25);
                        fp =fopen("/tmp/mnt/data_persist/dev/bin/tpms.out", "w");
                        fbuf[104] = 0;
						fprintf(fp,"%s\n",fbuf);
						fclose(fp);
						break;
                                     
                  default:
				  
                        break;
                  
                }
			}
			else
              break;
           
		}
		break;

	  case 0x0601:   // Board has completed the Advertisement SCAN, restart it
      
		ble_start_discovery();
		data_len = read_chars(buf, 64);  // Must read the command status before exiting
		break;
	  
      default:
	
		break;
	}  

	if (data_len == -1)  // Board Timeout ? Re initialize it. (TO fix: Board does not need re-initialization in case timeout due to no advertisement receieved) 
	{

		ble_init();
		ble_start_discovery();
		data_len = read_chars(buf, 64);
	}
    

    libusb_release_interface(devh, 0);

    if (devh)
            libusb_close(devh);
    libusb_exit(NULL);

    return rc;
}
