#pragma once

#include "AlphaGraphic/Texture/Texture.h"

namespace AG {

enum class RenderTextureFormat
{
    RGB,
    RGBA,
    Depth,
    DepthStencil,
};

class RenderTexture : public Texture
{
public:
    RenderTexture(int width, int height, RenderTextureFormat format = RenderTextureFormat::RGBA);
    ~RenderTexture();

    void Bind(unsigned int slot = 0) const override;
    void Unbind()                    const override;

    void BindFramebuffer()   const;
    void UnbindFramebuffer() const;

    void Resize(int width, int height);

    unsigned int        GetFBO()    const { return m_FBO; }
    RenderTextureFormat GetFormat() const { return m_Format; }

private:
    void Create();
    void Destroy();

    unsigned int        m_FBO    = 0;
    unsigned int        m_RBO    = 0;
    RenderTextureFormat m_Format;
};

} // namespace AG