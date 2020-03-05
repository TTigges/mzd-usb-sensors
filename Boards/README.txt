usbget
======

- Unterstützt jetzt so ziemlich alle Arduino boards.
- Die option -d <boardname> kann man weg lassen. Es wird dann das erste
  Arduino board genommen das am USB port gefunden wird.

usbget kennt folgende Befehle:
==============================

List USB devices
----------------
usbget -u

wlohwass@wolfix:$ usbget -u
VId=8087 PId=8000 [- no access -]
VId=1d6b PId=0002 [- no access -]
VId=8087 PId=8008 [- no access -]
VId=1d6b PId=0002 [- no access -]
VId=1d6b PId=0003 [- no access -]
VId=0403 PId=6001 [FTDI] FT232R USB UART arduino_pro
VId=0557 PId=2213 [- no access -]
VId=0557 PId=8021 [- no access -]
VId=03eb PId=2104 [ATMEL] AVRISP mkII
VId=046d PId=082d [- no access -]
VId=04b8 PId=089b [EPSON] EPSON XP-800 Series
VId=1d6b PId=0002 [- no access -]

List modules
------------
usbget -l

wlohwass@wolfix:$ usbget -l
TPMS
OIL
RGB

Info command
------------
usbget -i

wlohwass@wolfix:$ usbget -i
version     = 0.1.5
simulate    = false
cs intr.    = 5382477
data intr.  = 14635574
max carr us = 10524
carr detect = 2348578
data avail. = 7543
max timings = 255
preamble ok = 5074
cksum ok    = 423
cksum fails = 4651
memory      = 224/394          <<<< Freier Speicher in bytes (aktuell / beim_start)

Modul configuration setzen
--------------------------
usbget -s <modul> -p "key=value" ...
usbget -s <modul> -p "key=value;key=value..."

Für das RGB Modul setzt man die Farbwerte.
Für das TPMS Modul kann man seine Sensor IDs setzen.

usbget -s RGB -p "R=100" -p "G=50" -p "B=50"
usbget -s TPMS -p "0=11223344" -p "1=22334455" -p "2=33445566" -p "3=44556677"

oder zusamengefasst

usbget -s RGB -p "R=100;G=50;B=50"
usbget -s TPMS -p "0=11223344;1=22334455;2=33445566;3=44556677"

Konfiguration abfragen
----------------------
usbget -c <module>

Für das RGB Modul sieht man die aktuellen Farbwerte:
wlohwass@wolfix:$ usbget -c RGB
R=100 G=50 B=50

Für das TPMS Modul alle sensor informationen.
wlohwass@wolfix:$ usbget -c TPMS
0 ID=aba12403 T=9.0 P=1.89 S=35
1 ID=aba12401 T=2.0 P=2.03 S=34
2 ID=aba12402 T=16.0 P=2.13 S=34
3 ID=aba12400 T=14.0 P=1.95 S=27
4 ID=aba1240c T=7.0 P=2.10 S=15
5 ID=aba12409 T=3.0 P=1.92 S=10
6 ID=aba12406 T=10.0 P=2.01 S=10
7 ID=aba1240b T=17.0 P=2.08 S=9
8 ID=aba1240e T=25.0 P=2.25 S=7
9 ID=00000000 T=0.0 P=0.0 S=0
10 ID=00000000 T=0.0 P=0.0 S=0
11 ID=00000000 T=0.0 P=0.00 S=0

T=Temperatur P=Pressure S=Score

Score wird wie folgt berechnet:
-------------------------------
Wird ein Datensatz für einen Sensor empfangen, Kriegt den Sensor einen Bonus
von 10 Punkten. Alle 20 sek. wird jedem Sensor ein Punkt abgezogen.
Die Sensoren sind nach Punktekonto sortiert.

WICHTIG:
========
Im Arduino code gibts 12 Slots für Sensoren.
Wenn mit usbget -s TPMS -p "0=xxxxxxxx" IDs gesetzt werden, dann werden die ersten 4 Slots für diese
Sensor IDs reserviert. Wenn das nicht gemacht wird, oder ALLE sensor IDs geloscht werden.

( Mit usbget -s TPMS -p "0=00000000;1=00000000;2=00000000;3=00000000" )

dann werden alle 12 slots gleich behandelt und nach Punktestand sortiert.

Das Display wird auch unterstützt. Es werden die Werte der Sensor Slots 0-4 angezeigt.

Werte abfragen
==============
usbget -q <module>

oder um alle Module abzufragen einfach:

usbgetwlohwass@wolfix:$ usbget
wlohwass@wolfix:$ cat /tmp/mnt/data_persist/dev/bin/tpms.out
ID=aba12403 T=18.0 P=2.03
ID=aba12402 T=6.0 P=2.19
ID=aba12401 T=5.0 P=1.95
ID=aba12400 T=13.0 P=2.25
wlohwass@wolfix:$ cat /tmp/mnt/data_persist/dev/bin/oil.out
oiltemp: 142.7 oilpress: -0.40
