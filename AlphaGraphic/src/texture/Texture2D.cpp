#include "AlphaGraphic/texture/Texture2D.h"

#include <stb_image.h>
#include <iostream>

namespace AG {

Texture2D::Texture2D(const std::string& path, bool flipVertically)
    : m_Path(path)
{
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);

    if (!data)
    {
        std::cerr << "[AlphaGraphic] ERROR::TEXTURE2D::Failed to load: " << path << "\n";
        return;
    }

    GLenum internalFormat = GL_RGB;
    GLenum dataFormat     = GL_RGB;

    if (m_Channels == 4)
    {
        internalFormat = GL_RGBA8;
        dataFormat     = GL_RGBA;
    }
    else if (m_Channels == 3)
    {
        internalFormat = GL_RGB8;
        dataFormat     = GL_RGB;
    }
    else if (m_Channels == 1)
    {
        internalFormat = GL_R8;
        dataFormat     = GL_RED;
    }

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_ID);
}

void Texture2D::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture2D::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace AG