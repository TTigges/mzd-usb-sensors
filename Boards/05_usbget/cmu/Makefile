#
# Makefile for building usbget for the CMU target (ARM cortex 9)
#

CC=../m3-toolchain/bin/arm-cortexa9_neon-linux-gnueabi-g++

SYSROOT= ../m3-toolchain/arm-cortexa9_neon-linux-gnueabi/sysroot

CFLAGS= --sysroot=$(SYSROOT) -Wall -MD -g -DCMU=1 -D__STDC_FORMAT_MACROS -march=armv7-a -mtune=cortex-a9 -mfpu=neon -std=c++11 -D_GLIBCXX_USE_C99 -DDBUS_API_SUBJECT_TO_CHANGE -I$(SYSROOT)/usr/include/libusb-1.0 -I$(SYSROOT)/usr/include/glib-2.0 -I$(SYSROOT)/usr/lib/glib-2.0/include -I../src

LDFLAGS= --sysroot=$(SYSROOT) -rdynamic -pthread -ldl -static-libstdc++ -lusb-1.0

####

TARGET= usbget
MODULES= support.o usb.o ftdi.o protocol.o usbget.o

all: $(TARGET)

clean:
	rm -f *.o
	rm -f *.d
	rm -f *~
	rm -f $(TARGET)

path:
	mkdir -p /tmp/mnt/data_persist/dev/bin

$(TARGET): $(MODULES)
	$(CC) $(LDFLAGS) -o $(TARGET) $(MODULES)

protocol.o: ../src/protocol.c ../src/support.h ../src/protocol.h
	$(CC) $(CFLAGS) -c ../src/protocol.c

support.o: ../src/support.c ../src/support.h
	$(CC) $(CFLAGS) -c ../src/support.c

usbget.o: ../src/usbget.c ../src/support.h ../src/usb.h ../src/protocol.h
	$(CC) $(CFLAGS) -c ../src/usbget.c

usb.o: ../src/usb.c ../src/support.h ../src/usb.h ../src/ftdi.h
	$(CC) $(CFLAGS) -c ../src/usb.c

ftdi.o: ../src/ftdi.c ../src/support.h ../src/ftdi.h
	$(CC) $(CFLAGS) -c ../src/ftdi.c
