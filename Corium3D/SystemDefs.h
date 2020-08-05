#pragma once

#if defined(__ANDROID__) || defined(ANDROID)
typedef struct ANativeWindow* Corium3DNativeWindowType;

#elif defined(_WIN32) || defined(__VC32__) || defined(_WIN64) || defined(__VC64__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */
#include <windows.h>
typedef HWND Corium3DEngineNativeWindowType;

#endif

