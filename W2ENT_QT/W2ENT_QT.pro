#-------------------------------------------------
#
# Project created by QtCreator 2013-11-25T19:27:20
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = The_Witcher_Converter
TEMPLATE = app

RC_FILE = app.rc

SOURCES += main.cpp\
    IrrAssimp/IrrAssimp.cpp \
    IrrAssimp/IrrAssimpExport.cpp \
    IrrAssimp/IrrAssimpImport.cpp \
    IrrAssimp/IrrAssimpUtils.cpp \
    Extractor_TW1_BIF.cpp \
    Extractor_TW2_DZIP.cpp \
    GUI_ExtFilesExplorer.cpp \
    GUI_Extractor_TW2_DZIP.cpp \
    GUI_Extractor_TW1_BIF.cpp \
    GUI_MaterialsExplorer.cpp \
    GUI_MainWindow.cpp \
    GUI_Options.cpp \
    Log.cpp \
    GUI_Search.cpp \
    Settings.cpp \
    GUI_Resize.cpp \
    Translator.cpp \
    QIrrlichtWidget.cpp \
    MeshCombiner.cpp \
    TW3_DataCache.cpp \
    GUI_CleanTexturesPath.cpp \
    Utils_TW.cpp \
    IO_MeshLoader_W3ENT.cpp \
    IO_MeshLoader_W2ENT.cpp \
    IO_MeshLoader_WitcherMDL.cpp \
    IO_MeshLoader_RE.cpp \
    IO_MeshWriter_RE.cpp \
    TW3_CSkeleton.cpp \
    Extractor_TW3_BUNDLE.cpp \
    GUI_Extractor_TW3_BUNDLE.cpp \
    Utils_Loaders_Irr.cpp \
    Utils_Loaders_Qt.cpp \
    DOBOZ/Compressor.cpp \
    DOBOZ/Decompressor.cpp \
    DOBOZ/Dictionary.cpp \
    GUI_Extractor_TW3_CACHE.cpp \
    Extractor_TW3_CACHE.cpp \
    Utils_Qt_Irr.cpp \
    GUI_About.cpp \
    IO_SpeedTreeLoader.cpp \
    GUI_Extractor_TheCouncil.cpp \
    Extractor_TheCouncil.cpp \
    IO_MeshLoader_CEF.cpp

HEADERS  += \
    IrrAssimp/IrrAssimp.h \
    IrrAssimp/IrrAssimpExport.h \
    IrrAssimp/IrrAssimpImport.h \
    IrrAssimp/IrrAssimpUtils.h \
    Extractor_TW1_BIF.h \
    Extractor_TW2_DZIP.h \
    GUI_ExtFilesExplorer.h \
    GUI_Extractor_TW2_DZIP.h \
    GUI_Extractor_TW1_BIF.h \
    Settings.h \
    GUI_Search.h \
    GUI_Resize.h \
    GUI_Options.h \
    TW3_DataCache.h \
    MeshCombiner.h \
    Log.h \
    Translator.h \
    GUI_CleanTexturesPath.h \
    QIrrlichtWidget.h \
    GUI_MainWindow.h \
    GUI_MaterialsExplorer.h \
    Utils_TW.h \
    IO_MeshLoader_WitcherMDL.h \
    IO_MeshLoader_W3ENT.h \
    IO_MeshLoader_W2ENT.h \
    IO_MeshWriter_RE.h \
    IO_MeshLoader_RE.h \
    Utils_Qt_Irr.h \
    TW3_CSkeleton.h \
    Utils_Halffloat.h \
    Extractor_TW3_BUNDLE.h \
    GUI_Extractor_TW3_BUNDLE.h \
    Utils_Loaders_Irr.h \
    Utils_Loaders_Qt.h \
    DOBOZ/Common.h \
    DOBOZ/Compressor.h \
    DOBOZ/Decompressor.h \
    DOBOZ/Dictionary.h \
    GUI_Extractor_TW3_CACHE.h \
    Extractor_TW3_CACHE.h \
    GUI_About.h \
    IO_SpeedTreeLoader.h \
    GUI_Extractor_TheCouncil.h \
    Extractor_TheCouncil.h \
    IO_MeshLoader_CEF.h

FORMS    += \
    GUI_ExtFilesExplorer.ui \
    GUI_Extractor_TW2_DZIP.ui \
    GUI_Extractor_TW1_BIF.ui \
    GUI_Search.ui \
    GUI_MainWindow.ui \
    GUI_MaterialsExplorer.ui \
    GUI_Options.ui \
    GUI_Resize.ui \
    GUI_CleanTexturesPath.ui \
    GUI_Extractor_TW3_BUNDLE.ui \
    GUI_Extractor_TW3_CACHE.ui \
    GUI_About.ui \
    GUI_Extractor_TheCouncil.ui

# Change the paths here :
LIBS += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-r5622-trunk\lib\Win32-gcc\libIrrlicht.a"
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-r5622-trunk\include" \

# If you use COMPILE_WITH_ASSIMP, set the path for Assimp
LIBS += C:\Users\Jean-Louis\Desktop\Libs\assimp-master\Builds\CB_Build\code\libassimp.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\assimp-master\include" \
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\assimp-master\Builds\CB_Build\include" \

# SNAPPY
LIBS += C:\Users\Jean-Louis\Desktop\Libs\snappy-windows-1.1.1.8\native\snappy32.lib
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\snappy-windows-1.1.1.8\include" \

# LZ4
LIBS += C:\Users\Jean-Louis\Desktop\Libs\lz4_v1_8_2_win32\dll\liblz4.lib
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\lz4_v1_8_2_win32\include" \

DISTFILES += \
    app.rc
