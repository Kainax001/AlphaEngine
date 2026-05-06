#pragma once

#include <glad/glad.h>  // GL 함수 공개 노출

// Core
#include "AlphaGraphic/core/Renderer.h"
#include "AlphaGraphic/core/Framebuffer.h"

// Buffer
#include "AlphaGraphic/buffer/VAO.h"
#include "AlphaGraphic/buffer/VBO.h"
#include "AlphaGraphic/buffer/EBO.h"
#include "AlphaGraphic/buffer/UBO.h"
#include "AlphaGraphic/buffer/FBO.h"

// Shader
#include "AlphaGraphic/shader/Shader.h"
#include "AlphaGraphic/shader/ComputeShader.h"
#include "AlphaGraphic/shader/ShaderLibrary.h"

// Texture
#include "AlphaGraphic/texture/Texture.h"
#include "AlphaGraphic/texture/Texture2D.h"
#include "AlphaGraphic/texture/TextureCubemap.h"
#include "AlphaGraphic/texture/Rendertexture.h"

// Mesh
#include "AlphaGraphic/mesh/Mesh.h"
#include "AlphaGraphic/mesh/StaticMesh.h"

// Scene
#include "AlphaGraphic/scene/light/Light.h"
#include "AlphaGraphic/scene/light/DirectionalLight.h"
#include "AlphaGraphic/scene/light/PointLight.h"
#include "AlphaGraphic/scene/light/SpotLight.h"
#include "AlphaGraphic/scene/Camera.h"
#include "AlphaGraphic/scene/Transform.h"
#include "AlphaGraphic/scene/Material.h"
#include "AlphaGraphic/scene/Model.h"