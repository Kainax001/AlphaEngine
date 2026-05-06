#pragma once

#include <vector>
#include "AlphaGraphic/buffer/FBO.h"

namespace AG {

enum class FramebufferTextureFormat
{
    None = 0,
    RGBA8,
    RedInteger,     
    Depth24Stencil8,
};

struct FramebufferAttachmentSpec
{
    FramebufferTextureFormat Format;
};

struct FramebufferSpec
{
    int Width, Height;
    std::vector<FramebufferAttachmentSpec> Attachments;
    int Samples = 1; // MSAA
};

class Framebuffer
{
public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();

    void Bind()   const;
    void Unbind() const;

    void Resize(int width, int height);

    unsigned int GetColorAttachment(int index = 0) const;
    unsigned int GetDepthAttachment()              const { return m_DepthAttachment; }

    const FramebufferSpec& GetSpec() const { return m_Spec; }

private:
    void Create();
    void Destroy();

    FBO                       m_FBO;
    std::vector<unsigned int> m_ColorAttachments;
    unsigned int              m_DepthAttachment = 0;
    FramebufferSpec           m_Spec;
};

} // namespace AG