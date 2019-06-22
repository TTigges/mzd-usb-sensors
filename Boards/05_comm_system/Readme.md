Open terminal in this folder.

Get the toolchain (see requirements):
    `git clone https://github.com/jmgao/m3-toolchain.git`

(Rename the folder to 'm3-toolchain' tbd.: check if this is still necessary)

Install make:
    `sudo apt-get make` or `sudo apt-get install build-essential`

(Delete/Backup existing executable 'usb_rbduo_get'.)

Compile the new executable with:
    `make`

Big thx to wolfix for the Makefile!

#Folder Structure should look like:#

05_comm_system
  ├ m3-toolchain/
  | ├ arm-cortexa9_neon-linux-gnueabi/
  | └ Readme.md
  ├ Makefile
  ├ Readme.md
  ├ usb_rbduo_get //.bak?
  └ usb_rbduo_get.c

#Vendor and Product IDs:#

##RedBear Duo##
`static const uint16_t VENDOR_ID = 0x2B04;`
`static const uint16_t PRODUCT_ID = 0xC058;`