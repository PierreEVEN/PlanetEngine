#pragma once

#include <GL/gl3w.h>
#include <iostream>
#include <cassert>

#ifndef GL_TABLE_TOO_LARGE
#define GL_TABLE_TOO_LARGE 0x8031
#endif

inline void GL_CHECK_ERROR()
{
	int gl_error_id = glGetError();

    while (gl_error_id != GL_NO_ERROR) {
        std::cerr << "opengl error : ";
        switch (gl_error_id)
        {
        case GL_NO_ERROR:
            std::cerr << "GL_NO_ERROR";
            break;
        case GL_INVALID_ENUM:
            std::cerr << "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            std::cerr << "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            std::cerr << "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            std::cerr << "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            std::cerr << "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            std::cerr << "GL_OUT_OF_MEMORY";
            break;
        case GL_TABLE_TOO_LARGE:
            std::cerr << "GL_TABLE_TOO_LARGE";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            std::cerr << "unknown error";
        }
        std::cerr << std::endl;

#ifdef _DEBUG
        assert((void("opengl error"), gl_error_id == GL_NO_ERROR));
#endif
        gl_error_id = glGetError();
    }
}