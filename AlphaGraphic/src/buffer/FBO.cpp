#include "AlphaGraphic/buffer/FBO.h"

namespace AG {

FBO::FBO()
{
    glGenFramebuffers(1, &m_ID);
}

FBO::~FBO()
{
    Delete();
}

void FBO::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
}

void FBO::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::Delete()
{
    if (m_ID != 0)
    {
        glDeleteFramebuffers(1, &m_ID);
        m_ID = 0;
    }
}

} // namespace AG
