
while [ true ]; do
    sudo ./usb_rbduo_get
    if [ -f /tmp/mnt/data_persist/dev/bin/usb_rbduo_get_log.out ]; then
        exit
    fi
done
