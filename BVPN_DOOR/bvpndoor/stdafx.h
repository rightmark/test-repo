#pragma once

#include "targetver.h"

#define NOMINMAX                // use min/max from STL instead
#define WIN32_LEAN_AND_MEAN

// QT
#pragma warning(once : 4127)    // conditional expression is constant
#include <QtWidgets/QtWidgets>


#if !defined(QT_DLL) // statically linked Qt
#include <QtCore/QtPlugin>

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)

#pragma message(">>> plugins compiling..")
#pragma comment(lib, "Qt5PlatformSupport.lib")
#pragma comment(lib, "platforms/qwindows.lib")
#pragma comment(lib, "imageformats/qico.lib")
#endif


// tweaks
#ifdef _DEBUG
#define _TESTING_
#endif
