#pragma once

#include "AlphaGraphic/Texture/Texture.h"

namespace AG {

class Texture2D : public Texture
{
public:
    Texture2D(const std::string& path, bool flipVertically = true);
    ~Texture2D();

    void Bind(unsigned int slot = 0) const override;
    void Unbind()                    const override;

    int                GetChannels() const { return m_Channels; }
    const std::string& GetPath()     const { return m_Path; }

private:
    int         m_Channels = 0;
    std::string m_Path;
};

} // namespace AG