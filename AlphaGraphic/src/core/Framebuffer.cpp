#include "AlphaGraphic/core/Framebuffer.h"

#include <glad/glad.h>
#include <iostream>

namespace AG {

Framebuffer::Framebuffer(const FramebufferSpec& spec)
    : m_Spec(spec)
{
    Create();
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

// ==============================================================================
// Bind / Unbind
// ==============================================================================
void Framebuffer::Bind() const
{
    m_FBO.Bind();
    glViewport(0, 0, m_Spec.Width, m_Spec.Height);
}

void Framebuffer::Unbind() const
{
    m_FBO.Unbind();
}

// ==============================================================================
// Resize
// ==============================================================================
void Framebuffer::Resize(int width, int height)
{
    m_Spec.Width  = width;
    m_Spec.Height = height;
    Destroy();
    Create();
}

// ==============================================================================
// Getters
// ==============================================================================
unsigned int Framebuffer::GetColorAttachment(int index) const
{
    if (index < 0 || index >= (int)m_ColorAttachments.size())
    {
        std::cerr << "[AlphaGraphic] ERROR::FRAMEBUFFER::Invalid color attachment index: " << index << "\n";
        return 0;
    }
    return m_ColorAttachments[index];
}

// ==============================================================================
// Private
// ==============================================================================
void Framebuffer::Create()
{
    m_FBO.Bind();

    for (auto& attachment : m_Spec.Attachments)
    {
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        switch (attachment.Format)
        {
        case FramebufferTextureFormat::RGBA8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Spec.Width, m_Spec.Height,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)m_ColorAttachments.size(),
                                   GL_TEXTURE_2D, texID, 0);
            m_ColorAttachments.push_back(texID);
            break;

        case FramebufferTextureFormat::RedInteger:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_Spec.Width, m_Spec.Height,
                         0, GL_RED_INTEGER, GL_INT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (int)m_ColorAttachments.size(),
                                   GL_TEXTURE_2D, texID, 0);
            m_ColorAttachments.push_back(texID);
            break;

        case FramebufferTextureFormat::Depth24Stencil8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Spec.Width, m_Spec.Height,
                         0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, texID, 0);
            m_DepthAttachment = texID;
            break;

        default:
            break;
        }
    }

    if (m_ColorAttachments.size() > 1)
    {
        std::vector<unsigned int> drawBuffers;
        for (int i = 0; i < (int)m_ColorAttachments.size(); i++)
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffers((int)drawBuffers.size(), drawBuffers.data());
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[AlphaGraphic] ERROR::FRAMEBUFFER::Framebuffer not complete\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy()
{
    if (!m_ColorAttachments.empty())
    {
        glDeleteTextures((int)m_ColorAttachments.size(), m_ColorAttachments.data());
        m_ColorAttachments.clear();
    }
    if (m_DepthAttachment)
    {
        glDeleteTextures(1, &m_DepthAttachment);
        m_DepthAttachment = 0;
    }
    // FBO 삭제는 m_FBO 소멸자에 위임 — Resize 시 재사용하기 위해 여기서는 삭제하지 않음
}

} // namespace AG