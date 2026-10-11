@echo off
setlocal

:: 改成 mingw1310_64
set "QT_MINGW_PATH=C:\Qt\Qt6.12\Tools\mingw1310_64\bin"
set "QT_CMAKE_PATH=C:\Qt\Qt6.12\Tools\CMake_64\bin"
set "QT_BIN_PATH=C:\Qt\Qt6.12\6.12.0\mingw_64\bin"
set "PATH=%QT_MINGW_PATH%;%QT_CMAKE_PATH%;%QT_BIN_PATH%;%PATH%"

:: 清理旧缓存
if exist build rmdir /s /q build
mkdir build
cd build

cmake .. ^
-DCMAKE_PREFIX_PATH="C:/Qt/Qt6.12/6.12.0/mingw_64" ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_C_COMPILER="%QT_MINGW_PATH%\gcc.exe" ^
-DCMAKE_CXX_COMPILER="%QT_MINGW_PATH%\g++.exe" ^
-G"MinGW Makefiles"

mingw32-make.exe -j 12

windeployqt.exe --release .\gametest.exe --no-translations
cd ..
endlocal
