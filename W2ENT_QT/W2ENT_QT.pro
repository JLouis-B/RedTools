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
    DomHelper.cpp \
    IrrAssimp/IrrAssimp.cpp \
    IrrAssimp/IrrAssimpExport.cpp \
    IrrAssimp/IrrAssimpImport.cpp \
    Extractor_TW1_BIF.cpp \
    Extractor_TW2_DZIP.cpp \
    GUI_ExtFilesExplorer.cpp \
    GUI_Extractor_TW2_DZIP.cpp \
    GUI_Extractor_TW1_BIF.cpp \
    GUI_MaterialsExplorer.cpp \
    GUI_MainWindow.cpp \
    GUI_Options.cpp \
    IrrAssimp/IrrAssimpUtils.cpp \
    GUI_Search.cpp \
    Log/CallbackLogger.cpp \
    Log/ConsoleLogger.cpp \
    Log/IAppLogger.cpp \
    Log/IrrFileLogger.cpp \
    Log/LoggerManager.cpp \
    Settings.cpp \
    GUI_Resize.cpp \
    Translator.cpp \
    QIrrlichtWidget.cpp \
    MeshCombiner.cpp \
    TW3_DataCache.cpp \
    GUI_CleanTexturesPath.cpp \
    UIThemeManager.cpp \
    Utils_Qt.cpp \
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
    IO_MeshLoader_CEF.cpp \
    IO_MeshLoader_TheCouncil_Prefab.cpp \
    IO_SceneLoader_TheCouncil.cpp \
    Utils_RedEngine.cpp \
    Utils_TheCouncil.cpp \
    Extractor_Dishonored2.cpp \
    GUI_Extractor_Dishonored2.cpp

HEADERS  += \
    CompileConfig.h \
    DomHelper.h \
    IrrAssimp/IrrAssimp.h \
    IrrAssimp/IrrAssimpExport.h \
    IrrAssimp/IrrAssimpImport.h \
    Extractor_TW1_BIF.h \
    Extractor_TW2_DZIP.h \
    GUI_ExtFilesExplorer.h \
    GUI_Extractor_TW2_DZIP.h \
    GUI_Extractor_TW1_BIF.h \
    IrrAssimp/IrrAssimpUtils.h \
    Log/CallbackLogger.h \
    Log/ConsoleLogger.h \
    Log/IAppLogger.h \
    Log/IrrFileLogger.h \
    Log/LoggerManager.h \
    Settings.h \
    GUI_Search.h \
    GUI_Resize.h \
    GUI_Options.h \
    TW3_DataCache.h \
    MeshCombiner.h \
    Translator.h \
    GUI_CleanTexturesPath.h \
    QIrrlichtWidget.h \
    GUI_MainWindow.h \
    GUI_MaterialsExplorer.h \
    UIThemeManager.h \
    Utils_Qt.h \
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
    IO_MeshLoader_CEF.h \
    IO_MeshLoader_TheCouncil_Prefab.h \
    IO_SceneLoader_TheCouncil.h \
    Utils_RedEngine.h \
    Utils_TheCouncil.h \
    Extractor_Dishonored2.h \
    GUI_Extractor_Dishonored2.h

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
    GUI_Extractor_TheCouncil.ui \
    GUI_Extractor_Dishonored2.ui

# Change the paths here :
LIBS += "C:\Users\Jean-Louis\Desktop\Libs\irr-svn\lib\Win32-gcc\libIrrlicht.a"
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\irr-svn\include" \

# If you use COMPILE_WITH_ASSIMP, set the path for Assimp
LIBS += C:\Users\Jean-Louis\Desktop\Libs\assimp-5.4.1\Builds\CB_Build\lib\libassimp.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\assimp-5.4.1\include" \
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\assimp-5.4.1\Builds\CB_Build\include" \

# We use the zlib bundled with Qt
LIBS += -lz

# SNAPPY
LIBS += C:\Users\Jean-Louis\Desktop\Libs\snappy-windows-1.1.1.8\native\snappy32.lib
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\snappy-windows-1.1.1.8\include" \

# LZ4
LIBS += C:\Users\Jean-Louis\Desktop\Libs\lz4_win32_v1_9_4\dll\liblz4.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\lz4_win32_v1_9_4\include" \

DISTFILES += \
    app.rc
