#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stdexcept>
#include <string>

// The complete loader. Even OpenGL 1.1 functions go through SDL on both platforms.
#define MARS_GL_FUNCTIONS(X) \
 X(void,Viewport,(GLint,GLint,GLsizei,GLsizei)) \
 X(void,Scissor,(GLint,GLint,GLsizei,GLsizei)) \
 X(void,ClearColor,(GLfloat,GLfloat,GLfloat,GLfloat)) \
 X(void,Clear,(GLbitfield)) \
 X(void,Enable,(GLenum)) X(void,Disable,(GLenum)) \
 X(void,BlendFunc,(GLenum,GLenum)) \
 X(void,DepthMask,(GLboolean)) \
 X(GLenum,GetError,()) X(const GLubyte*,GetString,(GLenum)) \
 X(void,GenVertexArrays,(GLsizei,GLuint*)) X(void,BindVertexArray,(GLuint)) \
 X(void,DeleteVertexArrays,(GLsizei,const GLuint*)) \
 X(void,GenBuffers,(GLsizei,GLuint*)) X(void,BindBuffer,(GLenum,GLuint)) \
 X(void,BufferData,(GLenum,GLsizeiptr,const void*,GLenum)) \
 X(void,DeleteBuffers,(GLsizei,const GLuint*)) \
 X(void,EnableVertexAttribArray,(GLuint)) \
 X(void,VertexAttribPointer,(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*)) \
 X(GLuint,CreateShader,(GLenum)) X(void,ShaderSource,(GLuint,GLsizei,const GLchar* const*,const GLint*)) \
 X(void,CompileShader,(GLuint)) X(void,GetShaderiv,(GLuint,GLenum,GLint*)) \
 X(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) X(void,DeleteShader,(GLuint)) \
 X(GLuint,CreateProgram,()) X(void,AttachShader,(GLuint,GLuint)) X(void,LinkProgram,(GLuint)) \
 X(void,GetProgramiv,(GLuint,GLenum,GLint*)) X(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei*,GLchar*)) \
 X(void,DeleteProgram,(GLuint)) X(void,UseProgram,(GLuint)) \
 X(GLint,GetUniformLocation,(GLuint,const GLchar*)) X(void,Uniform1f,(GLint,GLfloat)) \
 X(void,Uniform3f,(GLint,GLfloat,GLfloat,GLfloat)) \
 X(void,Uniform1i,(GLint,GLint)) X(void,ActiveTexture,(GLenum)) \
 X(void,DrawElements,(GLenum,GLsizei,GLenum,const void*)) X(void,DrawArrays,(GLenum,GLint,GLsizei)) \
 X(void,MultiDrawElements,(GLenum,const GLsizei*,GLenum,const void* const*,GLsizei)) \
 X(void,GenFramebuffers,(GLsizei,GLuint*)) X(void,BindFramebuffer,(GLenum,GLuint)) \
 X(void,DeleteFramebuffers,(GLsizei,const GLuint*)) X(GLenum,CheckFramebufferStatus,(GLenum)) \
 X(void,FramebufferTexture2D,(GLenum,GLenum,GLenum,GLuint,GLint)) \
 X(void,GenTextures,(GLsizei,GLuint*)) X(void,BindTexture,(GLenum,GLuint)) \
 X(void,TexImage2D,(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*)) \
 X(void,TexImage3D,(GLenum,GLint,GLint,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*)) \
 X(void,GenerateMipmap,(GLenum)) \
 X(void,TexParameteri,(GLenum,GLenum,GLint)) X(void,DeleteTextures,(GLsizei,const GLuint*)) \
 X(void,BlitFramebuffer,(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum)) \
 X(void,ReadPixels,(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*)) \
 X(void,PixelStorei,(GLenum,GLint))

namespace mars {
struct GL {
#define DECLARE(ret,name,args) using name##Fn=ret(APIENTRY*)args; name##Fn name=nullptr;
    MARS_GL_FUNCTIONS(DECLARE)
#undef DECLARE
    GL() {
#define LOAD(ret,name,args) name=reinterpret_cast<name##Fn>(SDL_GL_GetProcAddress("gl" #name)); \
        if(!name) throw std::runtime_error("Missing OpenGL entry point: gl" #name);
        MARS_GL_FUNCTIONS(LOAD)
#undef LOAD
    }
};
}
#undef MARS_GL_FUNCTIONS
