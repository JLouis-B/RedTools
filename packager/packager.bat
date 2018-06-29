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


:: create folder
rmdir "%foldername%"
mkdir "%foldername%"

::copy qwindows.dll
set foldername-plugin=%foldername%/plugins
mkdir "%foldername-plugin%"
copy "%qt-plugin-dir%\platforms\qwindows.dll" "%foldername-plugin%"

::copy tools
set foldername-tools=%foldername%/tools
mkdir "%foldername-tools%"
xcopy "%tools-dir%" "%foldername-tools%"

:: copy dll and exe
cd "%release-dir%"
copy *.dll "%~dp0/%foldername%"
copy The_Witcher_Converter.exe "%~dp0/%foldername%"
cd %~dp0

:: copy data folder
cd "%data-dir%"
copy * "%~dp0/%foldername%"
mkdir "%~dp0%foldername%/langs"
xcopy langs "%~dp0%foldername%/langs"
cd %~dp0

set command=7z.exe a %filename% %foldername%
echo %command%
%command%
pause