#-------------------------------------------------
#
# Project created by QtCreator 2013-11-25T19:27:20
#
#-------------------------------------------------

QT       += core gui xml

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
    meshcombiner.cpp

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
    meshcombiner.h

FORMS    += mainwindow.ui \
    options.ui \
    search.ui \
    resize.ui \
    cleantexturespath.ui \
    extfiles.ui

# Change the paths here :
LIBS += "C:\Users\Jean-Louis_\Desktop\Libs\irrlicht-code-5103-trunk\lib\Win32-gcc\libIrrlicht.a"
INCLUDEPATH += "C:\Users\Jean-Louis_\Desktop\Libs\irrlicht-code-5103-trunk\include" \

# If you use COMPILE_WITH_ASSIMP, set the path for Assimp
LIBS += C:\Users\Jean-Louis_\Desktop\Libs\assimp-master\CB_build\code\libassimp.dll.a
INCLUDEPATH += "C:\Users\Jean-Louis_\Desktop\Libs\assimp-master\include" \

DISTFILES += \
    app.rc
