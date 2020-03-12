
mkdir -f /tmp/mnt/data_persist/dev/bin

while [ true ]; do
    sudo ./usbget
    if [ -f /tmp/mnt/data_persist/dev/bin/usbget.log ]; then
        exit
    fi
done
