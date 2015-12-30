#-------------------------------------------------
#
# Project created by QtCreator 2013-11-25T19:27:20
#
#-------------------------------------------------

QT       += core gui xml concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = W2ENT_QT
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
    tw1bifextractorui.cpp

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
    tw1bifextractorui.h

FORMS    += mainwindow.ui \
    options.ui \
    search.ui \
    resize.ui \
    cleantexturespath.ui \
    extfiles.ui \
    tw1bifextractorui.ui

# Change the paths here :
LIBS += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-5226-trunk\lib\Win32-gcc\libIrrlicht.a"
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\irrlicht-code-5226-trunk\include" \

# If you use COMPILE_WITH_ASSIMP, set the path for Assimp
LIBS += C:\Users\Jean-Louis\Desktop\Libs\Assimp\CB-BUILD\code\libassimp.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis\Desktop\Libs\Assimp\include" \

DISTFILES += \
    app.rc
