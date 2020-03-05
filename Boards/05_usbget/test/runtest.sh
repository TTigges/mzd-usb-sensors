#!/bin/bash
# 
# Requires a working Redbear Duo controller connected to the PC.
#

#DEV=redbear_duo
DEV=arduino_nano_clone

P=/tmp/mnt/data_persist/dev/bin

mkdir -p $P
rm -f test.log

echo --- usage --- >test.log
../local/usbget -? >>test.log 2>&1

echo --- arduino_nano list --- >>test.log
../local/usbget -d arduino_nano -l >>test.log 2>&1

echo --- arduino_pro list --- >>test.log
../local/usbget -d arduino_pro -l >>test.log 2>&1

echo --- redbear_duo list --- >>test.log
../local/usbget -d $DEV -l >>test.log 2>&1

echo --- info --- >>test.log
../local/usbget -d $DEV -i >>test.log 2>&1

echo --- config --- >>test.log
../local/usbget -d $DEV -c TPMS >>test.log 2>&1
../local/usbget -d $DEV -c OIL >>test.log 2>&1
../local/usbget -d $DEV -c WS2801 >>test.log 2>&1

echo --- query --- >>test.log
rm -f $P/tpms.out
../local/usbget -d $DEV -q TPMS >>test.log 2>&1
cat $P/tpms.out >>test.log 2>&1

rm -f $P/oil.out
../local/usbget -d $DEV -q OIL >>test.log 2>&1
cat $P/oil.out >>test.log 2>&1

rm -f $P/ws2801.out
../local/usbget -d $DEV -q WS2801 >>test.log 2>&1

echo --- query multiple --- >>test.log
rm -f $P/tpms.out $P/oil.out
../local/usbget -d $DEV -q TPMS -q OIL >>test.log 2>&1
cat $P/tpms.out >>test.log 2>&1
cat $P/oil.out >>test.log 2>&1

echo --- set --- >>test.log
../local/usbget -d $DEV -s RGB -p R=10 -p G=10 -p B=10 >>test.log 2>&1

echo --- set max parameters --- >>test.log
../local/usbget -d $DEV -s BLA -p 1 -p 2 -p 3 -p 4 -p 5 -p 6 -p 7 -p 8 -p 9 -p 10 >>test.log 2>&1

# ====== negative tests
echo --- negative tests --- >>test.log

echo --- wrong device --- >>test.log
../local/usbget -d arduino_bla -l >>test.log 2>&1

echo --- wrong action --- >>test.log
../local/usbget -d $DEV -q BLABLA >>test.log 2>&1

echo --- to many parameters --- >>test.log
../local/usbget -d $DEV -s BLA -p 1 -p 2 -p 3 -p 4 -p 5 -p 6 -p 7 -p 8 -p -9 -p 10 -p 11 >>test.log 2>&1

echo --- long device name --- >>test.log
../local/usbget -d redbear_duo_redbear_duo -q BLABLA >>test.log 2>&1

echo --- long action name --- >>test.log
../local/usbget -d $DEV -q BLABLA7890BLABLA7890BLABLA7890 -v >>test.log 2>&1

echo --- long parameter key --- >>test.log
../local/usbget -d $DEV -s WS2801 -p "123456789=123" >>test.log 2>&1

./rxdiff.pl test.log.ref test.log

if [ $? == 0 ]; then
    echo OK
else
    echo DIFFS
fi

