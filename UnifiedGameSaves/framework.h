// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <winioctl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

// C++ Standard Library Headers
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
