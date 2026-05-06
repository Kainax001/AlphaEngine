#include "AlphaGraphic/texture/TextureCubemap.h"

#include <stb_image.h>
#include <iostream>

namespace AG {

TextureCubemap::TextureCubemap(const std::array<std::string, 6>& faces, bool flipVertically)
{
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

    // right, left, top, bottom, front, back
    constexpr unsigned int targets[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < 6; i++)
        LoadFace(targets[i], faces[i], flipVertically);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

TextureCubemap::~TextureCubemap()
{
    glDeleteTextures(1, &m_ID);
}

void TextureCubemap::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

void TextureCubemap::Unbind() const
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubemap::LoadFace(unsigned int target, const std::string& path, bool flip)
{
    stbi_set_flip_vertically_on_load(flip);

    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &channels, 0);

    if (!data)
    {
        std::cerr << "[AlphaGraphic] ERROR::TEXTURE_CUBEMAP::Failed to load face: " << path << "\n";
        return;
    }

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(target, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

} // namespace AG