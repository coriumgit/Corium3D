#include "Timer.h"

#if defined(__ANDROID__) || defined(ANDROID)
	#include <Time.h>
#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */
	#include <Windows.h>
#endif

namespace Corium3DUtils {

	Timer::Timer() {}

#if defined(__ANDROID__) || defined(ANDROID)

	long long Timer::getCurrentTime(void) const {
		struct timespec res;
		clock_gettime(CLOCK_REALTIME, &res);
		return res.tv_sec + res.tv_nsec / 1000000000;
	}

#elif defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__) /* Win32 and WinCE */

	inline long long initPerformanceFreq() {
		LARGE_INTEGER performanceFreq;
		QueryPerformanceFrequency(&performanceFreq);
		return performanceFreq.QuadPart;
	}

	// returns time in seconds
	double Timer::getCurrentTime(void) const {
		static const long long performanceFreq(initPerformanceFreq());
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		return (double)time.QuadPart / performanceFreq;
	}

#endif

} // namespace Corium3DUtils