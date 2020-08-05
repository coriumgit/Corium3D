//
// Created by omer on 07/01/02018.
//

#pragma once

// #define LOGD(logTag, ...) do {if (DEBUG) Logger::logd(logTag, __VA_ARGS__);} while (0)
// #define LOGE(logTag, ...) do {Logger::loge(logTag, __VA_ARGS__);} while (0)
// #define LOGI(logTag, ...) do {Logger::logi(logTag, __VA_ARGS__);} while (0)

namespace Corium3DUtils {

	class Logger {
	public:
		Logger();
		void logd(const char* logTag, const char* logFmt, ...) const;
		void loge(const char* logTag, const char* logFmt, ...) const;
		void logi(const char* logTag, const char* logFmt, ...) const;
	};

} // namespace Corium3D

