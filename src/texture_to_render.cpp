#include <GL/glew.h>
#include <iostream>
#include "texture_to_render.h"
#include "debuggl.h"

TextureToRender::TextureToRender()
{
}

TextureToRender::~TextureToRender()
{
    if (fb_ < 0)
        return ;
    unbind();
    glDeleteFramebuffers(1, &fb_);
    glDeleteTextures(1, &tex_);
    glDeleteRenderbuffers(1, &dep_);
}

void TextureToRender::create(int width, int height)
{
    w_ = width;
    h_ = height;

    // Create framebuffer
    glGenFramebuffers(1, &fb_);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_);

    // Create texture
    CHECK_GL_ERROR(glGenTextures(1, &tex_));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, tex_));
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 768, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1280, 720, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    //CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    //CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    //CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, w_, h_, 0,GL_RGB, GL_UNSIGNED_BYTE, 0));

    // Create depth buffer
    //glGenRenderbuffers(1, &dep_);
    //glBindRenderbuffer(GL_RENDERBUFFER, dep_);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_, h_);

    // *NEW*
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dep_);
    //glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,tex_,0);
    //GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0 };
    //glDrawBuffers(1, DrawBuffers); 
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer object as render target" << std::endl;
    } else {
        std::cerr << "Framebuffer ready" << std::endl;
    }
    unbind();
}

void TextureToRender::bind()
{
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fb_));
}

void TextureToRender::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

