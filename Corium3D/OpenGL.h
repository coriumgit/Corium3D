//
// Created by omer on 27/01/02018.
//

#pragma once

#include "ServiceLocator.h"

#if defined(__ANDROID__) || defined(ANDROID)
	#if __ANDROID_API__ >= 24
		#include <GLES3/gl32.h>
	#elif __ANDROID_API__ >= 21
		#include <GLES3/gl31.h>
	#else
		#include <GLES3/gl3.h>
	#endif
#elif defined(_WIN32) || defined(__VC32__) || defined(_WIN64) || defined(__VC64__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1
	#endif

	#include <GL/glew.h>
	#include <GL/wglew.h>
#endif

namespace Corium3D {

#define CHECK_GL_ERROR(func_name) do { if (DEBUG && checkGlError(func_name)) return 0; } while (0)

#define CHECK_FRAMEBUFFER_STATUS(target) do{ if (DEBUG && !checkFramebufferStatus(target)) return 0; } while(0)

	bool createGlProg(const GLchar* vertexShaderCode, const GLchar* fragShaderCode,
		GLuint* progOut, GLuint* vertexShaderOut, GLuint* fragShaderOut);

	inline void glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		if (type == GL_DEBUG_TYPE_ERROR)
			ServiceLocator::getLogger().loge("GL", "type: 0x%x | severity: 0x%x | message: %s", type, severity, message);
		else
			ServiceLocator::getLogger().logi("GL", "type: 0x%x | severity: 0x%x | message: %s", type, severity, message);
	}

	inline bool checkGlError(const char* funcName) {
		GLint err = glGetError();
		if (err != GL_NO_ERROR) {
			ServiceLocator::getLogger().loge("Corium3DOpenGL", "GL error after %s(): 0x%08x\n", funcName, err);
			return true;
		}
		return false;
	}

	inline bool checkFramebufferStatus(GLenum target) {
		GLenum framebufferStatus = glCheckFramebufferStatus(target);
		if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
			switch (framebufferStatus) {
			case GL_FRAMEBUFFER_UNDEFINED:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "Default framebuffer is bound, but is undefined.");
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "Necessary attachment is uninitialized.");
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "No image is attached to the framebuffer.");
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "Every drawing buffer has an attachment.");
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "An attachment exists for reading.");
			case GL_FRAMEBUFFER_UNSUPPORTED:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "Combination of images are incompatible by implementation requierments.");
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				ServiceLocator::getLogger().loge("Corium3DOpenGL", "Number of samples for all images do not match.");
			}

			return false;
		}

		return true;
	}

} // namespace Corium3D
