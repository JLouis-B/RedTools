set release-dir="../../Dev/build-W2ENT_QT-MinGW_32bit/release"
set data-dir="../data"
set qt-plugin-dir=C:\Qt\5.11.1\mingw53_32\plugins

set /p version=Version:
set prefix=The_witcher_converter

set filename=%prefix%_%version%.zip
echo %filename%

::copy qwindows.dll
rmdir plugins
mkdir plugins
copy "%qt-plugin-dir%\platforms\qwindows.dll" ".\plugins"

set filelist=%release-dir%/The_Witcher_Converter.exe
set filelist= %filelist% %release-dir%/Qt5Core.dll
set filelist= %filelist% %release-dir%/Qt5Gui.dll
set filelist= %filelist% %release-dir%/Qt5Widgets.dll
set filelist= %filelist% %release-dir%/Qt5Xml.dll
set filelist= %filelist% %release-dir%/libgcc_s_dw2-1.dll
set filelist= %filelist% %release-dir%/libstdc++-6.dll
set filelist= %filelist% %release-dir%/libwinpthread-1.dll
set filelist= %filelist% %release-dir%/Irrlicht.dll
set filelist= %filelist% %release-dir%/libassimp.dll
set filelist= %filelist% %release-dir%/liblz4.dll
set filelist= %filelist% %release-dir%/snappy32.dll
set filelist= %filelist% %data-dir%
set filelist= %filelist% plugins
echo %filelist%

set command=7z.exe a %filename% %filelist%
echo %command%
%command%
pause