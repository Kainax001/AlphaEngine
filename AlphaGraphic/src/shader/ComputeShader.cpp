#include "AlphaGraphic/shader/ComputeShader.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace AG {

ComputeShader::ComputeShader(const char* computePath)
{
    // std::string   computeCode;
    // std::ifstream cShaderFile;

    // cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    // try
    // {
    //     cShaderFile.open(computePath);
    //     std::stringstream cShaderStream;
    //     cShaderStream << cShaderFile.rdbuf();
    //     cShaderFile.close();
    //     computeCode = cShaderStream.str();
    // }
    // catch (std::ifstream::failure& e)
    // {
    //     std::cerr << "[AlphaGraphic] ERROR::COMPUTE_SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
    // }

    // const char* cShaderCode = computeCode.c_str();

    // unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    // glShaderSource(compute, 1, &cShaderCode, NULL);
    // glCompileShader(compute);
    // checkCompileErrors(compute, "COMPUTE");

    // ID = glCreateProgram();
    // glAttachShader(ID, compute);
    // glLinkProgram(ID);
    // checkCompileErrors(ID, "PROGRAM");

    // glDeleteShader(compute);

    std::cerr << "[AlphaGraphic] ComputeShader: OpenGL 4.3 needed\n";
}

void ComputeShader::Use() const
{
    // glUseProgram(ID);
}

void ComputeShader::Dispatch(unsigned int x, unsigned int y, unsigned int z) const
{
    // glDispatchCompute(x, y, z);
    // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void ComputeShader::SetInt(const std::string& name, int value) const
{
    // glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) const
{
    // glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::checkCompileErrors(unsigned int shader, const std::string& type)
{
    // GLint  success;
    // GLchar infoLog[1024];
    // ...
}

} // namespace AG