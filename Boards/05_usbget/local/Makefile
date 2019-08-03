#
# Makefile for building usbget for the local environment.
#

CC= cc

CFLAGS= -Wall -I/usr/include/libusb-1.0 -I../src

LDFLAGS= -lusb-1.0

####

TARGET= usbget
MODULES= support.o usb.o ftdi.o atmega32u4.o protocol.o usbget.o

all: $(TARGET)

clean:
	rm -f *.o
	rm -f *.d
	rm -f *~
	rm -f $(TARGET)

path:
	mkdir -p /tmp/mnt/data_persist/dev/bin

$(TARGET): $(MODULES)
	$(CC) -o $(TARGET) $(MODULES) $(LDFLAGS)

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

atmega32u4.o: ../src/atmega32u4.c ../src/support.h ../src/atmega32u4.h
	$(CC) $(CFLAGS) -c ../src/atmega32u4.c
