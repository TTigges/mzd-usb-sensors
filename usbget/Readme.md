# USBGET

USBGET is a command line tool to query and configure the USBUNIT.

[Compile](#compile)<br>
[Using USBGET](#using-usbget)<br>
[USBGET Help](#display-help)<br>
[List USB devices](#list-usb-devices)<br>
[USBUNIT statistics](#internal-information-and-statistics)<br>
[List supported USBUNIT modules](#list-supported-usbunit-modules)<br>
[Query module data](#query-module-data)<br>
[Configure a USBUNIT module](#configure-a-usbunit-module)<br>

Details of supported modules can be found here: [MODULES](doc/module.md)

## Compile

[Index](#usbget)<br>

### Preconditions to compile USBGET on Linux:
a) install build-essentials<br>
b) clone m3-toolchain/ from root directory:
https://github.com/TTigges/m3-toolchain
(forke of jmgao/m3-toolchain)

```
usbget
|-cmu/                 Target folder of the usbget executable (CMU version)
|-doc/                 Documentation
|-local/               Target folder of the usbget executable (Linux local version)
|-m3-toolchain/        Compiler toolchain (CMU)
|-src/                 USBGET source
|-test/                Testing folder
|-Makefile
|-Readme.md
```

To compile USBGET run "make" in the usbget folder. This compiles both, the CMU and the local version.

Folder cmu contains the executable for the CMU.
The local folder contains the executable runnable on the local Linux box.

This has been testet on Ubuntu 18.04.2, Debian 9.9, Debian 10.4

## Using USBGET

[Index](#usbget)<br>

USBGET connects to the USBUNIT via the USB bus. Use option -d to select the correct USB device.

Before running USBGET the following directory has to be created.

```
mkdir -p /tmp/mnt/data_persist/dev/bin
```

Example: usbget -d arduino_nano -i

Running UBSGET without the -d option automatically selects the first compatible device.

In case of an error more information can be found in the log file created in /tmp/mnt/data_persist/dev/bin.

### Display help

[Index](#usbget)<br>

Syntax: usbget -?

```
$ usbget -?

usbget 0.2.0

 usage: usbget [options] [command ... ] 

   Options:
     -d device_name             USB device type
                                If -d option is omitted search
                                for suitable device.
        Valid device names:
          redbear_duo
          arduino_pro
          arduino_nano
          arduino_nano_clone
          arduino_micro
     -v                         Enable debug output
     -?                         Print usage

   Commands:
     -u                         List USB devices
     -i                         Query device infos
     -l                         List supported actions
     -q action [-p param ... ]  Query action
     -s action [-p param ... ]  Set action
     -c action                  Query action config

   In case no command is specified all supported actions are queried.
```

### List USB devices

[Index](#usbget)<br>

Syntax: usbget [-d device] [-v] -u

```
$ usbget -u
VId=8087 PId=8000 [- no access -]  
VId=1d6b PId=0002 [- no access -]  
VId=1a86 PId=7523 [] USB2.0-Serial arduino_nano_clone
VId=1d6b PId=0002 [- no access -]  
VId=1d6b PId=0003 [- no access -]  
VId=1d6b PId=0002 [- no access -]  
```

To access some USB devices root permissions are required. Those devices are marked "- no access -".

### Internal information and statistics

[Index](#usbget)<br>

Syntax: usbget [-d device] [-v] -i

```
$ usbget  -i
version     = 0.2.2
simulate    = false
cs intr.    = 3831215
data intr.  = 19879891
max carr us = 9976
carr detect = 1669001
data avail. = 191
max timings = 148
bit errors  = 2598
preamble ok = 184
cksum ok    = 100
cksum fails = 84
memory      = 176/341
```

### List supported USBUNIT modules

[Index](#usbget)<br>

Syntax: usbget [-d device] [-v] -l

```
$ usbget  -l
DISP
TPMS
OIL
RGB
```

Details of supported modules can be found here: [MODULES](doc/module.md)

### Query module data

[Index](#usbget)<br>

Syntax: usbget [-d device] [-v] -q modul

```
$ usbget  -q TPMS
$ cat /tmp/mnt/data_persist/dev/bin/tpms.out 
0: 19.0 2.19 1: 3.0 1.99 2: 20.0 2.01 3: 19.0 1.90
```

The queried data is stored in a file in the /tmp/mnt/data_persist/dev/bin folder.

### Configure a USBUNIT module

[Index](#usbget)<br>

Query configuration:<br>
Syntax: usbget [-d device] [-v] -c module

Set configuration:<br>
Syntax: usbget [-d device] [-v] -s module -p "key=value" -p "key=value" ...<br>
Alternative syntax: usbget [-d device] [-v] -s module -p "key=value[;key=value...]"

```
$ usbget -c TPMS
0 ID=00000000 T=0.0 P=0.00 S=0
1 ID=00000000 T=0.0 P=0.00 S=0
2 ID=00000000 T=0.0 P=0.00 S=0
3 ID=00000000 T=0.0 P=0.00 S=0
...
$ usbget -s TPMS -p "0=aba12400;1=aba12401"
$ usbget -c TPMS
0 ID=aba12400 T=0.0 P=0.00 S=0
1 ID=aba12401 T=0.0 P=0.00 S=0
2 ID=00000000 T=0.0 P=0.00 S=0
3 ID=00000000 T=0.0 P=0.00 S=0
```


