# USBGET

USBGET ist ein Werkzeug zum Auslesen und Einstellen der USBUNIT.

[Kompilieren](#kompilieren)<br>
[USBGET aufrufen](#usbget-aufrufen)<br>
[USBGET Hilfe](#hilfe-anzeigen)<br>
[USB Devices anzeigen](#usb-devices-anzeigen)<br>
[USBUNIT Statistiken](#interne-informationen-und-statistiken-der-usbunit-anzeigen)<br>
[USBUNIT Verfügbare Module](#module-der-usbunit-anzeigen)<br>
[USBUNIT Moduldaten abfragen](#moduldaten-abfragen)<br>
[USBUNIT Modulkonfiguration](#modulkonfiguration-auslesen-und-einstellen)<br>

Konfigurationsdetails für die einzelnen Module: [MODULE](doc/module.md)

## Kompilieren

[Inhalt](#usbget)<br>

### Voraussetzungen zum kompilieren (nur unter Linux*):
a) install build-essentials

b) m3-toolchain/ vom root-Verzeichnis aus clonen:
https://github.com/TTigges/m3-toolchain
(forked von jmgao/m3-toolchain)

```
usbget
|-cmu/                 Zielorder für usbget (CMU Version)
|-doc/                 Documentation
|-local/               Zielordner für usbget (Linux local Version)
|-m3-toolchain/        Compiler toolchain (CMU)
|-src/                 Quelldateien
|-test/                Testing
|-Makefile
|-Readme.md
```

Zum Übersetzen "make" im Verzeichnis usbget aufrufen.

Der Ordner cmu enthält danach die ausführbare Datei für die CMU.
Der Ordner local enthält danach die auführbare Datei die auf dem lokalen Linux läuft.

Getestet mit Ubuntu 18.04.2, Debian 9.9, Debian 10.4

## USBGET aufrufen

[Inhalt](#usbget)<br>

USBGET verbindet sich mit der USBUNIT über den USB Bus. Dazu kann mit der Option -d das gewünschte USB device ausgewählt werden.

Vor dem Aufruf muss folgendes Verzeichnis erstellt werden:

```
mkdir -p /tmp/mnt/data_persist/dev/bin
```

Beispiel: usbget -d arduino_nano -i

Wird UBSGET ohne -d Option aufgerufen wird das erste kompatible USB Device benutzt. 

Bei einen Fehler wird eine log Datei in /tmp/mnt/data_persist/dev/bin erzeugt.

### Hilfe anzeigen

[Inhalt](#usbget)<br>

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

### USB Devices anzeigen

[Inhalt](#usbget)<br>

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

USB devices auf die nur root Zugriff hat sind mit "- no access -" markiert.

### Interne Informationen und Statistiken der USBUNIT anzeigen

[Inhalt](#usbget)<br>

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

### Module der USBUNIT anzeigen 

[Inhalt](#usbget)<br>

Syntax: usbget [-d device] [-v] -l

```
$ usbget  -l
DISP
TPMS
OIL
RGB
```

### Moduldaten abfragen

[Inhalt](#usbget)<br>

Syntax: usbget [-d device] [-v] -q modul

```
$ usbget  -q TPMS
$ cat /tmp/mnt/data_persist/dev/bin/tpms.out 
0: 19.0 2.19 1: 3.0 1.99 2: 20.0 2.01 3: 19.0 1.90
```

Die Daten werden in einer Datei im Ordner /tmp/mnt/data_persist/dev/bin abgelegt.

### Modulkonfiguration auslesen und einstellen.

[Inhalt](#usbget)<br>

Auslesen:<br>
Syntax: usbget [-d device] [-v] -c module

Einstellen:<br>
Syntax: usbget [-d device] [-v] -s module -p "key=value" -p "key=value" ...<br>
Syntax: usbget [-d device] [-v] -s module -p "key=value[;key=value...]"

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


