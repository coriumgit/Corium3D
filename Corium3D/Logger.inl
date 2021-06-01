#pragma once

#include "Logger.h"

namespace Corium3DUtils {

#if defined(__ANDROID__) || defined(ANDROID)
	#include <android/log.h>
	#define logd_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_DEBUG, logTag, logFmt, argptr)
	#define loge_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_ERROR, logTag, logFmt, argptr)
	#define logi_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_INFO, logTag, logFmt, argptr)
#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */
	#include <stdio.h>
	#include <stdarg.h>
	#define logd_func(logTag, logFmt, argptr) printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n")
	#define loge_func(logTag, logFmt, argptr) printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n")
	#define logi_func(logTag, logFmt, argptr) printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n")
#endif

	inline void Logger::logd(const char* logTag, const char* logFmt, ...) const {
#ifdef DEBUG
			va_list argptr;
			va_start(argptr, logFmt);
			logd_func(logTag, logFmt, argptr);
			va_end(argptr);
#endif
	}

	inline void Logger::loge(const char* logTag, const char* logFmt, ...) const {
		va_list argptr;
		va_start(argptr, logFmt);
		loge_func(logTag, logFmt, argptr);
		va_end(argptr);
	}

	inline void Logger::logi(const char* logTag, const char* logFmt, ...) const {
		va_list argptr;
		va_start(argptr, logFmt);
		logi_func(logTag, logFmt, argptr);
		va_end(argptr);
	}

} // namespace Corium3D