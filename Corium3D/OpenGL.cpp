//
// Created by omer on 27/01/02018.
//

#include "OpenGL.h"

namespace Corium3D {

    GLuint addShader(GLuint prog, const GLchar* shaderCode, GLenum shaderType);
    GLuint linkProg(GLuint prog);

    bool createGlProg(const GLchar* vertexShaderCode, const GLchar* fragShaderCode,
        GLuint* progOut, GLuint* vertexShaderOut, GLuint* fragShaderOut) {
        *progOut = glCreateProgram();
        CHECK_GL_ERROR("glCreateProgram");
#if !DEBUG
        addShader(*progOut, vertexShaderCode, GL_VERTEX_SHADER);
        addShader(*progOut, fragShaderCode, GL_FRAGMENT_SHADER);
        linkProg(*progOut);
#else
        if (!addShader(*progOut, vertexShaderCode, GL_VERTEX_SHADER) || !addShader(*progOut, fragShaderCode, GL_FRAGMENT_SHADER) || !linkProg(*progOut))
            return false;
#endif

        return true;
    }

    GLuint addShader(GLuint prog, const GLchar* shaderCode, GLenum shaderType) {
        GLuint shaderObj = glCreateShader(shaderType);
        CHECK_GL_ERROR("glCreateShader");
        glShaderSource(shaderObj, 1, &shaderCode, NULL);
        glCompileShader(shaderObj);

#if DEBUG
        GLint wasCompileOk = GL_FALSE;
        glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &wasCompileOk);
        if (wasCompileOk != GL_TRUE) {
            GLint infoLogLen = 0;
            glGetShaderiv(shaderObj, GL_INFO_LOG_LENGTH, &infoLogLen);
            if (infoLogLen > 0) {
                GLchar* infoLog = new GLchar[infoLogLen];
                glGetShaderInfoLog(shaderObj, infoLogLen, NULL, infoLog);
                ServiceLocator::getLogger().loge("createGlProg", "Could not compile shader. compiler says:\n%s\n", infoLog);
                delete infoLog;
            }
            glDeleteShader(shaderObj);
            return 0;
        }
#endif

        glAttachShader(prog, shaderObj);

        return shaderObj;
    }

    GLuint linkProg(GLuint prog) {
        glLinkProgram(prog);
#if DEBUG
        GLint wasLinkOk;
        glGetProgramiv(prog, GL_LINK_STATUS, &wasLinkOk);
        if (wasLinkOk != GL_TRUE) {
            GLint infoLogLen = 0;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);
            if (infoLogLen) {
                GLchar* infoLog = new GLchar[infoLogLen];
                glGetProgramInfoLog(prog, infoLogLen, NULL, infoLog);
                ServiceLocator::getLogger().loge("Could not link program:\n%s\n", infoLog);
                delete infoLog;
            }

            return 0;
        }
#endif
        return prog;
    }

} // namespace Corium3D