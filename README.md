# mzd-usb-sensors

Enables additional custom apps on MZD CMU to receive data from external sensors via usb connected microcontrollers.

This README is not final.


### Files structure:

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

**To be finished.**


### How it works:

The Speedometer App triggers a shell script (speedometer.sh) that reads data and provides these for the app. We modified the script to run an executable: usbget

[**_usbget_**](usbget/Readme.md) establishes a serial connection via usb to the connected microcontroller board. usbget can query data but also send data. For more information, see the readme for usbget.

**_usbunit_** is the board side code that allows the microcontroller to handle the queries. The 433 MHz receiver and oil sensors are connected to the microcontroller board. The RedBear Duo (OOP*) is also supported, which has BLE on board and allowed to receive BLE sensors (valve cover type).

[**_modules_**](usbget/doc/module.md) are pieces of code runnung on the USBUNIT to provide that actual funcionality.
There are modules available to access TPMS data as well as oil pressure and temperature.


### Schematic:

```
CMU                     Microcontroller via USB
│                       │
└─ App ──── [usbget] ─ [usbunit]
   │         │          │  (optional:)
   *.out-files          ├─ CC1101 433 MHz receiver ─) ) )    ( ( (─ (OEM) 433 MHz TPMS sensors
                        ├─ onboard BLE receiver¹ ───) ) )    ( ( (─ BLE TPMS sensors²
                        └─ wire connected sensors ───────────────── oil pressure or temperature sensors³

```

¹ RedBear Duo (OOP*) is supported, for other boards or BLE receivers the code needs to be changed and adapted in order to be supported.  
² depending on the BLE protocol and the decoding of the signal, the necessary code in usbunit needs to be changed and adapted.  
³ depending on the sensor, the calculation of the incoming signal needs to be changed and adapted.


### Supported Oil Sensors:

- Pressure sensor:    Prosport PSSMOPS-PK
- Temperature sensor: Prosport PSOWTS-JPNWP


### \* BLE – or: If your car does not have TPMS sensors but you want TPMS data:

We started with BLE sensors (valve cover type) and decoded the received signals. Depending on the sensors you get, you might have to entirely start from scratch, decoding the signals. We also had issues receiving all four sensors, even with an external antenna installed.

The supported RedBear Duo board is also unfortunately out of production. I have a small batch available, if anyone wants to try it out.

For now, we suggest to install TPMS sensors (valve stem type), VDO Type TG1C or compatible 3rd party sensors, even if your car does not have/need active TPMS sensors.


### 433 MHz vs 315 MHz, Europa/ROTW vs US:

In Europe and the rest of the world, 433 MHz sensors are used for active TPMS while in the USA, 315 MHz is used. For now, mzd-usb-sensors supports 433 MHz signals via the CC1101 receiver. Supporting 315 MHz sensors is something we might want to look into => **TBD**


## Thanks:

Wolfix for coming up with the main protocol when he heard that I wanted the CMU to communicate with a usb connected microcontroller, for the support and all the updates and ideas! I can't thank you enough!

jayrockk for supporting the idea from the beginning, for all the help with receiving the 433 MHz signals and for the work on the PCBs.

Mazdaracerdude from mazda3revolution.com for setting up an example based on the m3-toolchain (https://github.com/jmgao/m3-toolchain) to receive BLE sensors with a RedBear BLE microcontroller. https://www.mazda3revolution.com/threads/reading-bluetoothtpms-tire-pressure-monitoring-sensor-data-to-your-cmu-infotainment.224882/

merbanan and zuckerschwerdt from https://github.com/merbanan/rtl_433 for their data receiver and device protocol list for the RTL-SDR to get started receiving the actual 433 MHz signals and their help to understanding and decoding the messages.

JSMSolns on hackster.io for setting up an example on how to use the CC1101 to receive 433 signals with an Arduino based microcontroller.
https://github.com/JSMSolns/TPMS_Toyota_UK_Decoder, https://www.hackster.io/jsmsolns/arduino-toyota-uk-tpms-tyre-pressure-display-b6e544

Trezdog44 (Trevelopment) and flyandi from mazda3revolution and mazdatweaks.com for stimulating the idea to build a custom app and providing examples and the CASDK.

Herko ter Horst, the developer of the Speedometer-App for MZD which I used and updated to display the received signals.