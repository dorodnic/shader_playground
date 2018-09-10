#include <GL/gl3w.h>

#include "fbo.h"

fbo::fbo(int w, int h) : _w(w), _h(h)
{
    glGenFramebuffers(1, &_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void fbo::createTextureAttachment()
{
    _color_tex.set_options(false, false);
    _color_tex.upload(4, 8, _w, _h, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _color_tex.get(), 0);
}

void fbo::createDepthTextureAttachment()
{
    _depth_tex.set_options(true, false);
    _depth_tex.upload(1, 32, _w, _h, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depth_tex.get(), 0);
}

void fbo::bind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glViewport(0, 0, _w, _h);
}

void fbo::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void fbo::createDepthBufferAttachment()
{
    if (_db) glDeleteRenderbuffers(1, &_db);
    glGenRenderbuffers(1, &_db);
    glBindRenderbuffer(GL_RENDERBUFFER, _db);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _w, _h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _db);
}

fbo::~fbo()
{
    glDeleteRenderbuffers(1, &_db);
    glDeleteFramebuffers(1, &_id);
}