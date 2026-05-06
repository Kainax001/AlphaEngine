#pragma once

#include <glad/glad.h>
#include <string>

namespace AG {

class ComputeShader
{
public:
    unsigned int m_ID;

    ComputeShader(const char* computePath);

    void Use()      const;
    void Dispatch(unsigned int x, unsigned int y, unsigned int z) const;

    void SetInt  (const std::string& name, int value)   const;
    void SetFloat(const std::string& name, float value) const;

private:
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

} // namespace AG