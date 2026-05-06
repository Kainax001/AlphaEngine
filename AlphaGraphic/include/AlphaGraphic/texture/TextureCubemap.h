#pragma once

#include "AlphaGraphic/Texture/Texture.h"
#include <array>

namespace AG {

class TextureCubemap : public Texture
{
public:
    // faces order: right, left, top, bottom, front, back
    TextureCubemap(const std::array<std::string, 6>& faces, bool flipVertically = false);
    ~TextureCubemap();

    void Bind(unsigned int slot = 0) const override;
    void Unbind()                    const override;

private:
    void LoadFace(unsigned int target, const std::string& path, bool flip);
};

} // namespace AG