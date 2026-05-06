#pragma once

#include <glad/glad.h>
#include <string>

namespace AG {

class Texture
{
public:
    virtual ~Texture() = default;

    virtual void Bind(unsigned int slot = 0) const = 0;
    virtual void Unbind()                    const = 0;

    unsigned int GetID()     const { return m_ID; }
    int          GetWidth()  const { return m_Width; }
    int          GetHeight() const { return m_Height; }

protected:
    unsigned int m_ID     = 0;
    int          m_Width  = 0;
    int          m_Height = 0;
};

} // namespace AG