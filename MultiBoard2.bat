@echo off
setlocal enabledelayedexpansion

:: === ADD ARDUINO CLI TO PATH ===
set PATH=%PATH%;C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\

:: === GLOBAL CONFIG ===
set "CLI=arduino-cli"
set "SKETCH_DIR=C:\Users\dagna\OneDrive\DocOneDrive\Arduino\MarineMultiDisplay"
set "BUILD_DIR=C:\build\shared"
set "IDE_BUILD_DIR=%SKETCH_DIR%\build\esp32.esp32.esp32s3"
set "PARTITION=ota_16MB_clean"
set "FLASH_SIZE=16MB"

:: === CLEAN STALE BUILD FOLDERS ===
echo Cleaning stale build folders...
for /d %%D in ("%LOCALAPPDATA%\arduino\sketches\*") do (
    echo Deleting %%D
    rmdir /s /q "%%D"
)

:: === Ensure IDE build directory exists ===
if not exist "%IDE_BUILD_DIR%" (
    echo Creating IDE build directory: %IDE_BUILD_DIR%
    mkdir "%IDE_BUILD_DIR%"
)

:: === Compile Targets ===
:: Pass the entire define string as a single quoted argument (keeps -D tokens together)
call :CompileTarget GUITMMD esp32:esp32:esp32s3 "-DESP32_SPIRAM -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue" 0
call :CompileTarget WAVWIDEMMD esp32:esp32:esp32s3 "-DWAVSHARE -DWIDEBOX -DESP32_SPIRAM -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue" 1
call :CompileTarget WAVMMD esp32:esp32:esp32s3 "-DWAVSHARE -DESP32_SPIRAM -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue" 1

echo All builds complete.
endlocal
pause
goto :EOF

:: === CompileTarget Subroutine ===
:CompileTarget
:: args: 1=TARGET_NAME 2=FQBN 3="DEFINE_STRING" 4=USB_CDC
set "TARGET_NAME=%~1"
set "FQBN=%~2"
set "EXTRA_FLAGS=%~3"
set "USB_CDC=%~4"

echo.
echo Compiling %TARGET_NAME%...

:: Ensure build folder exists
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Run compile; pass the full flags string only inside --build-property values (quoted)
"%CLI%" compile --fqbn "%FQBN%" ^
    --build-path "%BUILD_DIR%" ^
    --build-property build.partitions=%PARTITION% ^
    --build-property build.flash_size=%FLASH_SIZE% ^
    --build-property build.compression=enabled ^
    --build-property build.usb_cdc_on_boot=%USB_CDC% ^
    --build-property compiler.cpp.extra_flags="%EXTRA_FLAGS%" ^
    --build-property compiler.c.extra_flags="%EXTRA_FLAGS%" ^
    --build-property compiler.S.extra_flags="%EXTRA_FLAGS%" ^
    --verbose

:: Copy .bin and .partitions.bin
set "BIN_SOURCE=%BUILD_DIR%\MarineMultiDisplay.ino.bin"
set "PART_SOURCE=%BUILD_DIR%\MarineMultiDisplay.ino.partitions.bin"
set "BIN_DEST=%IDE_BUILD_DIR%\%TARGET_NAME%.ino.bin"
set "PART_DEST=%IDE_BUILD_DIR%\%TARGET_NAME%.partitions.bin"

echo Copying binaries for %TARGET_NAME%...
if exist "%BIN_SOURCE%" (
    copy /y "%BIN_SOURCE%" "%BIN_DEST%"
    echo Binary copied to: %BIN_DEST%
) else (
    echo ERROR: Binary not found at %BIN_SOURCE%
)

if exist "%PART_SOURCE%" (
    copy /y "%PART_SOURCE%" "%PART_DEST%"
    echo Partition copied to: %PART_DEST%
) else (
    echo ERROR: Partition binary not found at %PART_SOURCE%
)

:: Clean build folder
echo Cleaning build folder...
rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

goto :EOF