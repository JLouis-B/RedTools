#-------------------------------------------------
#
# Project created by QtCreator 2013-11-25T19:27:20
#
#-------------------------------------------------

QT       += core gui xml concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = The_Witcher_Converter
TEMPLATE = app

RC_FILE = app.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    qirrlichtwidget.cpp \
    options.cpp \
    search.cpp \
    translator.cpp \
    resize.cpp \
    CREMeshWriter.cpp \
    CW2ENTMeshFileLoader.cpp \
    IrrAssimp/IrrAssimp.cpp \
    IrrAssimp/IrrAssimpExport.cpp \
    IrrAssimp/IrrAssimpImport.cpp \
    CW3ENTMeshFileLoader.cpp \
    cleantexturespath.cpp \
    extfiles.cpp \
    log.cpp \
    utils.cpp \
    IrrAssimp/IrrAssimpUtils.cpp \
    CREMeshFileLoader.cpp \
    CSkeleton.cpp \
    settings.cpp \
    meshcombiner.cpp \
    tw1bifextractor.cpp \
    tw1bifextractorui.cpp \
    CWitcherMDLMeshFileLoader.cpp \
    LoadersUtils.cpp \
    W3_DataCache.cpp

HEADERS  += mainwindow.h \
    qirrlichtwidget.h \
    options.h \
    search.h \
    translator.h \
    resize.h \
    CREMeshWriter.h \
    CW2ENTMeshFileLoader.h \
    IrrAssimp/IrrAssimp.h \
    IrrAssimp/IrrAssimpExport.h \
    IrrAssimp/IrrAssimpImport.h \
    CW3ENTMeshFileLoader.h \
    cleantexturespath.h \
    extfiles.h \
    log.h \
    utils.h \
    IrrAssimp/IrrAssimpUtils.h \
    CREMeshFileLoader.h \
    CSkeleton.h \
    settings.h \
    halffloat.h \
    meshcombiner.h \
    tw1bifextractor.h \
    tw1bifextractorui.h \
    CWitcherMDLMeshFileLoader.h \
    LoadersUtils.h \
    W3_DataCache.h

FORMS    += mainwindow.ui \
    options.ui \
    search.ui \
    resize.ui \
    cleantexturespath.ui \
    extfiles.ui \
    tw1bifextractorui.ui

# Change the paths here :
LIBS += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-5323-trunk\lib\Win32-gcc\libIrrlicht.a"
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-5323-trunk\include" \

# If you use COMPILE_WITH_ASSIMP, set the path for Assimp
LIBS += C:\Users\Jean-Louis\Desktop\Libs\assimp-master\CB_build\code\libassimp.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\assimp-master\include" \

DISTFILES += \
    app.rc
