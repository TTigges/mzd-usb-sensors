
CC=./m3-toolchain/bin/arm-cortexa9_neon-linux-gnueabi-g++

SYSROOT= ./m3-toolchain/arm-cortexa9_neon-linux-gnueabi/sysroot

CFLAGS= --sysroot=$(SYSROOT) -Wall -MD -O3 -DCMU=1 -D__STDC_FORMAT_MACROS -march=armv7-a -mtune=cortex-a9 -mfpu=neon -std=c++11 -D_GLIBCXX_USE_C99 -DDBUS_API_SUBJECT_TO_CHANGE -I$(SYSROOT)/usr/include/libusb-1.0 -I$(SYSROOT)/usr/include/glib-2.0 -I$(SYSROOT)/usr/lib/glib-2.0/include

LDFLAGS= --sysroot=$(SYSROOT) -rdynamic -pthread -ldl -static-libstdc++ -lusb-1.0

TARGET= usb_rbduo_get

all: $(TARGET)

clean:
	rm -f $(TARGET).o
	rm -f $(TARGET)

$(TARGET): $(TARGET).c

cp_source:
	cp local/$(TARGET).c .

