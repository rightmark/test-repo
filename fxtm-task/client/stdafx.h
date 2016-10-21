// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// STL
#include <algorithm>
#include <exception>
#include <chrono>
#include <random>

#define NOMINMAX                            // use min/max from STL instead
#define WIN32_LEAN_AND_MEAN

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

#pragma warning(disable : 4127)
#include <atlpath.h>
#include <atltime.h>


#include <ws2tcpip.h>
#include <mstcpip.h>

#include "wsahelper.h"
