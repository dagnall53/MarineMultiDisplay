echo "This bat file will download the essential files to program the MarineMultiDisplay Hardware. You will first need to Plug the display into a COM port and enter the Com port for the device"
set /p NUM=Enter Com port number:
echo  "The program will now make local copies of the Binaries needed to program the board"


powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/esptool.exe', 'esptool.exe')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/MarineMultiDisplay.ino.bootloader.bin', 'MarineMultiDisplay.ino.bootloader.bin')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/MarineMultiDisplay.ino.partitions.bin', 'MarineMultiDisplay.ino.partitions.bin')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/boot_app0.bin', 'boot_app0.bin')"

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/MarineMultiDisplay.ino.bin', 'MarineMultiDisplay.ino.bin')"

esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 MarineMultiDisplay.ino.bootloader.bin 0x8000 MarineMultiDisplay.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 MarineMultiDisplay.ino.bin
 
echo " I will now delete all files copied here for the programming"
del  MarineMultiDisplay.ino.bootloader.bin  >nul 2>&1
del  MarineMultiDisplay.ino.partitions.bin  >nul 2>&1
del boot_app0.bin >nul 2>&1
del MarineMultiDisplay.ino.bin >nul 2>&1
del  esptool.exe  >nul 2>&1
echo "Finished press any key to exit"
Pause
