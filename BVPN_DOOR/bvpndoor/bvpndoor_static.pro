# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

TEMPLATE = app
TARGET = bvpndoor_static
DESTDIR = ../Win32/Release-static
QT += core widgets gui
CONFIG += release
DEFINES += QT_NO_DEBUG_OUTPUT QT_WIDGETS_LIB
INCLUDEPATH += . \
    ./GeneratedFiles \
    ./GeneratedFiles/Release-static
LIBS += -L"$(QTDIR)/plugins" \
    -lWs2_32 \
    -lwinmm \
    -limm32
PRECOMPILED_HEADER = stdafx.h
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/Release-static
OBJECTS_DIR += Release-static
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(bvpndoor_static.pri)
