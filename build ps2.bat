@echo off
setlocal
cd /d "%~dp0"

REM PS2 build changes often add/remove generated objects and compatibility shims.
REM Default to incremental builds so small PS2 iteration changes do not rebuild
REM the whole project. Use:
REM   build ps2.bat clean
REM only when CMake/source-list/toolchain changes leave stale objects behind.
if /I "%~1"=="clean" (
    if exist "build\ps2-release" rmdir /s /q "build\ps2-release"
)

REM El log siempre va a <prefijo de guardado>/userdata/log.txt (como el juego
REM original). El tracer de colores de borde esta apagado por defecto; para
REM activarlo, poner PS2_TRACE_ENABLE a 1 en Ps2Trace.h y recompilar.
cmake --preset ps2-release
cmake --build --preset ps2-release --parallel
pause


:: Optional ISO
if /i not "%1"=="iso" goto :end

echo.
echo === Creating ISO ===

set ISO_ROOT=%PROJECT_DIR%iso_root
mkdir "%ISO_ROOT%" 2>nul
copy /Y "%BUILD_DIR%\%ELF_NAME%" "%ISO_ROOT%\SLUS_67420.69" >nul
xcopy /E /I /Y "%PROJECT_DIR%data" "%ISO_ROOT%\data" >nul

> "%ISO_ROOT%\SYSTEM.CNF" (
    echo BOOT2 = cdrom0:\SLUS_67420.69;1
    echo VER = 1.00
    echo VMODE = NTSC
    echo HDDUNITPOWER = NICHDD
)

where mkisofs >nul 2>&1
if errorlevel 1 where genisoimage >nul 2>&1
if errorlevel 1 (
    echo WARNING: mkisofs not found. Install: https://github.com/tpn/winsdk-10/raw/master/cdrtools
    echo ELF still works in PCSX2 without ISO.
    goto :end
)

mkisofs -o "%ISO_NAME%" -V "PVZ PS2" -joliet -rock -publisher "OptiJuegos" "%ISO_ROOT%"
echo   ISO: %ISO_NAME%

:end
pause

