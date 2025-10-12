@echo off
echo This bat file will download the essential files to program the MarineMultiDisplay Hardware.
echo You will first need to plug the display into a COM port.

:: Use PowerShell to detect ESP32-like COM ports
for /f %%A in ('powershell -Command ^
  "Get-WmiObject Win32_PnPEntity | Where-Object { $_.Name -match 'COM' -and ($_.Name -match 'ESP32|CH340|CP210|FTDI|USB Serial Device') } | ForEach-Object { if ($_.Name -match 'COM(\d+)') { $matches[1] } }"') do (
  set DETECTED_COM=%%A
)

if defined DETECTED_COM (
  echo Detected ESP32 device on COM%DETECTED_COM%
  set /p NUM=Press Enter to confirm or enter a different COM port number [default: %DETECTED_COM%]:
  if "%NUM%"=="" set NUM=%DETECTED_COM%
) else (
  echo No ESP32 device detected automatically.
  set /p NUM=Enter COM port number manually:
)

echo Using COM%NUM% for programming.

echo  "The program will now make local copies of the Binaries needed to program the board"
echo " Select 1 for GUITRON display  Select 2 for WAVshare module 3 for Wide Waveshare '4.3' board"
set /p choice=Enter 1 2 or 3:
if "%choice%"=="3" (
echo " Seleced WIDE (4.3') WAVSHARE VERSION"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/WAVWIDEMMD.ino.bin', 'WAVWIDEMMD.ino.bin')"
echo " renaming for programming"
ren WAVWIDEMMD.ino.bin MarineMultiDisplay.ino.bin
)


if "%choice%"=="2" (
echo " Seleced WAVSHARE VERSION"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/WAVMMD.ino.bin', 'WAVMMD.ino.bin')"
echo " renaming for programming"
ren WAVMMD.ino.bin MarineMultiDisplay.ino.bin
)
if "%choice%"=="1" (
echo " Seleced GUITRON VERSION"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/GUITMMD.ino.bin', 'GUITMMD.ino.bin')"
echo " renaming for programming"
ren GUITMMD.ino.bin MarineMultiDisplay.ino.bin
)

echo " getting other files needed for programing..."

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/esptool.exe', 'esptool.exe')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/MarineMultiDisplay.ino.bootloader.bin', 'MarineMultiDisplay.ino.bootloader.bin')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/MarineMultiDisplay.ino.partitions.bin', 'MarineMultiDisplay.ino.partitions.bin')"
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://dagnall53.github.io/MarineMultiDisplay/build/esp32.esp32.esp32s3/boot_app0.bin', 'boot_app0.bin')"


echo " Starting programmer"
esptool.exe --chip esp32s3 --port COM%NUM% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 MarineMultiDisplay.ino.bootloader.bin 0x8000 MarineMultiDisplay.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 MarineMultiDisplay.ino.bin


echo " I will now delete all files copied here for the programming"
del  MarineMultiDisplay.ino.bootloader.bin  >nul 2>&1
del  MarineMultiDisplay.ino.partitions.bin  >nul 2>&1
del boot_app0.bin >nul 2>&1
del MarineMultiDisplay.ino.bin >nul 2>&1 
del  esptool.exe  >nul 2>&1
echo "Finished press any key to exit"
Pause

