#pragma once

// correct project settings checkout..

#if (_MSC_VER < 1700)
#error 'C++11 syntax compilation requires at least Visual C++ 2012
#endif

#if !defined(_MT)
#error 'Compilation requires multi-threaded environment
#endif

/*
// @TODO: km 20161210 - static QT library should be prepared..
#if defined(_DLL) && !defined(_DEBUG)
#error 'Compilation requires static multi-threaded CRT for Release build
#endif
*/

#if defined(_CHAR_UNSIGNED)
#error 'Char type must be signed
#endif

#if !defined(_CPPUNWIND)
#error 'Exception handling must be enabled
#endif

#if defined(_CPPRTTI) && !defined(_DEBUG)
#error 'Run-Time Type Information should be disabled for Release build
#endif


// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>
