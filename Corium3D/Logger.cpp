#include "Logger.h"

#if defined(__ANDROID__) || defined(ANDROID)
	#include <android/log.h>
#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */
	#include <stdio.h>
	#include <stdarg.h>
#endif

namespace Corium3DUtils {

#if defined(__ANDROID__) || defined(ANDROID)
	#define logd_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_DEBUG, logTag, logFmt, argptr)
	#define loge_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_ERROR, logTag, logFmt, argptr)
	#define logi_func(logTag, logFmt, argptr) __android_log_vprint(ANDROID_LOG_INFO, logTag, logFmt, argptr)
#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */
	#define logd_func(logTag, logFmt, argptr) printf("[DEBUG]"); printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n"); fflush(stdout);
	#define loge_func(logTag, logFmt, argptr) printf("[ERROR]"); printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n"); fflush(stdout);
	#define logi_func(logTag, logFmt, argptr) printf("[INFO]"); printf(logTag); printf(">> "); vprintf(logFmt, argptr); printf("\n"); fflush(stdout);
#endif

	Logger::Logger() {}

	void Logger::logd(const char* logTag, const char* logFmt, ...) const {
		va_list argptr;
		va_start(argptr, logFmt);
		logd_func(logTag, logFmt, argptr);
		va_end(argptr);
	}

	void Logger::loge(const char* logTag, const char* logFmt, ...) const {
		va_list argptr;
		va_start(argptr, logFmt);
		loge_func(logTag, logFmt, argptr);
		va_end(argptr);
	}

	void Logger::logi(const char* logTag, const char* logFmt, ...) const {
		va_list argptr;
		va_start(argptr, logFmt);
		logi_func(logTag, logFmt, argptr);
		va_end(argptr);
	}

} // namespace Corium3D