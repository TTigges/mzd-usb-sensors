# mzd-usb-sensors

Enables additional custom apps on MZD CMU to poll data from external sensors via usb connected microcontrollers.

This README is not final.

## Files

**TBD**

```
mzd-usb-sensors
│
├─── usbget
│    ├── …
│    └── …
│
└─── usbunit
     ├── …
     └── …
```

## How it works:

**TBD**

```
CMU                  Microcontroller via USB
│                    │
└─ App ─ [usbget] ─ [usbunit]
   │      │          │  (optional:)
   *.out-files       ├─ CC1101 433 MHz receiver     ─ (OEM) 433 MHz TPMS sensors
                     ├─ onboard BLE receiver¹       ─ BLE TPMS sensors²
                     └─ wire connected sensors ────── oil pressure or temperature sensors³


¹ RedBear Duo (OOP) is supported, for other boards or BLE receivers the code needs to be changed and adapted in order to be supported.
² depending on the BLE protocol and the decoding of the signal, the necessary code in usbunit needs to be changed and adapted.
³ depending on the sensor, the calculation of the incoming signal needs to be changed and adapted.
```

**TBD**


#### Thanks:

Wolfix for comming up with the main protocol when he heard that I wanted the CMU to communicate with a usb connected microcontroller, for the support and all the updates and ideas! I can't thank you enough!

jayrockk for supporting the idea from the beginning, for all the help with receiving the 433 MHz signals and for the work on the PCBs.

Mazdaracerdude from mazda3revolution.com for setting up an example based on the m3-toolchain (https://github.com/jmgao/m3-toolchain) to receive BLE sensors with a RedBear BLE microcontroller. https://www.mazda3revolution.com/threads/reading-bluetoothtpms-tire-pressure-monitoring-sensor-data-to-your-cmu-infotainment.224882/

merbanan and zuckerschwerdt from https://github.com/merbanan/rtl_433 for their data receiver and device protocol list for the RTL-SDR to get started receiving the actual 433 MHz signals and their help to understanding and decoding the messages.

JSMSolns on hackster.io for setting up an example on how to use the CC1101 to receive 433 signals with an Arduino based microcontroller.
https://github.com/JSMSolns/TPMS_Toyota_UK_Decoder, https://www.hackster.io/jsmsolns/arduino-toyota-uk-tpms-tyre-pressure-display-b6e544

Trezdog44 (Trevelopment) and flyandi from mazda3revolution and mazdatweaks.com for stimulating the idea to build a custom app and providing examples and the CASDK.

Herko ter Horst, the developer of the Speedometer-App for MZD which I used and updated to display the received signals.