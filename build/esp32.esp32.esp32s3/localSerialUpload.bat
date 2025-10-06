echo "This bat file will download the essential files to program the MarineMultiDisplay Hardware. You will first need to Plug the display into a COM port and enter the Com port for the device"
set /p NUM=Enter Com port number:

echo " Starting programmer"


esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 "MarineMultiDisplay.ino.bootloader.bin" 0x8000 "MarineMultiDisplay.ino.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "MarineMultiDisplay.ino.bin" 



//esptool.exe --chip esp32s3 --port COM%NUM% --baud 115200  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 MarineMultiDisplay.ino.bootloader.bin 0x8000 MarineMultiDisplay.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 MarineMultiDisplay.ino.bin
