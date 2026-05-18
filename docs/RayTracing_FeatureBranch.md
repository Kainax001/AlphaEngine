# Ray Tracing 기능 브랜치 변경사항 문서

**브랜치**: `feature/ray-tracing`  
**비교 기준**: `main` 브랜치  
**작성일**: 2026-05-19

---

## 목차

1. [개요](#1-개요)
2. [요구사항](#2-요구사항)
3. [전체 데이터 흐름](#3-전체-데이터-흐름)
4. [새로 추가된 파일](#4-새로-추가된-파일)
   - 4.1 SSBO
   - 4.2 RenderProxy
   - 4.3 BVHBuilder
   - 4.4 BufferManager
   - 4.5 FullscreenQuad
5. [기존 파일 변경사항](#5-기존-파일-변경사항)
   - 5.1 Model (specular 맵 로드)
   - 5.2 CameraComponent::ToProxy()
   - 5.3 LightComponent::ToProxy()
   - 5.4 MeshComponent (삼각형 추출 + specular ID)
   - 5.5 World (RT 렌더 파이프라인)
6. [컴퓨트 셰이더 (Raytracer.comp)](#6-컴퓨트-셰이더-raytracercomp)
7. [알려진 한계 및 향후 개선사항](#7-알려진-한계-및-향후-개선사항)

---

## 1. 개요

`main` 브랜치는 순수 래스터화(Forward Rendering) 파이프라인만 지원한다.  
`feature/ray-tracing` 브랜치는 이에 더해 **OpenGL 4.3 Compute Shader 기반 레이 트레이싱** 렌더 경로를 추가한다.

| 항목 | main | feature/ray-tracing |
|------|------|---------------------|
| 렌더 방식 | Forward Rasterization | Rasterization / RayTracing / Hybrid |
| 광원 처리 | UBO (Lit.frag) | SSBO (Raytracer.comp, 동적 크기) |
| BVH | 없음 | CPU Midpoint-split 빌드, SSBO 업로드 |
| 텍스처 | Diffuse (슬롯 0) | Diffuse + Specular (중복 제거 후 슬롯 5-20) |
| GL 최소 버전 | 4.1 | **4.3** (SSBO, Compute Shader) |

활성화 방법 (Sandbox 예시):
```cpp
// World 초기화 이후 한 줄만 추가하면 RT 모드로 전환된다.
world->EnableRayTracing("assets/shaders/Raytracer.comp", 1280, 720);
```

---

## 2. 요구사항

`Vcpkg.json` 및 `window.json` 변경 사항:

```json
// window.json — GL 컨텍스트 버전을 4.3으로 올림
// (main은 4.1이었음)
{
  "gl_major": 4,
  "gl_minor": 3
}
```

SSBO와 Compute Shader는 OpenGL **4.3** 이상에서만 동작한다.  
NVIDIA 드라이버 기준 GL 4.3은 GTX 600 시리즈 이후 모두 지원한다.

---

## 3. 전체 데이터 흐름

```
[매 프레임 — RT 모드]

World::Render()
  │
  ├─ 카메라 / 라이트 수집 (항상)
  │
  ├─ [m_RTGeoDirty == true 일 때만]
  │    CollectSceneProxy()
  │      └─ MeshComponent::FillTriangles()   → 월드 공간 삼각형 배열 생성
  │    텍스처 ID 중복 제거 (diffuse / specular 독립)
  │    BVHBuilder::Build()                   → CPU에서 BVH 트리 구성
  │    BufferManager::Update()               → Triangle SSBO, BVH SSBO GPU 업로드
  │    m_CachedTriangles / BVHNodes 저장
  │
  ├─ [캐시 히트 시]
  │    BufferManager::UpdateDynamic()        → Camera SSBO, Light SSBO만 업로드
  │
  ├─ Diffuse 텍스처  → GL_TEXTURE5  ~ GL_TEXTURE12
  ├─ Specular 텍스처 → GL_TEXTURE13 ~ GL_TEXTURE20
  │
  ├─ glBindImageTexture(0, m_RTOutputTexture) → image2D u_Output (write-only)
  ├─ ComputeShader::Dispatch(width/16, height/16)
  │    └─ Raytracer.comp
  │         ├─ 픽셀별 월드 공간 레이 생성 (카메라 역행렬)
  │         ├─ TraverseBVH() → 최근접 삼각형 탐색
  │         ├─ Möller-Trumbore 교차 검사
  │         ├─ 법선 / UV 보간 + NaN 가드
  │         ├─ Diffuse + Specular 텍스처 샘플링
  │         ├─ Shade() — Blinn-Phong (Diffuse + Specular)
  │         └─ ACES Filmic 토네맵 + 감마 보정 → imageStore
  │
  └─ FullscreenQuad::Draw(m_RTOutputTexture) → 화면에 결과 blit
```

---

## 4. 새로 추가된 파일

### 4.1 SSBO
**파일**: `AlphaGraphic/include/AlphaGraphic/buffer/SSBO.h`  
**파일**: `AlphaGraphic/src/buffer/SSBO.cpp`

`main`에는 존재하지 않는다. UBO는 크기가 고정이지만, 삼각형/BVH 데이터는 모델마다 크기가 달라지므로 **동적 크기 조정**이 가능한 SSBO가 필요하다.

```cpp
// ── SSBO.h ────────────────────────────────────────────────────────────────
class SSBO
{
public:
    // size    : 초기 GPU 버퍼 크기 (bytes)
    // binding : GLSL 쪽 layout(std430, binding=N) 과 일치해야 한다
    SSBO(GLsizeiptr size, GLuint binding, GLenum usage = GL_DYNAMIC_DRAW);
    ~SSBO();

    void Bind()   const;   // glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID)
    void Unbind() const;

    // offset 위치부터 size 바이트를 data로 덮어쓴다 (glBufferSubData)
    void UpdateData(GLintptr offset, GLsizeiptr size, const void* data);

    // 현재 할당 크기와 다를 경우 glBufferData로 재할당한다.
    // [중요] newSize == m_Size 이면 재할당하지 않는다.
    //        이것이 핵심: LightSSBO는 라이트 개수만큼만 할당되어야
    //        GLSL의 Lights.lights.length()가 정확한 개수를 반환한다.
    //        <= 로 비교하면 라이트가 줄어도 재할당 안 됨 → 쓰레기 데이터 포함 위험.
    void Resize(GLsizeiptr newSize);

private:
    GLuint     m_ID      = 0;
    GLuint     m_Binding = 0;
    GLsizeiptr m_Size    = 0;
    GLenum     m_Usage   = GL_DYNAMIC_DRAW;
};
```

```cpp
// ── SSBO.cpp (Resize 핵심 구현) ────────────────────────────────────────────
void SSBO::Resize(GLsizeiptr newSize)
{
    // 크기가 같으면 재할당 생략 — 불필요한 GPU 스톨 방지
    if (newSize == m_Size) return;
    m_Size = newSize;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    // nullptr 전달: GPU 메모리만 확보하고 CPU 데이터는 바로 채우지 않는다
    glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, nullptr, m_Usage);
    // binding point 재등록 — glBufferData 후 반드시 필요
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
```

---

### 4.2 RenderProxy
**파일**: `AlphaGraphic/include/AlphaGraphic/core/RenderProxy.h`

`main`의 `SceneProxy`는 Camera / Mesh / Light 정보만 담는 가벼운 구조체였다.  
RT 브랜치에서는 **삼각형 배열, BVH 노드 배열, 텍스처 ID 목록**이 추가된다.

```cpp
// ── RenderProxy.h ─────────────────────────────────────────────────────────

// GPU std430 레이아웃: vec3은 패딩 이슈가 있으므로 모든 필드를 vec4로 선언.
// CPU 측 struct와 GLSL struct의 바이트 오프셋이 정확히 일치해야 한다.

struct TriangleProxy
{
    glm::vec4 v0, v1, v2;   // xyz = 월드 공간 정점 위치 (w = 미사용)
    glm::vec4 n0, n1, n2;   // xyz = 월드 공간 스무드 법선 (w = 미사용)
    glm::vec4 uv01;          // xy = uv0,  zw = uv1
    glm::vec4 uv2mat;
    // xy = uv2
    // z  = diffuse 텍스처 슬롯 인덱스 (중복 제거 후, 0~7)
    // w  = specular 텍스처 슬롯 인덱스 (없으면 -1.0)
};

struct BVHNodeProxy
{
    glm::vec4 aabbMin;   // xyz = AABB 최솟값
    glm::vec4 aabbMax;   // xyz = AABB 최댓값
    int left;
    // 내부 노드: 왼쪽 자식 인덱스
    // 리프 노드: -1
    int right;
    // 내부 노드: 오른쪽 자식 인덱스
    // 리프 노드: 해당 리프가 포함하는 삼각형 개수 (triCount)
    int triOffset;       // 리프 노드일 때 첫 번째 삼각형의 배열 인덱스
    int pad;
};

struct SceneProxy
{
    CameraProxy                camera;
    std::vector<MeshProxy>     meshes;
    std::vector<LightProxy>    lights;
    std::vector<TriangleProxy> triangles;    // BVH 빌드 후 재정렬된 삼각형
    std::vector<BVHNodeProxy>  bvhNodes;     // 인덱스 0 = 루트
    std::vector<GLuint>        diffuseTexIDs;  // 메시별 diffuse GL 텍스처 ID
    std::vector<GLuint>        specularTexIDs; // 메시별 specular GL 텍스처 ID
};
```

---

### 4.3 BVHBuilder
**파일**: `AlphaGraphic/include/AlphaGraphic/core/BVHBuilder.h`  
**파일**: `AlphaGraphic/src/core/BVHBuilder.cpp`

삼각형을 공간 분할 트리로 구성해 레이-씬 교차 검사를 O(N) → O(log N)으로 줄인다.

```cpp
// ── BVHBuilder.h ───────────────────────────────────────────────────────────
class BVHBuilder
{
public:
    // tris 배열을 제자리 재정렬(std::partition)하면서 BVH를 구성한다.
    // 반환값: BVHNodeProxy 배열 (인덱스 0이 루트 노드)
    // 주의: tris의 순서가 변경된다. TriangleSSBO와 BVHNodeSSBO를 동시에 업로드해야 한다.
    static std::vector<BVHNodeProxy> Build(std::vector<TriangleProxy>& tris);

private:
    // [start, end) 범위의 삼각형으로 재귀적으로 서브트리를 구성한다.
    // 반환값: 이 노드의 nodes 배열 내 인덱스
    static int BuildRecursive(std::vector<BVHNodeProxy>& nodes,
                               std::vector<TriangleProxy>& tris,
                               int start, int end);
};
```

```cpp
// ── BVHBuilder.cpp (핵심 로직) ─────────────────────────────────────────────

static int BuildRecursive(std::vector<BVHNodeProxy>& nodes,
                           std::vector<TriangleProxy>& tris,
                           int start, int end)
{
    // 노드를 먼저 push_back해 인덱스를 예약한다.
    // 이후 자식 노드 빌드 중 nodes가 재할당될 수 있으므로
    // 레퍼런스 대신 인덱스(nodeIdx)로 접근해야 한다.
    int nodeIdx = static_cast<int>(nodes.size());
    nodes.push_back(BVHNodeProxy{});

    // 이 노드가 포함하는 모든 삼각형의 AABB를 계산한다
    glm::vec3 bMin( 1e30f), bMax(-1e30f);
    for (int i = start; i < end; i++)
    {
        glm::vec3 c = TriCentroid(tris[i]);
        bMin = glm::min(bMin, glm::min(tris[i].v0, glm::min(tris[i].v1, tris[i].v2)).xyz);
        bMax = glm::max(bMax, glm::max(tris[i].v0, glm::max(tris[i].v1, tris[i].v2)).xyz);
    }
    nodes[nodeIdx].aabbMin = glm::vec4(bMin, 0.0f);
    nodes[nodeIdx].aabbMax = glm::vec4(bMax, 0.0f);

    int count = end - start;

    if (count <= 4)  // 리프 조건: 삼각형이 4개 이하면 더 분할하지 않는다
    {
        nodes[nodeIdx].left      = -1;       // 리프임을 표시
        nodes[nodeIdx].right     = count;    // 삼각형 개수
        nodes[nodeIdx].triOffset = start;    // 삼각형 배열 시작 인덱스
        return nodeIdx;
    }

    // 가장 긴 축을 기준으로 공간 분할 (Midpoint Split)
    glm::vec3 extent = bMax - bMin;
    int axis = (extent.x > extent.y)
             ? (extent.x > extent.z ? 0 : 2)
             : (extent.y > extent.z ? 1 : 2);
    float mid = (bMin[axis] + bMax[axis]) * 0.5f;

    // std::partition: mid보다 작은 삼각형을 왼쪽으로 이동
    auto it = std::partition(tris.begin() + start, tris.begin() + end,
        [axis, mid](const TriangleProxy& t) {
            return TriCentroid(t)[axis] < mid;
        });
    int splitIdx = static_cast<int>(it - tris.begin());

    // 모든 삼각형이 한쪽에 몰릴 경우 강제로 절반 분할 (무한 재귀 방지)
    if (splitIdx == start || splitIdx == end)
        splitIdx = (start + end) / 2;

    // 재귀 빌드
    int leftChild  = BuildRecursive(nodes, tris, start,     splitIdx);
    int rightChild = BuildRecursive(nodes, tris, splitIdx,  end);

    // 주의: 재귀 호출 후 nodes가 재할당됐을 수 있으므로 인덱스로 접근
    nodes[nodeIdx].left      = leftChild;
    nodes[nodeIdx].right     = rightChild;
    nodes[nodeIdx].triOffset = -1;   // 내부 노드는 triOffset 미사용
    return nodeIdx;
}
```

---

### 4.4 BufferManager
**파일**: `AlphaGraphic/include/AlphaGraphic/core/BufferManager.h`  
**파일**: `AlphaGraphic/src/core/BufferManager.cpp`

Camera / Light / Triangle / BVH 네 가지 SSBO를 관리한다.

```cpp
// ── BufferManager.h ────────────────────────────────────────────────────────
class BufferManager
{
public:
    // GLSL 셰이더의 binding 번호와 반드시 일치해야 한다
    static constexpr GLuint CAMERA_BINDING   = 2;  // CameraBlock
    static constexpr GLuint LIGHT_BINDING    = 3;  // LightBlock
    static constexpr GLuint TRIANGLE_BINDING = 4;  // TriangleBlock
    static constexpr GLuint BVH_BINDING      = 5;  // BVHBlock

    BufferManager();

    // SceneProxy 전체를 GPU에 업로드한다 (씬 변경 시)
    void Update(const SceneProxy& proxy);

    // 카메라·라이트 SSBO만 업로드한다 (정적 씬의 매 프레임 경로)
    // FillTriangles / BVH 재빌드 없이 카메라 이동을 반영한다
    void UpdateDynamic(const CameraProxy& camera,
                       const std::vector<LightProxy>& lights);

    void BindAll()   const;  // 모든 SSBO를 binding point에 연결
    void UnbindAll() const;

private:
    std::unique_ptr<SSBO> m_CameraSSBO;    // sizeof(CameraProxy) = 4*mat4 + vec4
    std::unique_ptr<SSBO> m_LightSSBO;     // sizeof(LightProxy) * lightCount (동적)
    std::unique_ptr<SSBO> m_TriangleSSBO;  // sizeof(TriangleProxy) * triCount (동적)
    std::unique_ptr<SSBO> m_BVHNodeSSBO;   // sizeof(BVHNodeProxy) * nodeCount (동적)
};
```

```cpp
// ── BufferManager.cpp (생성자) ─────────────────────────────────────────────
BufferManager::BufferManager()
{
    m_CameraSSBO = std::make_unique<SSBO>(sizeof(CameraProxy), CAMERA_BINDING);

    // LightSSBO를 딱 1개 크기로 초기화한다.
    // 이유: Resize()는 크기가 다를 때만 재할당한다.
    //       MAX_LIGHTS(64) 크기로 시작하면 실제 라이트가 2개여도 length() = 64가 됨.
    //       → 초기화되지 않은 62개 라이트가 셰이더에 노출돼 NaN/흰색 문제 발생.
    //       1개로 시작 → Update() 시 실제 개수로 Resize() → length() == 실제 개수.
    m_LightSSBO    = std::make_unique<SSBO>(sizeof(LightProxy),    LIGHT_BINDING);
    m_TriangleSSBO = std::make_unique<SSBO>(sizeof(TriangleProxy), TRIANGLE_BINDING);
    m_BVHNodeSSBO  = std::make_unique<SSBO>(sizeof(BVHNodeProxy),  BVH_BINDING);
}
```

---

### 4.5 FullscreenQuad
**파일**: `AlphaGraphic/include/AlphaGraphic/core/FullscreenQuad.h`  
**파일**: `AlphaGraphic/src/core/FullscreenQuad.cpp`

Compute Shader가 출력한 `image2D` 텍스처를 화면 전체에 그린다.

```cpp
// ── FullscreenQuad.h ───────────────────────────────────────────────────────
class FullscreenQuad
{
public:
    FullscreenQuad();   // NDC [-1,1] 범위 쿼드 VAO/VBO 생성
    ~FullscreenQuad();

    // textureID: Compute Shader가 imageStore한 GL_RGBA32F 텍스처
    // 내부적으로 간단한 fullscreen.vert/frag 셰이더로 화면에 blit한다
    void Draw(GLuint textureID) const;

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    std::unique_ptr<Shader> m_Shader;
};
```

---

## 5. 기존 파일 변경사항

### 5.1 Model — specular 맵 로드
**파일**: `AlphaGraphic/src/scene/Model.cpp`  
**파일**: `AlphaGraphic/include/AlphaGraphic/scene/Model.h`

`main`은 `aiTextureType_DIFFUSE`만 읽는다.  
RT 브랜치는 `aiTextureType_SPECULAR`도 함께 로드해 메시별로 저장한다.

```cpp
// ── Model.h 변경 ───────────────────────────────────────────────────────────
// [추가] specular 텍스처 배열 및 접근자
const std::vector<std::shared_ptr<Texture2D>>& GetMeshSpecular() const
    { return m_MeshSpecular; }

// [추가] 멤버
std::vector<std::shared_ptr<Texture2D>> m_MeshSpecular; // 메시별 specular (null 가능)
```

```cpp
// ── Model.cpp ProcessMesh() 변경 ──────────────────────────────────────────
// [이전] diffuse만 로드
// [이후] diffuse + specular 동시 로드

std::shared_ptr<Texture2D> diffuseTex  = nullptr;
std::shared_ptr<Texture2D> specularTex = nullptr;

if (mesh->mMaterialIndex < scene->mNumMaterials)
{
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    // Assimp가 .mtl의 map_Kd (diffuse) 를 읽는다
    auto diffList = LoadMaterialTextures(mat, aiTextureType_DIFFUSE,  "diffuse");
    if (!diffList.empty())  diffuseTex  = diffList[0];

    // Assimp가 .mtl의 map_Ks (specular) 를 읽는다
    // specular 맵이 없는 메시는 null → GetSpecularTextureIDs()에서 ID=0 반환
    auto specList = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "specular");
    if (!specList.empty())  specularTex = specList[0];
}

m_MeshDiffuse.push_back(diffuseTex);
m_MeshSpecular.push_back(specularTex);  // [추가]
```

---

### 5.2 CameraComponent::ToProxy()
**파일**: `AlphaScene/src/CameraComponent.cpp`

RT 셰이더는 역행렬로 픽셀 → 월드 공간 레이를 재구성한다. `main`에는 이 메서드가 없다.

```cpp
// ── CameraComponent.cpp (추가) ────────────────────────────────────────────
AG::CameraProxy CameraComponent::ToProxy() const
{
    AG::CameraProxy proxy;

    proxy.view        = GetViewMatrix();
    proxy.projection  = GetProjectionMatrix(m_Aspect);

    // 역행렬은 셰이더에서 NDC → 뷰 공간, 뷰 → 월드 공간 변환에 사용된다.
    // glm::inverse()는 CPU에서 한 번만 계산 후 SSBO로 전달 → 셰이더에서 재계산 불필요.
    proxy.invView        = glm::inverse(proxy.view);
    proxy.invProjection  = glm::inverse(proxy.projection);

    // 레이 오리진 = 카메라 월드 위치
    proxy.position = glm::vec4(m_Camera.GetPosition(), 1.0f);

    return proxy;
}
```

---

### 5.3 LightComponent::ToProxy()
**파일**: `AlphaScene/src/LightComponent.cpp`

UBO는 Directional/Point/Spot을 고정 필드에 매핑한다.  
SSBO 방식은 타입을 `position.w`에 정수로 인코딩해 하나의 배열 원소로 통일한다.

```cpp
// ── LightComponent.cpp (추가) ─────────────────────────────────────────────
AG::LightProxy LightComponent::ToProxy() const
{
    AG::LightProxy proxy{};
    if (!m_Light) return proxy;

    proxy.color = glm::vec4(m_Light->GetColor(), 0.0f);

    switch (m_Light->GetType())
    {
        case AG::LightType::Directional:
        {
            auto* dl = static_cast<AG::DirectionalLight*>(m_Light.get());
            // position.w = 0.0 → 셰이더에서 int(L.position.w) == 0 → Directional
            proxy.position  = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            // direction.w = intensity (셰이더에서 L.direction.w로 읽음)
            proxy.direction = glm::vec4(dl->GetDirection(), m_Light->GetIntensity());
            break;
        }
        case AG::LightType::Point:
        {
            auto* pl = static_cast<AG::PointLight*>(m_Light.get());
            // position.w = 1.0 → Point
            proxy.position  = glm::vec4(m_Owner->GetTransform().GetPosition(), 1.0f);
            proxy.direction = glm::vec4(0.0f, 0.0f, 0.0f, m_Light->GetIntensity());
            proxy.attenuation = glm::vec4(
                pl->GetConstant(), pl->GetLinear(), pl->GetQuadratic(), 0.0f);
            break;
        }
        case AG::LightType::Spot:
        {
            auto* sl = static_cast<AG::SpotLight*>(m_Light.get());
            // position.w = 2.0 → Spot
            proxy.position  = glm::vec4(m_Owner->GetTransform().GetPosition(), 2.0f);
            proxy.direction = glm::vec4(sl->GetDirection(), m_Light->GetIntensity());
            // spotParams.x = cos(innerCutOff), spotParams.y = cos(outerCutOff)
            // 셰이더는 cosine 값으로 직접 비교하므로 라디안 → cos 변환을 CPU에서 수행
            proxy.spotParams = glm::vec4(
                glm::cos(glm::radians(sl->GetCutOff())),
                glm::cos(glm::radians(sl->GetOuterCutOff())),
                0.0f, 0.0f);
            break;
        }
    }
    return proxy;
}
```

---

### 5.4 MeshComponent — 삼각형 추출 및 specular ID
**파일**: `AlphaScene/src/MeshComponent.cpp`  
**파일**: `AlphaScene/include/AlphaScene/MeshComponent.h`

`main`에는 `FillTriangles()`가 없다. RT는 CPU에서 삼각형을 월드 공간으로 변환해야 BVH를 구성할 수 있다.

```cpp
// ── MeshComponent.cpp (추가) ──────────────────────────────────────────────
void MeshComponent::FillTriangles(std::vector<AG::TriangleProxy>& out,
                                   int matBase) const
{
    if (!m_Model) return;

    glm::mat4 model     = m_Owner->GetTransform().GetModelMatrix();
    // normalMatrix = transpose(inverse(M)) — 비균등 스케일 시 법선이 틀어지는 것을 보정
    glm::mat3 normalMat = glm::mat3(glm::inverseTranspose(glm::mat3(model)));

    const auto& meshes = m_Model->GetMeshes();
    for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
    {
        const auto& verts   = meshes[meshIdx]->GetVertices();
        const auto& indices = meshes[meshIdx]->GetIndices();

        // matBase + meshIdx = 이 Actor 내에서의 전역 메시 인덱스
        // (여러 Actor가 있을 때 각각의 메시 인덱스가 겹치지 않도록 matBase로 오프셋)
        float matIdxF = static_cast<float>(matBase + static_cast<int>(meshIdx));

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const auto& a = verts[indices[i]];
            const auto& b = verts[indices[i + 1]];
            const auto& c = verts[indices[i + 2]];

            AG::TriangleProxy tri;

            // 정점을 월드 공간으로 변환 (w=1 → 위치벡터)
            glm::vec4 wa = model * glm::vec4(a.Position, 1.0f);
            glm::vec4 wb = model * glm::vec4(b.Position, 1.0f);
            glm::vec4 wc = model * glm::vec4(c.Position, 1.0f);
            tri.v0 = glm::vec4(glm::vec3(wa), 0.0f);
            tri.v1 = glm::vec4(glm::vec3(wb), 0.0f);
            tri.v2 = glm::vec4(glm::vec3(wc), 0.0f);

            // 법선도 월드 공간으로 변환 (normalMat 사용, 정규화)
            tri.n0 = glm::vec4(glm::normalize(normalMat * a.Normal), 0.0f);
            tri.n1 = glm::vec4(glm::normalize(normalMat * b.Normal), 0.0f);
            tri.n2 = glm::vec4(glm::normalize(normalMat * c.Normal), 0.0f);

            tri.uv01   = glm::vec4(a.TexCoords, b.TexCoords);
            // uv2mat.z = 메시 인덱스 (World에서 diffuse/specular 슬롯으로 리맵됨)
            // uv2mat.w = 같은 메시 인덱스로 초기화, World에서 specular 슬롯으로 리맵됨
            tri.uv2mat = glm::vec4(c.TexCoords, matIdxF, matIdxF);

            out.push_back(tri);
        }
    }
}

// diffuse 텍스처 ID 목록 반환 (null 텍스처는 ID=0)
std::vector<GLuint> MeshComponent::GetDiffuseTextureIDs() const
{
    std::vector<GLuint> ids;
    if (!m_Model) return ids;
    for (const auto& tex : m_Model->GetMeshDiffuse())
        ids.push_back(tex ? tex->GetID() : 0u);
    return ids;
}

// [추가] specular 텍스처 ID 목록 반환
std::vector<GLuint> MeshComponent::GetSpecularTextureIDs() const
{
    std::vector<GLuint> ids;
    if (!m_Model) return ids;
    for (const auto& tex : m_Model->GetMeshSpecular())
        ids.push_back(tex ? tex->GetID() : 0u);  // specular 없으면 0
    return ids;
}
```

---

### 5.5 World — RT 렌더 파이프라인
**파일**: `AlphaScene/include/AlphaScene/World.h`  
**파일**: `AlphaScene/src/World.cpp`

`main`의 `Render()`는 Stage 3 래스터화만 수행한다.  
RT 브랜치는 `RenderMode`에 따라 래스터화 / RT / Hybrid를 선택한다.

#### World.h 추가 멤버

```cpp
// ── World.h (추가된 부분) ──────────────────────────────────────────────────

// 렌더 모드 열거형 (main에 없음)
enum class RenderMode {
    Rasterization,  // 기본 포워드 렌더링 (기존과 동일)
    RayTracing,     // Compute Shader 전용, 래스터화 완전 대체
    Hybrid,         // 래스터화 + RT 합성 (미래 확장용)
};

// RT 활성화 / 비활성화
void EnableRayTracing(const std::string& computePath,
                      int width, int height,
                      RenderMode mode = RenderMode::RayTracing);
void DisableRayTracing();

// private 멤버 (RT 관련)
RenderMode                             m_RenderMode     = RenderMode::Rasterization;
std::unique_ptr<AG::ComputeShader>     m_ComputeShader;
std::unique_ptr<AG::BufferManager>     m_BufferManager;
std::unique_ptr<AG::FullscreenQuad>    m_FullscreenQuad;
GLuint                                 m_RTOutputTexture = 0;
int                                    m_RTWidth = 0, m_RTHeight = 0;
std::vector<std::unique_ptr<AG::SSBO>> m_OwnedRTSSBOs;

// BVH 캐시 — 액터/메시가 변경될 때만 재빌드, 매 프레임 재빌드 금지
std::vector<AG::TriangleProxy>         m_CachedTriangles;
std::vector<AG::BVHNodeProxy>          m_CachedBVHNodes;
std::vector<GLuint>                    m_CachedDiffuseTexIDs;
std::vector<GLuint>                    m_CachedSpecularTexIDs;
bool                                   m_RTGeoDirty = true;
```

#### World.cpp — EnableRayTracing()

```cpp
// ── World.cpp EnableRayTracing() ──────────────────────────────────────────
void World::EnableRayTracing(const std::string& computePath,
                              int width, int height, RenderMode mode)
{
    m_RenderMode = mode;
    m_RTWidth    = width;
    m_RTHeight   = height;

    // 컴퓨트 셰이더 로드 (IsValid()로 컴파일 성공 여부 확인 가능)
    m_ComputeShader  = std::make_unique<AG::ComputeShader>(computePath);
    m_BufferManager  = std::make_unique<AG::BufferManager>();
    m_FullscreenQuad = std::make_unique<AG::FullscreenQuad>();

    // GL_RGBA32F: HDR 출력을 위해 채널당 32bit float 사용
    // GL_RGBA8이면 tonemapping 전 밝기 정보가 손실된다
    glGenTextures(1, &m_RTOutputTexture);
    glBindTexture(GL_TEXTURE_2D, m_RTOutputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
```

#### World.cpp — Render() RT 단계 (핵심 변경)

```cpp
// ── World.cpp Render() — Stage 3b RT 패스 ────────────────────────────────
// [main과의 차이]
// main: Stage 3에서 MeshComponent::Render()만 호출 (래스터화)
// RT  : Stage 3b에서 Compute Shader Dispatch + FullscreenQuad::Draw() 추가

if ((m_RenderMode == RenderMode::RayTracing || m_RenderMode == RenderMode::Hybrid)
    && m_ComputeShader && m_ComputeShader->IsValid()
    && m_RTOutputTexture != 0 && m_BufferManager)
{
    // ── (A) 씬 지오메트리가 변경됐을 때만 재빌드 ──────────────────────────
    // FlushPendingActors()에서 PendingAdd/Remove가 있을 때 m_RTGeoDirty = true
    if (m_RTGeoDirty || m_CachedTriangles.empty())
    {
        AG::SceneProxy proxy = CollectSceneProxy();
        // CollectSceneProxy()는 FillTriangles()를 호출하므로 비용이 크다.
        // 정적 씬에서는 첫 프레임에 한 번만 실행된다.

        // ── 텍스처 ID 중복 제거 ─────────────────────────────────────────
        // 배낭 모델의 경우 79개 메시가 약 5개 diffuse, 5개 specular 텍스처를 공유.
        // 중복을 제거하지 않으면 메시 인덱스 8~78이 샘플러 범위(0~7)를 초과해
        // 모두 fallback 색상(흰색/회색)으로 렌더링된다.
        {
            auto buildDedup = [](const std::vector<GLuint>& ids,
                                 std::unordered_map<GLuint,int>& slotMap,
                                 std::vector<GLuint>& unique)
            {
                for (GLuint id : ids)
                    if (slotMap.find(id) == slotMap.end())
                    {
                        slotMap[id] = static_cast<int>(unique.size());
                        unique.push_back(id);
                    }
            };

            // diffuse: ID=0 포함 (null → 슬롯 0, 셰이더 fallback이 처리)
            std::unordered_map<GLuint,int> diffSlot;
            std::vector<GLuint> uniqueDiff;
            buildDedup(proxy.diffuseTexIDs, diffSlot, uniqueDiff);

            // specular: ID=0 제외 → sentinel -1.0f 사용
            // ID=0인 메시를 슬롯에 포함시키면 바인딩되지 않은 텍스처 유닛에서
            // vec4(0,0,0,1)이 반환돼 specStrength=0 → 광택 없음.
            // 대신 셰이더에서 albedo 밝기 기반 기본값을 사용하도록 한다.
            std::unordered_map<GLuint,int> specSlot;
            std::vector<GLuint> uniqueSpec;
            for (GLuint id : proxy.specularTexIDs)
                if (id != 0 && specSlot.find(id) == specSlot.end())
                {
                    specSlot[id] = static_cast<int>(uniqueSpec.size());
                    uniqueSpec.push_back(id);
                }

            // 삼각형의 uv2mat.z/w를 중복 제거된 슬롯 인덱스로 리맵
            for (auto& tri : proxy.triangles)
            {
                int orig = static_cast<int>(tri.uv2mat.z);
                int nD   = static_cast<int>(proxy.diffuseTexIDs.size());
                int nS   = static_cast<int>(proxy.specularTexIDs.size());

                if (orig >= 0 && orig < nD)
                    tri.uv2mat.z = float(diffSlot.at(proxy.diffuseTexIDs[orig]));

                if (orig >= 0 && orig < nS)
                {
                    GLuint sid = proxy.specularTexIDs[orig];
                    // specular 없음 → -1.0 sentinel → 셰이더에서 albedo 기반 기본값
                    tri.uv2mat.w = (sid == 0) ? -1.0f
                                              : float(specSlot.at(sid));
                }
                else tri.uv2mat.w = -1.0f;
            }

            proxy.diffuseTexIDs  = std::move(uniqueDiff);
            proxy.specularTexIDs = std::move(uniqueSpec);
        }

        // ── BVH 빌드 ────────────────────────────────────────────────────
        if (!proxy.triangles.empty())
            proxy.bvhNodes = AG::BVHBuilder::Build(proxy.triangles);
        // Build()는 tris를 제자리 재정렬하므로 SSBO 업로드 전에 호출해야 한다.

        // ── GPU 업로드 (이동 전에 수행) ─────────────────────────────────
        m_BufferManager->Update(proxy);

        // ── 캐시 저장 ────────────────────────────────────────────────────
        m_CachedTriangles      = std::move(proxy.triangles);
        m_CachedBVHNodes       = std::move(proxy.bvhNodes);
        m_CachedDiffuseTexIDs  = std::move(proxy.diffuseTexIDs);
        m_CachedSpecularTexIDs = std::move(proxy.specularTexIDs);
        m_RTGeoDirty = false;
    }
    else
    {
        // ── 캐시 히트: 카메라 + 라이트만 업데이트 ─────────────────────
        // 삼각형 변환(FillTriangles) + BVH 재빌드 + 8.7MB SSBO 재업로드 생략
        // 정적 씬 기준으로 이 경로가 매 프레임 실행된다.
        AG::CameraProxy camProxy{};
        std::vector<AG::LightProxy> lightProxies;
        for (const auto& actor : m_Actors)
        {
            if (!actor->IsActive()) continue;
            if (auto* cam = actor->GetComponent<CameraComponent>())
                camProxy = cam->ToProxy();
            if (auto* light = actor->GetComponent<LightComponent>())
                lightProxies.push_back(light->ToProxy());
        }
        m_BufferManager->UpdateDynamic(camProxy, lightProxies);
    }

    // ── 텍스처 바인딩 ─────────────────────────────────────────────────────
    // SSBO binding(5)과 sampler2D binding은 네임스페이스가 분리돼 충돌 없음.
    // Diffuse  → 텍스처 유닛 5-12  (u_Tex0..7)
    // Specular → 텍스처 유닛 13-20 (u_SpecTex0..7)
    static constexpr int MAX_TEX = 8;
    int numDiff = std::min((int)m_CachedDiffuseTexIDs.size(),  MAX_TEX);
    int numSpec = std::min((int)m_CachedSpecularTexIDs.size(), MAX_TEX);
    for (int i = 0; i < numDiff; i++)
    {
        glActiveTexture(GL_TEXTURE5  + i);
        glBindTexture(GL_TEXTURE_2D, m_CachedDiffuseTexIDs[i]);
    }
    for (int i = 0; i < numSpec; i++)
    {
        glActiveTexture(GL_TEXTURE13 + i);
        glBindTexture(GL_TEXTURE_2D, m_CachedSpecularTexIDs[i]);
    }

    // ── Compute Shader 실행 ───────────────────────────────────────────────
    // glBindImageTexture: image2D(write-only, RGBA32F) — compute가 직접 픽셀 기록
    glBindImageTexture(0, m_RTOutputTexture, 0, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA32F);

    m_BufferManager->BindAll();

    m_ComputeShader->Use();
    // 샘플러 유니폼은 glUseProgram 이후에만 설정 가능
    m_ComputeShader->SetInt("u_Tex0", 5); // ... u_Tex7=12
    m_ComputeShader->SetInt("u_SpecTex0", 13); // ... u_SpecTex7=20

    // 로컬 그룹 16x16 → 픽셀 전체 커버를 위해 올림 나누기
    m_ComputeShader->Dispatch(
        static_cast<GLuint>((m_RTWidth  + 15) / 16),
        static_cast<GLuint>((m_RTHeight + 15) / 16));

    // MemoryBarrier: imageStore 완료 후 texture 샘플링이 가능하도록 동기화
    m_ComputeShader->MemoryBarrier();

    m_BufferManager->UnbindAll();

    // Compute 출력 텍스처를 화면 전체에 그린다
    m_FullscreenQuad->Draw(m_RTOutputTexture);
}
```

---

## 6. 컴퓨트 셰이더 (Raytracer.comp)

**파일**: `Sandbox/assets/shaders/Raytracer.comp`  
`main`에 존재하지 않는 완전히 새로운 파일이다.

```glsl
// ── Raytracer.comp ────────────────────────────────────────────────────────
#version 430 core
// OpenGL 4.3 필수 — Compute Shader는 4.3에서 코어로 도입됨

layout(local_size_x = 16, local_size_y = 16) in;
// 16x16 = 256 스레드/그룹.  warp 크기(32)의 배수이므로 NVIDIA에서 효율적.

// 출력 이미지 (binding=0) — Compute Shader가 직접 픽셀을 기록한다
layout(rgba32f, binding = 0) uniform image2D u_Output;

// ── 텍스처 ──────────────────────────────────────────────────────────────────
// layout(binding=N)을 쓰지 않고 glUniform1i로 명시적으로 유닛을 지정한다.
// 이유: 일부 드라이버에서 compute shader의 sampler layout binding이 무시됨.
uniform sampler2D u_Tex0;     // diffuse  슬롯 0 → GL_TEXTURE5
// ... u_Tex1~7 (GL_TEXTURE6~12)
uniform sampler2D u_SpecTex0; // specular 슬롯 0 → GL_TEXTURE13
// ... u_SpecTex1~7 (GL_TEXTURE14~20)

// ── SSBO 바인딩 ────────────────────────────────────────────────────────────
// SSBO binding 네임스페이스와 sampler2D 텍스처 유닛은 완전히 독립적.
// BVHBlock이 binding=5를 사용해도 u_Tex0의 유닛 5와 충돌하지 않는다.
layout(std430, binding = 2) readonly buffer CameraBlock   { ... } Camera;
layout(std430, binding = 3) readonly buffer LightBlock    { LightProxy lights[]; } Lights;
layout(std430, binding = 4) readonly buffer TriangleBlock { Triangle triangles[]; } Triangles;
layout(std430, binding = 5) readonly buffer BVHBlock      { BVHNode nodes[]; } BVH;

// ── Möller–Trumbore 레이-삼각형 교차 ─────────────────────────────────────
float HitTriangle(vec3 orig, vec3 dir, Triangle tri, out vec2 bary)
{
    vec3  e1 = tri.v1.xyz - tri.v0.xyz;
    vec3  e2 = tri.v2.xyz - tri.v0.xyz;
    vec3  h  = cross(dir, e2);
    float a  = dot(e1, h);
    if (abs(a) < 1e-7) return -1.0;  // 레이가 삼각형과 평행

    float f = 1.0 / a;
    vec3  s = orig - tri.v0.xyz;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return -1.0;

    vec3  q = cross(s, e1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0) return -1.0;

    float t = f * dot(e2, q);
    if (t < 0.001) return -1.0;  // 자기 교차 방지 (epsilon offset)

    bary = vec2(u, v);           // 무게중심 좌표 (u, v) → w = 1-u-v
    return t;
}

// ── BVH 스택 기반 순회 ────────────────────────────────────────────────────
// 재귀 대신 스택을 사용 (GLSL은 재귀 불가)
void TraverseBVH(vec3 orig, vec3 dir,
                 out float tMin, out int hitIdx, out vec2 hitBary)
{
    tMin   = 1e20; hitIdx = -1; hitBary = vec2(0.0);

    // 슬랩 테스트에 사용할 역방향 벡터 (0으로 나누기 방지)
    vec3 invDir = vec3(
        abs(dir.x) > 1e-8 ? 1.0/dir.x : 1e20,
        abs(dir.y) > 1e-8 ? 1.0/dir.y : 1e20,
        abs(dir.z) > 1e-8 ? 1.0/dir.z : 1e20);

    int stack[64]; int top = 0;
    stack[top++] = 0; // 루트 노드부터 탐색

    while (top > 0)
    {
        int idx = stack[--top];
        BVHNode node = BVH.nodes[idx];

        // AABB 슬랩 테스트 — 레이가 이 노드의 바운딩 박스를 지나가는지 확인
        vec3 t0 = (node.aabbMin.xyz - orig) * invDir;
        vec3 t1 = (node.aabbMax.xyz - orig) * invDir;
        float tEnter = max(max(min(t0,t1).x, min(t0,t1).y), min(t0,t1).z);
        float tExit  = min(min(max(t0,t1).x, max(t0,t1).y), max(t0,t1).z);
        // 박스 불통과 또는 이미 찾은 교차보다 멀면 스킵
        if (tEnter > tExit || tExit < 0.001 || tEnter > tMin) continue;

        if (node.left < 0)  // 리프 노드
        {
            for (int i = node.triOffset; i < node.triOffset + node.right; i++)
            {
                vec2 bary; float t = HitTriangle(orig, dir, Triangles.triangles[i], bary);
                if (t > 0.001 && t < tMin) { tMin = t; hitIdx = i; hitBary = bary; }
            }
        }
        else  // 내부 노드 — 두 자식을 스택에 push
        {
            if (top + 1 < 64) { stack[top++] = node.left; stack[top++] = node.right; }
        }
    }
}

// ── specular 맵 샘플링 ────────────────────────────────────────────────────
float SampleSpecular(int idx, vec2 uv)
{
    if (idx == 0) return texture(u_SpecTex0, uv).r;
    // ... idx 1~7
    return -1.0;  // 맵 없음 → 호출자가 albedo 기반 기본값 사용
}

// ── Shade() — Blinn-Phong 조명 계산 ─────────────────────────────────────
const float SHININESS = 64.0;

vec3 Shade(vec3 hitPos, vec3 normal, vec3 albedo, float specStrength)
{
    // viewDir: 히트 포인트 → 카메라 방향 (specular 계산용)
    vec3 viewDir = normalize(Camera.camPos.xyz - hitPos);
    vec3 result  = albedo * 0.12;  // ambient (diffuse의 12%)

    uint numLights = uint(Lights.lights.length());
    for (uint i = 0u; i < numLights; i++)
    {
        LightProxy L = Lights.lights[i];
        float intensity = L.direction.w;
        if (intensity < 0.001) continue;  // 초기화되지 않은 라이트 슬롯 스킵

        vec3  lightDir    = vec3(0.0, 1.0, 0.0);
        float attenuation = 1.0;

        if (int(L.position.w) == 0)       // Directional
        {
            vec3 d = L.direction.xyz;
            if (dot(d,d) < 1e-10) continue;
            lightDir = -normalize(d);
        }
        else if (int(L.position.w) == 1)  // Point
        {
            vec3 toLight = L.position.xyz - hitPos;
            float dist   = length(toLight);
            if (dist < 1e-5) continue;
            lightDir    = toLight / dist;
            // max(..., 1.0): 분모를 1 이상으로 보장 → attenuation <= 1.0
            // 이 보장이 없으면 c=0일 때 attenuation = ∞ → ∞*0 = NaN 위험
            attenuation = 1.0 / max(L.attenuation.x
                                  + L.attenuation.y * dist
                                  + L.attenuation.z * dist * dist, 1.0);
        }
        else                              // Spot
        {
            // Point와 동일하게 처리 후 스팟 팩터 곱산
            // eps == 0 → 하드 엣지, 0/0 NaN 방지
            float eps = L.spotParams.x - L.spotParams.y;
            float spotFactor = (abs(eps) < 1e-5)
                ? (dot(-lightDir, normalize(L.direction.xyz)) >= L.spotParams.x ? 1.0 : 0.0)
                : clamp((dot(-lightDir, normalize(L.direction.xyz))
                         - L.spotParams.y) / eps, 0.0, 1.0);
            attenuation *= spotFactor;
        }

        // diff <= 0 가드: 뒷면 처리 + Inf*0=NaN 방지 (attenuation이 클 때)
        float diff = dot(normal, lightDir);
        if (diff <= 0.0 || isnan(diff)) continue;

        vec3 lightColor = L.color.rgb * intensity * attenuation;

        result += albedo * lightColor * diff;  // Lambertian Diffuse

        // Blinn-Phong Specular: half vector = 라이트 방향과 시선 방향의 중간
        vec3  halfVec = normalize(lightDir + viewDir);
        float spec    = pow(max(dot(normal, halfVec), 0.0), SHININESS);
        result += lightColor * spec * specStrength;  // spec 강도는 텍스처 맵 또는 기본값
    }
    return result;
}

// ── main() ────────────────────────────────────────────────────────────────
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(u_Output);
    if (pixel.x >= size.x || pixel.y >= size.y) return;  // 이미지 경계 밖 스레드 제거

    // ── 픽셀 → 월드 공간 레이 생성 ────────────────────────────────────────
    vec2 uv  = (vec2(pixel) + 0.5) / vec2(size);  // 픽셀 중심 UV
    vec2 ndc = uv * 2.0 - 1.0;                    // NDC [-1, 1]

    // NDC → 뷰 공간 → 월드 공간 변환 (역행렬은 CPU에서 계산해 SSBO로 전달)
    vec4 rayClip  = vec4(ndc, -1.0, 1.0);
    vec4 rayView  = Camera.invProjection * rayClip;
    rayView       = vec4(rayView.xy, -1.0, 0.0);  // w=0: 방향벡터
    vec3 rayWorld = normalize((Camera.invView * rayView).xyz);
    vec3 rayOrig  = Camera.camPos.xyz;

    // ── BVH 순회 → 최근접 삼각형 ──────────────────────────────────────────
    float tMin; int hitIdx; vec2 hitBary;
    TraverseBVH(rayOrig, rayWorld, tMin, hitIdx, hitBary);

    vec3 color = vec3(0.05, 0.07, 0.12);  // 미스 색상 (배경 하늘)

    if (hitIdx >= 0)
    {
        Triangle tri = Triangles.triangles[hitIdx];
        float w      = 1.0 - hitBary.x - hitBary.y;  // 무게중심 w

        // ── 법선 보간 + NaN 가드 ────────────────────────────────────────
        // 퇴화 삼각형(면적=0)에서 법선이 vec3(0)이면 normalize() = NaN.
        // NaN이 dot()에 들어가면 조명 결과가 NaN → imageStore(NaN) → 검은 픽셀(NVIDIA).
        vec3 norm = w * tri.n0.xyz + hitBary.x * tri.n1.xyz + hitBary.y * tri.n2.xyz;
        float lenSq = dot(norm, norm);
        if (isnan(lenSq) || lenSq < 1e-10)
            norm = cross(tri.v1.xyz - tri.v0.xyz, tri.v2.xyz - tri.v0.xyz);
        norm = (dot(norm,norm) > 1e-10) ? normalize(norm) : vec3(0.0, 1.0, 0.0);

        // ── UV 보간 ────────────────────────────────────────────────────
        vec2 uvHit = w * tri.uv01.xy + hitBary.x * tri.uv01.zw + hitBary.y * tri.uv2mat.xy;
        if (isnan(uvHit.x) || isnan(uvHit.y)) uvHit = vec2(0.0);

        // ── 텍스처 샘플링 ──────────────────────────────────────────────
        int diffIdx = int(tri.uv2mat.z);   // diffuse 슬롯 (0~7)
        int specIdx = int(tri.uv2mat.w);   // specular 슬롯 (0~7) 또는 -1

        vec3 albedo = SampleDiffuse(diffIdx, uvHit).rgb;
        if (isnan(albedo.r) || isnan(albedo.g) || isnan(albedo.b)) albedo = vec3(0.8);

        float specStrength = SampleSpecular(specIdx, uvHit);
        if (specStrength < 0.0)
        {
            // specular 맵 없음 → albedo 밝기로 재질 추정
            // lum 높음(금속·나무·밝은 가죽) → specStrength 높음
            // lum 낮음(천·어두운 직물) → specStrength 낮음
            float lum = dot(albedo, vec3(0.299, 0.587, 0.114));
            specStrength = mix(0.08, 0.55, smoothstep(0.15, 0.75, lum));
        }

        vec3 hitPos = rayOrig + tMin * rayWorld;
        color = Shade(hitPos, norm, albedo, specStrength);
    }

    // ── NaN 최종 방어선 ────────────────────────────────────────────────────
    if (isnan(color.r) || isnan(color.g) || isnan(color.b)) color = vec3(0.0);

    // ── ACES Filmic 토네맵 ────────────────────────────────────────────────
    // Reinhard 대비 장점: 중간 톤 채도 보존, 하이라이트 압축이 더 자연스러움.
    // 언리얼 엔진 / 영화 파이프라인 표준 공식.
    const float EXPOSURE = 0.5;
    color *= EXPOSURE;  // 노출값 조절 (낮출수록 어두워짐)
    {
        const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
        color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    }

    // sRGB 감마 보정 (선형 공간 → 디스플레이)
    color = pow(color, vec3(1.0 / 2.2));

    imageStore(u_Output, pixel, vec4(color, 1.0));
}
```

---

## 7. 알려진 한계 및 향후 개선사항

| 한계 | 설명 | 개선 방향 |
|------|------|-----------|
| 그림자 없음 | 레이가 광원까지 차폐 검사를 하지 않는다 | Shadow Ray 추가 (2차 BVH 순회) |
| 반사/굴절 없음 | 단순 직접 조명만 계산 | Path Tracing 또는 반사 레이 |
| 텍스처 최대 8장 | diffuse/specular 각 8슬롯 제한 | `sampler2DArray` 또는 Bindless Texture |
| BVH 갱신 없음 | 액터 Transform이 바뀌어도 캐시 미갱신 | Transform dirty flag → `m_RTGeoDirty = true` |
| 단일 모델 메시 제한 | 여러 Actor에 MeshComponent가 있으면 matBase 오프셋으로 처리되나 텍스처 슬롯 경합 가능 | 씬 전체 텍스처 아틀라스 |
| 노멀 맵 미지원 | specular 맵은 지원하나 노멀 맵은 미반영 | `u_NormTex0..7` 추가, 탄젠트 공간 변환 |
| Hybrid 모드 미완성 | `RenderMode::Hybrid` 열거값은 존재하나 G-buffer 합성 미구현 | Deferred RT Reflection |
