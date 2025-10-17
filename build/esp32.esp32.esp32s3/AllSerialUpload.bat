echo "This bat file will download the essential files to program the MarineMultiDisplay Hardware."
echo "  You will first need to Plug the display into a COM port and enter the Com port for the device"
set /p NUM=Enter Com port number:
echo  "The program will now make local copies of the Binaries needed to program the board"
echo "  1 for GUITRON display"  
echo "  2 for WAVshare module"
echo "  3 for Wide Waveshare '4.3' board"
echo " 4 for the MarineMultiDisplay.ino"
set /p choice=Enter 1 2 3or 4:

echo " Starting programmer"

if "%choice%"=="1" (
esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 "MarineMultiDisplay.ino.bootloader.bin" 0x8000 "MarineMultiDisplay.ino.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "GUITMMD.ino.bin" 
)

if "%choice%"=="3" (
esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 "MarineMultiDisplay.ino.bootloader.bin" 0x8000 "MarineMultiDisplay.ino.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "WAVWIDEMMD.ino.bin" 
)
if "%choice%"=="2" (
esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 "MarineMultiDisplay.ino.bootloader.bin" 0x8000 "MarineMultiDisplay.ino.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "WAVMMD.ino.bin" 
)
if "%choice%"=="4" (
esptool.exe --chip esp32s3 --port COM%NUM% --baud 115200  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 MarineMultiDisplay.ino.bootloader.bin 0x8000 MarineMultiDisplay.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 MarineMultiDisplay.ino.bin
)