@echo off
cd %~dp0

echo The Witcher Converter Packager
echo ------------------------------
@echo on

set release-dir=./../../Dev/build-W2ENT_QT-MinGW_32bit/release
set data-dir=../data
set tools-dir=../tools
set qt-plugin-dir=C:\Qt\5.11.1\mingw53_32\plugins

set prefix=The_witcher_converter
@echo off
set /p version=Set the version (%prefix%_version.zip) : 

set filename=%prefix%_%version%.zip
echo generated file will be : %filename%
@echo on

:: create build dir
@echo off
set foldername=./%prefix%_%version%
@echo on
rmdir "%foldername%" /s /q
mkdir "%foldername%"

::copy qwindows.dll
@echo off
set foldername-plugin=%foldername%/plugins
@echo on
mkdir "%foldername-plugin%"
copy "%qt-plugin-dir%\platforms\qwindows.dll" "%foldername-plugin%"

:: copy dll and exe
cd "%release-dir%"
copy *.dll "%~dp0%foldername%"
copy The_Witcher_Converter.exe "%~dp0%foldername%"

:: copy data folder
cd "%~dp0%data-dir%"
xcopy /s * "%~dp0%foldername%"
cd %~dp0

:: 7zip
del %filename%
@echo off
set command=7z.exe a %filename% %foldername%
@echo on
%command%

:: delete build dir
rmdir "%foldername%" /s /q

pause