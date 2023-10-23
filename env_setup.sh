. $HOME/Packages/esp-idf/export.sh

echo idf.py menuconfig
echo idf.py build
echo idf.py -p /dev/ttyUSB0 flash
echo idf.py -p /dev/ttyACM0 flash

alias do_flash='killall picocom; idf.py -p /dev/ttyUSB0 flash'
## alias do_flash='killall picocom; idf.py -p /dev/ttyACM0 flash'
