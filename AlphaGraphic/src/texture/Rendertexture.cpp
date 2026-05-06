#include "AlphaGraphic/texture/Rendertexture.h"

#include <iostream>

namespace AG {

RenderTexture::RenderTexture(int width, int height, RenderTextureFormat format)
    : m_Format(format)
{
    m_Width  = width;
    m_Height = height;
    Create();
}

RenderTexture::~RenderTexture()
{
    Destroy();
}

// ==============================================================================
// Bind / Unbind 
// ==============================================================================
void RenderTexture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);

    GLenum target = (m_Format == RenderTextureFormat::Depth ||
                     m_Format == RenderTextureFormat::DepthStencil)
                    ? GL_TEXTURE_2D : GL_TEXTURE_2D;

    glBindTexture(target, m_ID);
}

void RenderTexture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ==============================================================================
// BindFramebuffer / UnbindFramebuffer 
// ==============================================================================
void RenderTexture::BindFramebuffer() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
}

void RenderTexture::UnbindFramebuffer() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ==============================================================================
// Resize
// ==============================================================================
void RenderTexture::Resize(int width, int height)
{
    m_Width  = width;
    m_Height = height;
    Destroy();
    Create();
}

// ==============================================================================
// Private
// ==============================================================================
void RenderTexture::Create()
{
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    if (m_Format == RenderTextureFormat::Depth)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ID, 0);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        GLenum internalFormat = (m_Format == RenderTextureFormat::RGBA) ? GL_RGBA8 : GL_RGB8;
        GLenum dataFormat     = (m_Format == RenderTextureFormat::RGBA) ? GL_RGBA  : GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0,
                     dataFormat, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ID, 0);

        // Depth/Stencil RBO
        glGenRenderbuffers(1, &m_RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);

        if (m_Format == RenderTextureFormat::DepthStencil)
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
        else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Width, m_Height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RBO);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[AlphaGraphic] ERROR::RENDER_TEXTURE::Framebuffer not complete\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderTexture::Destroy()
{
    if (m_RBO) glDeleteRenderbuffers(1, &m_RBO);
    if (m_ID)  glDeleteTextures(1, &m_ID);
    if (m_FBO) glDeleteFramebuffers(1, &m_FBO);
    m_RBO = m_ID = m_FBO = 0;
}

} // namespace AG