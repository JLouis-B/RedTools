cd %~dp0

set release-dir=./../../Dev/build-W2ENT_QT-MinGW_32bit/release
set data-dir=../data
set tools-dir=../tools
set qt-plugin-dir=C:\Qt\5.11.1\mingw53_32\plugins

set prefix=The_witcher_converter
set /p version=Version(%prefix%_version):

set foldername=./%prefix%_%version%
set filename=%prefix%_%version%.zip
echo %filename%


:: create build dir
rmdir "%foldername%"
mkdir "%foldername%"

::copy qwindows.dll
set foldername-plugin=%foldername%/plugins
mkdir "%foldername-plugin%"
copy "%qt-plugin-dir%\platforms\qwindows.dll" "%foldername-plugin%"

:: copy dll and exe
cd "%release-dir%"
copy *.dll "%~dp0/%foldername%"
copy The_Witcher_Converter.exe "%~dp0/%foldername%"
cd %~dp0

:: copy data folder
cd "%data-dir%"
xcopy /s * "%~dp0%foldername%"
cd %~dp0

set command=7z.exe a %filename% %foldername%
%command%

:: delete build dir
rmdir "%foldername%"

pause