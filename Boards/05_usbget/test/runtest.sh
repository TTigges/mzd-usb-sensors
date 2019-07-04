#!/bin/bash
# 
# Requires a working Redbear Duo controller connected to the PC.
#

P=/tmp/mnt/data_persist/dev/bin

mkdir -p $P
rm -f test.log

echo --- usage --- >test.log
sudo ../local/usbget -? >>test.log 2>&1

echo --- arduino_nano list --- >>test.log
sudo ../local/usbget -d arduino_nano -l >>test.log 2>&1

echo --- arduino_pro list --- >>test.log
sudo ../local/usbget -d arduino_pro -l >>test.log 2>&1

echo --- redbear_duo list --- >>test.log
sudo ../local/usbget -d redbear_duo -l >>test.log 2>&1

echo --- redbear_duo info --- >>test.log
sudo ../local/usbget -d redbear_duo -i ALL >>test.log 2>&1

echo --- redbear_duo config --- >>test.log
sudo ../local/usbget -d redbear_duo -c TPMS >>test.log 2>&1
sudo ../local/usbget -d redbear_duo -c OIL >>test.log 2>&1
sudo ../local/usbget -d redbear_duo -c WS2801 >>test.log 2>&1

echo --- redbear_duo query --- >>test.log
sudo rm -f $P/tpms.out
sudo ../local/usbget -d redbear_duo -q TPMS >>test.log 2>&1
cat $P/tpms.out >>test.log 2>&1

sudo rm -f $P/oil.out
sudo ../local/usbget -d redbear_duo -q OIL >>test.log 2>&1
cat $P/oil.out >>test.log 2>&1

sudo rm -f $P/ws2801.out
sudo ../local/usbget -d redbear_duo -q WS2801 >>test.log 2>&1
cat $P/ws2801.out >>test.log 2>&1

echo --- redbear_duo set --- >>test.log
sudo ../local/usbget -d redbear_duo -s WS2801 -p R=10 -p G=10 -p B=10 >>test.log 2>&1

# ====== negative tests
echo --- negative tests --- >>test.log

echo --- wrong device --- >>test.log
sudo ../local/usbget -d arduino_bla -l >>test.log 2>&1

echo --- wrong action --- >>test.log
sudo ../local/usbget -d redbear_duo -q BLABLA >>test.log 2>&1

echo --- long device name --- >>test.log
sudo ../local/usbget -d redbear_duo_redbear_duo -q BLABLA >>test.log 2>&1

echo --- long action name --- >>test.log
sudo ../local/usbget -d redbear_duo -q BLABLA7890BLABLA7890BLABLA7890 -v >>test.log 2>&1

diff test.log test.log.ref
