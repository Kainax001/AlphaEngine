# Ray Tracing 브랜치 코드 변경사항 비교

> **대상 브랜치**: `main` → `feature/ray-tracing`  
> **변경 목적**: 기존 래스터화(Rasterization) 파이프라인을 유지하면서, OpenGL 4.3 Compute Shader 기반 Ray Tracing 파이프라인을 선택적으로 활성화할 수 있는 시스템 추가.

---

## 목차

1. [AlphaGraphic.h — 헤더 통합 파일](#1-alphagraphich--헤더-통합-파일)
2. [SSBO — 신규 버퍼 클래스](#2-ssbo--신규-버퍼-클래스)
3. [RenderProxy.h — 신규 데이터 전송 구조체](#3-renderproxyh--신규-데이터-전송-구조체)
4. [BVHBuilder — 신규 BVH 가속 구조](#4-bvhbuilder--신규-bvh-가속-구조)
5. [BufferManager — 신규 SSBO 관리자](#5-buffermanager--신규-ssbo-관리자)
6. [FullscreenQuad — 신규 풀스크린 렌더러](#6-fullscreenquad--신규-풀스크린-렌더러)
7. [StaticMesh — CPU 측 지오메트리 보존](#7-staticmesh--cpu-측-지오메트리-보존)
8. [Model — Specular 텍스처 로딩 추가](#8-model--specular-텍스처-로딩-추가)
9. [CameraComponent — ToProxy() 추가](#9-cameracomponent--toproxy-추가)
10. [LightComponent — ToProxy() 추가](#10-lightcomponent--toproxy-추가)
11. [MeshComponent — Ray Tracing 지원 메서드 추가](#11-meshcomponent--ray-tracing-지원-메서드-추가)
12. [World.h — RenderMode 및 RT 상태 추가](#12-worldh--rendermode-및-rt-상태-추가)
13. [World.cpp — RT 파이프라인 통합](#13-worldcpp--rt-파이프라인-통합)
14. [Sandbox.cpp — EnableRayTracing 호출](#14-sandboxcpp--enableraytracing-호출)
15. [Raytracer.comp — 신규 Compute Shader](#15-raytracercomp--신규-compute-shader)

---

## 1. AlphaGraphic.h — 헤더 통합 파일

### Before (main)
```cpp
// Core
#include "AlphaGraphic/core/Renderer.h"
#include "AlphaGraphic/core/Framebuffer.h"

// Buffer
#include "AlphaGraphic/buffer/VAO.h"
#include "AlphaGraphic/buffer/VBO.h"
#include "AlphaGraphic/buffer/EBO.h"
#include "AlphaGraphic/buffer/UBO.h"
#include "AlphaGraphic/buffer/FBO.h"
```

### After (feature/ray-tracing)
```cpp
// Core
#include "AlphaGraphic/core/Renderer.h"
#include "AlphaGraphic/core/Framebuffer.h"
#include "AlphaGraphic/core/RenderProxy.h"   // ← 추가: SceneProxy, CameraProxy 등
#include "AlphaGraphic/core/BVHBuilder.h"    // ← 추가: BVH 빌더
#include "AlphaGraphic/core/BufferManager.h" // ← 추가: SSBO 묶음 관리자
#include "AlphaGraphic/core/FullscreenQuad.h"// ← 추가: 풀스크린 VAO

// Buffer
#include "AlphaGraphic/buffer/VAO.h"
#include "AlphaGraphic/buffer/VBO.h"
#include "AlphaGraphic/buffer/EBO.h"
#include "AlphaGraphic/buffer/UBO.h"
#include "AlphaGraphic/buffer/SSBO.h"        // ← 추가: Shader Storage Buffer
#include "AlphaGraphic/buffer/FBO.h"
```

**변경 이유**: RT 파이프라인에 필요한 5개의 새 클래스를 퍼블릭 API에 노출. 사용자는 `#include <AlphaGraphic/AlphaGraphic.h>` 한 줄로 모든 기능을 사용할 수 있다.

---

## 2. SSBO — 신규 버퍼 클래스

### Before
없음 (신규 파일).

### After — `AlphaGraphic/buffer/SSBO.h`
```cpp
namespace AG {

// GL_SHADER_STORAGE_BUFFER 래퍼.
// UBO와 달리 크기 제한이 없고 GLSL 쪽에서 .length()로 원소 수를 읽을 수 있다.
class SSBO {
public:
    SSBO(GLsizeiptr size, GLuint binding);
    ~SSBO();

    // 현재 데이터를 통째로 교체한다.
    void UpdateData(const void* data, GLsizeiptr size);

    // size가 현재와 다를 때만 glBufferData(재할당) → glBufferSubData.
    // GLSL .length()가 정확한 원소 수를 반환하려면
    // 버퍼 크기 = sizeof(struct) * 실제 원소 수여야 한다.
    void Resize(GLsizeiptr newSize);

    void Bind()   const;
    void Unbind() const;

    GLuint     GetID()      const { return m_ID; }
    GLsizeiptr GetSize()    const { return m_Size; }
    GLuint     GetBinding() const { return m_Binding; }

private:
    GLuint     m_ID      = 0;
    GLsizeiptr m_Size    = 0;
    GLuint     m_Binding = 0;
};

} // namespace AG
```

### After — `AlphaGraphic/src/buffer/SSBO.cpp` (핵심 부분)
```cpp
void SSBO::Resize(GLsizeiptr newSize)
{
    // "==" 비교: 크기가 같으면 재할당 없이 반환.
    // "<=" 로 쓰면 LightSSBO가 1개→0개로 줄어도 재할당이 안 돼
    // GLSL .length()가 이전 값을 유지하는 버그 발생.
    if (newSize == m_Size) return;

    m_Size = newSize;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_Size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
```

**변경 이유**: UBO는 최대 크기가 65KB이고 GLSL에서 배열 길이를 알 수 없다. SSBO는 수십MB도 가능하고 `triangles.length()` 처럼 정확한 카운트를 읽을 수 있어 BVH 탐색 루프에 필수적이다.

---

## 3. RenderProxy.h — 신규 데이터 전송 구조체

### Before
없음 (신규 파일).

### After — `AlphaGraphic/core/RenderProxy.h`
```cpp
namespace AG {

// ── CameraProxy ────────────────────────────────────────────────────────────
// 카메라 행렬 4개 + 위치. SSBO binding=2.
// Compute Shader가 역행렬(invView, invProjection)로 NDC → 월드 공간 레이를 복원한다.
struct CameraProxy {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invView;           // 카메라 → 월드 변환
    glm::mat4 invProjection;     // 클립 → 뷰 변환
    glm::vec4 position;          // xyz = 월드 위치 (w = 1)
};

// ── LightProxy ─────────────────────────────────────────────────────────────
// 광원 하나를 std430으로 표현. 타입을 position.w에 인코딩한다.
// 래스터화의 LightData와 구조가 다른 이유: SSBO는 vec4만 허용(std430 정렬)
struct LightProxy {
    glm::vec4 position;    // xyz=위치,   w=타입(0=방향광, 1=점광, 2=스팟)
    glm::vec4 direction;   // xyz=방향,   w=강도(intensity)
    glm::vec4 color;       // rgb=색상,   w=미사용
    glm::vec4 attenuation; // x=상수, y=선형, z=이차 감쇠
    glm::vec4 spotParams;  // x=cosInner, y=cosOuter (스팟 전용)
};

// ── TriangleProxy ──────────────────────────────────────────────────────────
// 메시의 삼각형 하나. BVH 빌드 후 재정렬된다.
// vec3 대신 vec4를 쓰는 이유: std430 레이아웃은 vec3를 16바이트로 패딩하므로
// 명시적으로 vec4를 써야 C++ 구조체와 GPU 레이아웃이 일치한다.
struct TriangleProxy {
    glm::vec4 v0, v1, v2;   // 월드 공간 정점 위치
    glm::vec4 n0, n1, n2;   // 월드 공간 정점 법선
    glm::vec4 uv01;          // xy=uv0, zw=uv1
    glm::vec4 uv2mat;        // xy=uv2, z=diffIdx(float), w=specIdx(float)
                              //   specIdx == -1.0 → 스팩큘러 맵 없음(센티넬)
};

// ── BVHNodeProxy ───────────────────────────────────────────────────────────
// BVH 내부/리프 노드. GPU가 읽는 순서 그대로 SSBO에 업로드된다.
struct BVHNodeProxy {
    glm::vec4 aabbMin;  // xyz = AABB 최솟값
    glm::vec4 aabbMax;  // xyz = AABB 최댓값
    int  left;          // 내부: 왼쪽 자식 인덱스 / 리프: -1
    int  right;         // 내부: 오른쪽 자식 인덱스 / 리프: 삼각형 수(triCount)
    int  triOffset;     // 리프의 첫 번째 삼각형 인덱스
    int  pad;           // std430 16바이트 정렬 패딩
};

// ── MeshProxy ──────────────────────────────────────────────────────────────
struct MeshProxy {
    glm::mat4 modelMatrix;
    glm::mat4 normalMatrix;  // = transpose(inverse(modelMatrix))
    glm::vec4 boundsCenter;  // xyz=중심, w=반지름(미사용)
};

// ── SceneProxy ─────────────────────────────────────────────────────────────
// CollectSceneProxy()가 한 프레임에 모든 컴포넌트를 순회해 채우는 컨테이너.
struct SceneProxy {
    CameraProxy              camera;
    std::vector<LightProxy>  lights;
    std::vector<MeshProxy>   meshes;
    std::vector<TriangleProxy> triangles; // BVH 빌드 전 원본 순서
    std::vector<BVHNodeProxy>  bvhNodes;
    std::vector<GLuint>      diffuseTexIDs;  // 메시 순서 그대로의 GL 텍스처 ID
    std::vector<GLuint>      specularTexIDs;
};

} // namespace AG
```

**변경 이유**: 래스터화는 각 컴포넌트가 직접 `shader.SetMat4()` 등을 호출했다. RT는 컴퓨트 셰이더가 GPU 메모리(SSBO)를 직접 읽으므로, CPU에서 모든 씬 데이터를 Proxy 구조체로 패키징해서 한 번에 업로드해야 한다.

---

## 4. BVHBuilder — 신규 BVH 가속 구조

### Before
없음 (신규 파일).

### After — `AlphaGraphic/core/BVHBuilder.h`
```cpp
namespace AG {

class BVHBuilder {
public:
    // in/out: tris가 BVH 리프 순서로 in-place 재정렬된다.
    // 반환값: GPU에 업로드할 BVHNodeProxy 배열
    static std::vector<BVHNodeProxy> Build(std::vector<TriangleProxy>& tris);
};

} // namespace AG
```

### After — `AlphaGraphic/src/core/BVHBuilder.cpp` (핵심 부분)
```cpp
// Midpoint Split: 가장 긴 축을 따라 삼각형 중점 기준으로 반반 분할.
// 재귀 호출마다 nodes에 push_back하기 때문에
// 재귀 내에서 node를 참조(reference)로 들고 있으면 vector 재할당 시 댕글링.
// → node는 int nodeIdx로 들고, 재귀 후 nodes[nodeIdx]로 재접근.
static int BuildRecursive(std::vector<BVHNodeProxy>& nodes,
                           std::vector<TriangleProxy>& tris,
                           int start, int end)
{
    int nodeIdx = static_cast<int>(nodes.size());
    nodes.push_back(BVHNodeProxy{}); // push 먼저 — 이후 참조 금지

    // AABB 계산
    glm::vec3 bMin( 1e30f), bMax(-1e30f);
    for (int i = start; i < end; i++) {
        /* min/max of v0,v1,v2 */
    }

    int count = end - start;
    if (count <= 4) {  // 리프 조건: 삼각형 4개 이하
        nodes[nodeIdx].left      = -1;        // -1 → 리프 표시
        nodes[nodeIdx].right     = count;     // 삼각형 수
        nodes[nodeIdx].triOffset = start;
    } else {
        // 가장 긴 축으로 파티션
        glm::vec3 extent = bMax - bMin;
        int axis = (extent.x > extent.y && extent.x > extent.z) ? 0
                 : (extent.y > extent.z) ? 1 : 2;
        float mid = 0.5f * (bMin[axis] + bMax[axis]);

        // std::partition: 중점 < mid인 삼각형을 앞으로 재배치
        auto pivot = std::partition(tris.begin() + start, tris.begin() + end,
            [axis, mid](const TriangleProxy& t) {
                float c = (t.v0[axis] + t.v1[axis] + t.v2[axis]) / 3.0f;
                return c < mid;
            });
        int splitIdx = static_cast<int>(pivot - tris.begin());
        if (splitIdx == start || splitIdx == end) splitIdx = (start + end) / 2;

        int leftChild  = BuildRecursive(nodes, tris, start,    splitIdx);
        int rightChild = BuildRecursive(nodes, tris, splitIdx, end);
        nodes[nodeIdx].left  = leftChild;
        nodes[nodeIdx].right = rightChild;
    }
    return nodeIdx;
}
```

**변경 이유**: Ray Tracing에서 씬의 모든 삼각형을 순서대로 테스트하면 O(N) 교차 검사가 필요하다. BVH를 쓰면 평균 O(log N)으로 줄어든다. 배경팩 모델의 경우 약 67,907개 삼각형이 있고, BVH 없이는 각 픽셀마다 전부 테스트해야 한다.

---

## 5. BufferManager — 신규 SSBO 관리자

### Before
없음 (신규 파일).

### After — `AlphaGraphic/core/BufferManager.h`
```cpp
namespace AG {

// SSBO 4개(Camera/Light/Triangle/BVH)의 생명주기와 업로드를 관리한다.
class BufferManager {
public:
    BufferManager();

    // 전체 SceneProxy를 받아 4개 SSBO를 모두 갱신 (씬 변경 시 호출)
    void Update(const SceneProxy& proxy);

    // 카메라 + 조명만 갱신 (정적 씬에서 매 프레임 호출, 삼각형 재업로드 없음)
    void UpdateDynamic(const CameraProxy& cam,
                       const std::vector<LightProxy>& lights);

    void BindAll();   // 4개 SSBO를 각자의 binding point에 바인드
    void UnbindAll();

    void RegisterSSBO(const std::string& name, SSBO* ssbo);

private:
    // binding=2: 카메라 (단일 구조체 → 1 슬롯으로 초기화)
    std::unique_ptr<SSBO> m_CameraSSBO;
    // binding=3: 조명 (실제 조명 수로 Resize, GLSL .length() 정확성 보장)
    std::unique_ptr<SSBO> m_LightSSBO;
    // binding=4: 삼각형 (BVH 재정렬 후 전체 업로드)
    std::unique_ptr<SSBO> m_TriangleSSBO;
    // binding=5: BVH 노드
    std::unique_ptr<SSBO> m_BVHNodeSSBO;
};

} // namespace AG
```

### After — 핵심 구현 (BufferManager.cpp)
```cpp
void BufferManager::Update(const SceneProxy& proxy)
{
    // 카메라 — 항상 1개
    m_CameraSSBO->UpdateData(&proxy.camera, sizeof(CameraProxy));

    // 조명 — 실제 개수에 맞게 Resize (GLSL .length() 정확성)
    // 주의: 초기화는 sizeof(LightProxy)*1 (0바이트면 .length()=0 보장 안 됨)
    if (!proxy.lights.empty()) {
        GLsizeiptr sz = sizeof(LightProxy) * proxy.lights.size();
        m_LightSSBO->Resize(sz);
        m_LightSSBO->UpdateData(proxy.lights.data(), sz);
    }

    // 삼각형 — 8.7MB 업로드 (67,907개 × 128바이트)
    if (!proxy.triangles.empty()) {
        GLsizeiptr sz = sizeof(TriangleProxy) * proxy.triangles.size();
        m_TriangleSSBO->Resize(sz);
        m_TriangleSSBO->UpdateData(proxy.triangles.data(), sz);
    }

    // BVH 노드
    if (!proxy.bvhNodes.empty()) {
        GLsizeiptr sz = sizeof(BVHNodeProxy) * proxy.bvhNodes.size();
        m_BVHNodeSSBO->Resize(sz);
        m_BVHNodeSSBO->UpdateData(proxy.bvhNodes.data(), sz);
    }
}

// 정적 씬에서 카메라 이동 시 삼각형/BVH SSBO 재업로드를 건너뜀
void BufferManager::UpdateDynamic(const CameraProxy& cam,
                                   const std::vector<LightProxy>& lights)
{
    m_CameraSSBO->UpdateData(&cam, sizeof(CameraProxy));
    if (!lights.empty()) {
        GLsizeiptr sz = sizeof(LightProxy) * lights.size();
        m_LightSSBO->Resize(sz);
        m_LightSSBO->UpdateData(lights.data(), sz);
    }
    // Triangle/BVH SSBO는 건드리지 않음 → GPU에 이전 데이터 그대로 유지
}
```

**변경 이유**: 래스터화는 UBO 하나(LightBlock)만 매 프레임 업데이트하면 됐다. RT는 삼각형(8.7MB), BVH 노드, 카메라, 조명 등 4종류를 관리해야 한다. BufferManager는 이 SSBO들의 생명주기와 업로드 타이밍을 한 곳에서 제어한다.

---

## 6. FullscreenQuad — 신규 풀스크린 렌더러

### Before
없음 (신규 파일).

### After
```cpp
// Compute Shader가 출력 텍스처에 쓴 결과를 화면에 표시하기 위한
// 삼각형 2개 VAO + 패스스루 셰이더.
class FullscreenQuad {
public:
    FullscreenQuad();   // VAO/VBO 초기화, 내장 vs/fs 컴파일
    void Draw(GLuint texID);  // 텍스처를 바인드하고 glDrawArrays
};
```

**변경 이유**: 래스터화는 각 메시를 glDrawElements로 그렸다. RT는 Compute Shader가 출력 이미지(`GL_RGBA32F` 텍스처)에 픽셀을 직접 써넣으므로, 그 텍스처를 화면 전체에 표시하는 별도의 풀스크린 패스가 필요하다.

---

## 7. StaticMesh — CPU 측 지오메트리 보존

### Before (main) — `AlphaGraphic/include/AlphaGraphic/mesh/StaticMesh.h`
```cpp
class StaticMesh {
    // ...
private:
    std::unique_ptr<VAO> m_VAO;
    std::unique_ptr<VBO> m_VBO;
    std::unique_ptr<EBO> m_EBO;
    std::string          m_Path;
    // 정점/인덱스 데이터는 GPU 업로드 후 CPU에서 제거됨
};
```

### After (feature/ray-tracing)
```cpp
class StaticMesh {
public:
    // Ray Tracing에서 CPU 측 삼각형 데이터에 접근하기 위한 getter
    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    const std::vector<GLuint>& GetIndices()  const { return m_Indices; }

private:
    std::unique_ptr<VAO>  m_VAO;
    std::unique_ptr<VBO>  m_VBO;
    std::unique_ptr<EBO>  m_EBO;
    std::string           m_Path;

    // GPU 업로드 후에도 CPU에 원본 데이터를 유지한다.
    // MeshComponent::FillTriangles()가 월드 공간 삼각형을 생성할 때 사용.
    std::vector<Vertex>   m_Vertices;
    std::vector<GLuint>   m_Indices;
};
```

### After — `AlphaGraphic/src/mesh/StaticMesh.cpp`
```cpp
void StaticMesh::Setup(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
    // ← 추가: GPU 업로드 전에 CPU 복사본 보관
    m_Vertices    = vertices;
    m_Indices     = indices;

    m_VertexCount = (unsigned int)vertices.size();
    m_IndexCount  = (unsigned int)indices.size();
    // ... VAO/VBO/EBO 설정 ...
}
```

**변경 이유**: 래스터화는 CPU에서 삼각형 데이터가 필요 없으므로 GPU 업로드 후 버릴 수 있었다. RT는 `MeshComponent::FillTriangles()`에서 CPU 측 정점을 월드 공간으로 변환해 `TriangleProxy`를 생성해야 하므로, `m_Vertices`/`m_Indices`를 메모리에 유지한다. (메모리 비용: 배경팩 모델 기준 약 10MB 추가)

---

## 8. Model — Specular 텍스처 로딩 추가

### Before (main) — `AlphaGraphic/src/scene/Model.cpp`
```cpp
std::shared_ptr<StaticMesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    // ... 정점/인덱스 처리 ...

    // MTL에서 diffuse 텍스처 로딩
    std::shared_ptr<Texture2D> diffuseTex = nullptr;
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        auto textures = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "diffuse");
        if (!textures.empty())
            diffuseTex = textures[0];
    }
    m_MeshDiffuse.push_back(diffuseTex);
    // specular 로딩 없음

    return std::make_shared<StaticMesh>(vertices, indices);
}
```

### After (feature/ray-tracing)
```cpp
std::shared_ptr<StaticMesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    // ... 정점/인덱스 처리 ...

    // MTL에서 diffuse / specular 텍스처 로딩
    std::shared_ptr<Texture2D> diffuseTex  = nullptr;
    std::shared_ptr<Texture2D> specularTex = nullptr;
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        auto diffTextures = LoadMaterialTextures(mat, aiTextureType_DIFFUSE,  "diffuse");
        if (!diffTextures.empty())  diffuseTex  = diffTextures[0];

        // ← 추가: specular 맵 로딩 (OBJ의 .mtl 파일에서 map_Ks 항목)
        auto specTextures = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "specular");
        if (!specTextures.empty())  specularTex = specTextures[0];
    }
    m_MeshDiffuse.push_back(diffuseTex);
    m_MeshSpecular.push_back(specularTex); // ← 추가

    return std::make_shared<StaticMesh>(vertices, indices);
}
```

### 헤더 변경 — `AlphaGraphic/include/AlphaGraphic/scene/Model.h`
```cpp
// 추가된 getter 3개
const std::vector<std::shared_ptr<StaticMesh>>&  GetMeshes()       const { return m_Meshes; }
const std::vector<std::shared_ptr<Texture2D>>&   GetMeshDiffuse()  const { return m_MeshDiffuse; }
const std::vector<std::shared_ptr<Texture2D>>&   GetMeshSpecular() const { return m_MeshSpecular; } // ← 신규

// 추가된 멤버
std::vector<std::shared_ptr<Texture2D>> m_MeshSpecular; // ← 신규 (null 가능)
```

**변경 이유**: Blinn-Phong shading에서 금속/광택 표면의 스팩큘러 강도를 텍스처로 제어하기 위해. 래스터화 셰이더는 이미 specular를 지원했지만 Assimp에서 스팩큘러 텍스처를 로드하지 않았다. RT는 per-material 스팩큘러가 필수다.

---

## 9. CameraComponent — ToProxy() 추가

### Before (main) — `AlphaScene/src/CameraComponent.cpp`
```cpp
// ToProxy() 없음. 셰이더에는 UBO/Uniform으로 직접 행렬 전달.
```

### After (feature/ray-tracing)
```cpp
#include <glm/gtc/matrix_inverse.hpp> // ← 추가

AG::CameraProxy CameraComponent::ToProxy() const
{
    AG::CameraProxy proxy;
    proxy.view           = GetViewMatrix();
    proxy.projection     = GetProjectionMatrix(m_Aspect);
    // ← 역행렬: Compute Shader가 클립 → 뷰 → 월드 레이를 복원할 때 필요
    proxy.invView        = glm::inverse(proxy.view);
    proxy.invProjection  = glm::inverse(proxy.projection);
    proxy.position       = glm::vec4(m_Camera.GetPosition(), 1.0f);
    return proxy;
}
```

**변경 이유**: 래스터화 버텍스 셰이더는 월드→클립 방향(forward transform)만 필요하다. Ray Tracing은 픽셀 좌표(NDC) → 뷰 공간 → 월드 공간 레이 방향을 계산해야 하므로 역행렬이 필요하다.

---

## 10. LightComponent — ToProxy() 추가

### Before (main) — `AlphaScene/src/LightComponent.cpp`
```cpp
// GetLightData() 만 존재 — LightUBOData(std140)를 채우는 용도.
// 래스터화 UBO 형식: 타입별로 별도 vec4 슬롯(dirDirection, pointPosition 등)
```

### After (feature/ray-tracing)
```cpp
#include <glm/gtc/matrix_transform.hpp> // ← 추가

AG::LightProxy LightComponent::ToProxy() const
{
    AG::LightProxy proxy{};
    if (!m_Light) return proxy;

    glm::vec3 color     = m_Light->GetColor();
    float     intensity = m_Light->GetIntensity();
    proxy.color = glm::vec4(color, 0.0f);

    switch (m_Light->GetType())
    {
        case AG::LightType::Directional:
        {
            auto* dl = static_cast<AG::DirectionalLight*>(m_Light.get());
            // position.w = 0.0 → GLSL이 타입을 판별하는 센티넬 값
            proxy.position  = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            proxy.direction = glm::vec4(dl->GetDirection(), intensity);
            break;
        }
        case AG::LightType::Point:
        {
            auto* pl = static_cast<AG::PointLight*>(m_Light.get());
            glm::vec3 pos   = m_Owner->GetTransform().GetPosition();
            // position.w = 1.0 → 점광
            proxy.position  = glm::vec4(pos, 1.0f);
            proxy.direction = glm::vec4(0.0f, 0.0f, 0.0f, intensity);
            proxy.attenuation = glm::vec4(
                pl->GetConstant(), pl->GetLinear(), pl->GetQuadratic(), 0.0f);
            break;
        }
        case AG::LightType::Spot:
        {
            auto* sl = static_cast<AG::SpotLight*>(m_Light.get());
            glm::vec3 pos   = m_Owner->GetTransform().GetPosition();
            // position.w = 2.0 → 스팟광
            proxy.position  = glm::vec4(pos, 2.0f);
            proxy.direction = glm::vec4(sl->GetDirection(), intensity);
            float cosInner  = glm::cos(glm::radians(sl->GetCutOff()));
            float cosOuter  = glm::cos(glm::radians(sl->GetOuterCutOff()));
            proxy.spotParams = glm::vec4(cosInner, cosOuter, 0.0f, 0.0f);
            break;
        }
    }
    return proxy;
}
```

**변경 이유**: 래스터화의 `LightUBOData`는 타입별로 고정 슬롯을 가져서 DirectionalLight가 항상 `dirDirection` 슬롯에 들어갔다. SSBO 기반 `LightProxy`는 가변 길이 배열이므로 타입을 `position.w` 필드에 인코딩해야 GLSL 셰이더가 런타임에 분기할 수 있다.

---

## 11. MeshComponent — Ray Tracing 지원 메서드 추가

### Before (main) — `AlphaScene/include/AlphaScene/MeshComponent.h`
```cpp
// RT 관련 없음
void Render(const RenderContext& ctx);
const std::shared_ptr<AG::Model>& GetModel() const { return m_Model; }
```

### After (feature/ray-tracing)
```cpp
void Render(const RenderContext& ctx); // 래스터화 — 유지

// ← 신규: SceneProxy 수집용
AG::MeshProxy ToProxy() const;

// ← 신규: CPU 정점을 월드 공간 TriangleProxy로 변환해 out에 추가
// matBase: 씬 내 전체 메시 인덱스 오프셋(다중 메시 구분용)
void FillTriangles(std::vector<AG::TriangleProxy>& out, int matBase = 0) const;

// ← 신규: GL 텍스처 ID 조회 (World에서 중복 제거용)
std::vector<GLuint> GetDiffuseTextureIDs()  const;
std::vector<GLuint> GetSpecularTextureIDs() const;
```

### After — `AlphaScene/src/MeshComponent.cpp`
```cpp
void MeshComponent::FillTriangles(std::vector<AG::TriangleProxy>& out, int matBase) const
{
    if (!m_Model) return;

    glm::mat4 model     = m_Owner->GetTransform().GetModelMatrix();
    // 법선에는 모델 행렬의 역전치(3×3)를 적용해야 비균등 스케일에서도 정확하다
    glm::mat3 normalMat = glm::mat3(glm::inverseTranspose(glm::mat3(model)));

    const auto& meshes = m_Model->GetMeshes();
    for (size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++)
    {
        const auto& mesh    = meshes[meshIdx];
        const auto& verts   = mesh->GetVertices();   // CPU 보존 데이터
        const auto& indices = mesh->GetIndices();
        // 재질 인덱스: 씬 전역 오프셋 + 메시 로컬 인덱스
        float matIdxF = static_cast<float>(matBase + static_cast<int>(meshIdx));

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const AG::Vertex& a = verts[indices[i]];
            const AG::Vertex& b = verts[indices[i + 1]];
            const AG::Vertex& c = verts[indices[i + 2]];

            AG::TriangleProxy tri;

            // 로컬 → 월드 변환 (정점 위치)
            glm::vec4 wa = model * glm::vec4(a.Position, 1.0f);
            glm::vec4 wb = model * glm::vec4(b.Position, 1.0f);
            glm::vec4 wc = model * glm::vec4(c.Position, 1.0f);
            tri.v0 = glm::vec4(wa.x, wa.y, wa.z, 0.0f);
            tri.v1 = glm::vec4(wb.x, wb.y, wb.z, 0.0f);
            tri.v2 = glm::vec4(wc.x, wc.y, wc.z, 0.0f);

            // 법선 변환 (역전치 행렬)
            glm::vec3 na = glm::normalize(normalMat * a.Normal);
            glm::vec3 nb = glm::normalize(normalMat * b.Normal);
            glm::vec3 nc = glm::normalize(normalMat * c.Normal);
            tri.n0 = glm::vec4(na, 0.0f);
            tri.n1 = glm::vec4(nb, 0.0f);
            tri.n2 = glm::vec4(nc, 0.0f);

            tri.uv01   = glm::vec4(a.TexCoords, b.TexCoords);
            // uv2mat.z = diffuse 재질 인덱스 (World에서 중복 제거 후 최종 슬롯으로 덮어씀)
            // uv2mat.w = specular 재질 인덱스 (마찬가지, -1.0 = 맵 없음)
            tri.uv2mat = glm::vec4(c.TexCoords, matIdxF, matIdxF);

            out.push_back(tri);
        }
    }
}
```

**변경 이유**: 래스터화는 GPU(VAO)에 정점 데이터를 올려두고 `glDrawElements`를 호출한다. RT는 CPU에서 정점을 월드 공간으로 변환해 SSBO에 다시 업로드해야 한다 (Compute Shader는 VAO를 읽지 못함).

---

## 12. World.h — RenderMode 및 RT 상태 추가

### Before (main)
```cpp
namespace AS {

class World {
public:
    // 기존 API만 존재
    void EnableDefaultLighting(GLuint binding = 1);
    // RT 관련 API 없음
private:
    // RT 관련 멤버 없음
    struct LightUBOData { /* 9개 vec4 */ };
    struct SharedUBOEntry { /* ... */ };

    std::vector<std::shared_ptr<Actor>> m_Actors;
    // ...
    float m_TimeScale  = 1.0f;
    float m_FixedDT    = 1.0f / 60.0f;
    float m_FixedAccum = 0.0f;
};
```

### After (feature/ray-tracing)
```cpp
// RenderMode: 렌더 파이프라인 선택 enum
// World 클래스 외부에 선언 — Sandbox.cpp에서 AS::RenderMode::RayTracing으로 직접 참조
enum class RenderMode {
    Rasterization, // 기본: 기존 3단계 포워드 파이프라인
    RayTracing,    // Compute Shader 전용, 풀스크린 출력
    Hybrid,        // 래스터 + RT 합성 (미래 구현)
};

namespace AS {

class World {
public:
    // ← 신규 RT API
    void EnableRayTracing(const std::string& computePath,
                          int width, int height,
                          RenderMode mode = RenderMode::RayTracing);
    void DisableRayTracing();
    bool IsRayTracingEnabled() const { return m_RenderMode != RenderMode::Rasterization; }
    RenderMode GetRenderMode() const { return m_RenderMode; }

    // 외부에서 SSBO를 등록할 수 있는 확장 포인트
    AG::SSBO* CreateRayTracingSSBO(const std::string& name,
                                   GLsizeiptr size, GLuint binding);

private:
    AG::SceneProxy CollectSceneProxy() const; // ← 신규

    // RT 상태 —————————————————————————————————————————
    RenderMode m_RenderMode = RenderMode::Rasterization;

    std::unique_ptr<AG::ComputeShader>  m_ComputeShader;
    std::unique_ptr<AG::BufferManager>  m_BufferManager;
    std::unique_ptr<AG::FullscreenQuad> m_FullscreenQuad;

    GLuint m_RTOutputTexture = 0; // GL_RGBA32F 풀스크린 출력 텍스처
    int    m_RTWidth  = 0;
    int    m_RTHeight = 0;

    std::vector<std::unique_ptr<AG::SSBO>> m_OwnedRTSSBOs; // 외부 등록 SSBO

    // RT 지오메트리 캐시 — 씬 변경 시에만 재빌드
    std::vector<AG::TriangleProxy> m_CachedTriangles;
    std::vector<AG::BVHNodeProxy>  m_CachedBVHNodes;
    std::vector<GLuint>            m_CachedDiffuseTexIDs;
    std::vector<GLuint>            m_CachedSpecularTexIDs;
    bool                           m_RTGeoDirty = true; // dirty flag
};
```

**변경 이유**: `RenderMode` enum으로 래스터화/RT/하이브리드를 런타임에 전환할 수 있다. RT 상태(ComputeShader, BufferManager, 출력 텍스처)를 World가 소유함으로써 외부 코드는 `EnableRayTracing()` 한 줄로 전체 파이프라인을 활성화할 수 있다.

---

## 13. World.cpp — RT 파이프라인 통합

### Before — `Render()` (main)
```cpp
void World::Render()
{
    RenderContext ctx;

    // Stage 1: 카메라 + 조명 수집
    for (auto& actor : m_Actors)
    {
        if (!actor->IsActive()) continue;
        if (auto* cam = actor->GetComponent<CameraComponent>())
        {
            ctx.view       = cam->GetViewMatrix();
            ctx.projection = cam->GetProjectionMatrix(cam->GetAspect());
            ctx.viewPos    = actor->GetTransform().GetPosition();
        }
        if (auto* light = actor->GetComponent<LightComponent>())
            ctx.lights.push_back(light->GetLightData());
    }

    // Stage 2: LightBlock UBO 업데이트
    if (m_DefaultLightingEnabled)
        UpdateDefaultLightingUBO(ctx);

    // Stage 3: 메시 드로우 (UBO 자동 링크)
    for (auto& actor : m_Actors)
    {
        if (!actor->IsActive()) continue;
        if (auto* mesh = actor->GetComponent<MeshComponent>())
        {
            for (const auto& [blockName, entry] : m_SharedUBOs)
                mesh->RegisterUBO(blockName, entry.ubo.get(), entry.binding);
            mesh->Render(ctx);
        }
    }
}
```

### After — `Render()` (feature/ray-tracing)
```cpp
void World::Render()
{
    RenderContext ctx;

    // Stage 1: 카메라 + 조명 수집 (래스터화용 RenderContext 채움 — 유지)
    for (auto& actor : m_Actors) { /* ... 동일 ... */ }

    // Stage 2a: LightBlock UBO 업데이트 (래스터화용 — 유지)
    if (m_DefaultLightingEnabled)
        UpdateDefaultLightingUBO(ctx);

    // Stage 3a: 래스터화 패스 (RenderMode::Rasterization 또는 Hybrid일 때)
    if (m_RenderMode == RenderMode::Rasterization ||
        m_RenderMode == RenderMode::Hybrid)
    {
        for (auto& actor : m_Actors)
        {
            if (!actor->IsActive()) continue;
            if (auto* mesh = actor->GetComponent<MeshComponent>())
            {
                for (const auto& [blockName, entry] : m_SharedUBOs)
                    mesh->RegisterUBO(blockName, entry.ubo.get(), entry.binding);
                mesh->Render(ctx);
            }
        }
    }

    // Stage 3b: Ray Tracing 패스 (RenderMode::RayTracing 또는 Hybrid일 때)
    if ((m_RenderMode == RenderMode::RayTracing || m_RenderMode == RenderMode::Hybrid)
        && m_ComputeShader && m_ComputeShader->IsValid()
        && m_RTOutputTexture != 0 && m_BufferManager)
    {
        if (m_RTGeoDirty || m_CachedTriangles.empty())
        {
            // ── 전체 업데이트 (씬 변경 시, 첫 프레임) ───────────────────
            AG::SceneProxy proxy = CollectSceneProxy();

            // 텍스처 중복 제거:
            // 79개 메시 × 재질 = 최대 79개 ID지만 실제 고유 텍스처는 8개 미만.
            // 슬롯 0~7만 Compute Shader가 지원하므로 ID→슬롯 매핑이 필수.
            {
                // diffuse: texID=0 포함 (검은색 fallback)
                // specular: texID=0 제외 → -1.0 센티넬 (없는 맵은 albedo로 대체)
                std::unordered_map<GLuint,int> diffSlot, specSlot;
                std::vector<GLuint> uniqueDiff, uniqueSpec;

                for (GLuint id : proxy.diffuseTexIDs)
                    if (diffSlot.find(id) == diffSlot.end())
                    { diffSlot[id] = uniqueDiff.size(); uniqueDiff.push_back(id); }

                for (GLuint id : proxy.specularTexIDs)
                    if (id != 0 && specSlot.find(id) == specSlot.end())
                    { specSlot[id] = uniqueSpec.size(); uniqueSpec.push_back(id); }

                // 삼각형의 matIdx를 원본 인덱스→고유 슬롯으로 재매핑
                for (auto& tri : proxy.triangles)
                {
                    int origIdx = int(tri.uv2mat.z);
                    tri.uv2mat.z = float(diffSlot.at(proxy.diffuseTexIDs[origIdx]));

                    GLuint sid = proxy.specularTexIDs[origIdx];
                    // -1.0: 셰이더에서 albedo 밝기 기반 기본값 사용
                    tri.uv2mat.w = (sid == 0) ? -1.0f : float(specSlot.at(sid));
                }
                proxy.diffuseTexIDs  = std::move(uniqueDiff);
                proxy.specularTexIDs = std::move(uniqueSpec);
            }

            // BVH 빌드 (삼각형 순서 in-place 재정렬)
            if (!proxy.triangles.empty())
                proxy.bvhNodes = AG::BVHBuilder::Build(proxy.triangles);

            // GPU 업로드 (8.7MB triangle + BVH + camera + lights)
            m_BufferManager->Update(proxy);

            // 캐시 저장 (다음 프레임 fast path 사용)
            m_CachedTriangles      = std::move(proxy.triangles);
            m_CachedBVHNodes       = std::move(proxy.bvhNodes);
            m_CachedDiffuseTexIDs  = std::move(proxy.diffuseTexIDs);
            m_CachedSpecularTexIDs = std::move(proxy.specularTexIDs);
            m_RTGeoDirty = false;
        }
        else
        {
            // ── 빠른 업데이트 (정적 씬, 카메라 이동만) ──────────────────
            // 삼각형/BVH SSBO는 건드리지 않음 → 매 프레임 8.7MB 절약
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

        m_BufferManager->BindAll();

        // diffuse 텍스처 → 유닛 5~12
        int numDiff = std::min((int)m_CachedDiffuseTexIDs.size(), 8);
        for (int i = 0; i < numDiff; i++)
        { glActiveTexture(GL_TEXTURE5 + i); glBindTexture(GL_TEXTURE_2D, m_CachedDiffuseTexIDs[i]); }

        // specular 텍스처 → 유닛 13~20
        int numSpec = std::min((int)m_CachedSpecularTexIDs.size(), 8);
        for (int i = 0; i < numSpec; i++)
        { glActiveTexture(GL_TEXTURE13 + i); glBindTexture(GL_TEXTURE_2D, m_CachedSpecularTexIDs[i]); }

        // 출력 이미지 바인딩 (image unit=0, GL_RGBA32F, write-only)
        glBindImageTexture(0, m_RTOutputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // Compute Shader 실행 (16×16 워크그룹)
        m_ComputeShader->Use();
        m_ComputeShader->SetInt("u_Tex0", 5);   /* 8개 diffuse 유닛 설정 */
        m_ComputeShader->SetInt("u_SpecTex0", 13); /* 8개 specular 유닛 설정 */
        // ... (u_Tex1~7, u_SpecTex1~7 설정 생략)
        m_ComputeShader->Dispatch(
            (m_RTWidth  + 15) / 16,
            (m_RTHeight + 15) / 16);
        m_ComputeShader->MemoryBarrier(); // imageStore 완료 대기

        m_BufferManager->UnbindAll();

        // 출력 텍스처를 화면 전체에 표시
        m_FullscreenQuad->Draw(m_RTOutputTexture);
    }
}
```

### `FlushPendingActors()` 변경
```cpp
void World::FlushPendingActors()
{
    // ← 추가: 액터 목록이 실제로 변경될 때만 dirty 플래그 설정
    // 매 프레임 무조건 true로 설정하면 BVH가 매 프레임 재빌드되어 60fps 불가능
    if (!m_PendingAdd.empty() || !m_PendingRemove.empty())
        m_RTGeoDirty = true;

    for (auto& actor : m_PendingAdd) { /* ... */ }
    // ...
}
```

### `EndPlay()` 변경
```cpp
void World::EndPlay()
{
    for (auto& actor : m_Actors) actor->EndPlay();
    m_Actors.clear();
    m_PendingAdd.clear();
    m_PendingRemove.clear();
    m_SharedUBOs.clear();
    m_DefaultLightingEnabled = false;
    // ← 추가: RT 캐시 정리
    m_OwnedRTSSBOs.clear();
    m_CachedTriangles.clear();
    m_CachedBVHNodes.clear();
    m_CachedDiffuseTexIDs.clear();
    m_CachedSpecularTexIDs.clear();
    m_RTGeoDirty = true;
}
```

**변경 이유 요약**:

| 변경 | 이유 |
|------|------|
| Stage 3a를 `if (Rasterization or Hybrid)` 로 래핑 | RT 모드에서 glDrawElements 호출 방지 |
| dirty 플래그 조건부 설정 | 매 프레임 BVH 재빌드(8.7MB 업로드) 방지 |
| fast path (`UpdateDynamic`) | 정적 씬에서 CPU→GPU 삼각형 전송 제거 |
| 텍스처 중복 제거 | 79메시 × 텍스처 → 8개 이하 고유 슬롯으로 압축 |
| -1.0 센티넬 | 스팩큘러 맵 없는 메시를 albedo 기반 기본값으로 대체 |

---

## 14. Sandbox.cpp — EnableRayTracing 호출

### Before (main)
```cpp
void Sandbox::OnWorldInit(AS::World* world)
{
    // ... 액터 설정 ...
    world->SaveScene(SANDBOX_ASSET_DIR "scenes/main.json");
    // RT 없음
}
```

### After (feature/ray-tracing)
```cpp
void Sandbox::OnWorldInit(AS::World* world)
{
    // ... 액터 설정 (동일) ...
    world->SaveScene(SANDBOX_ASSET_DIR "scenes/main.json");

    // ← 추가: Ray Tracing 활성화 (주석 처리 시 래스터화로 복귀)
    world->EnableRayTracing(
        SANDBOX_ASSET_DIR "shaders/Raytracer.comp",
        1280, 720,
        AS::RenderMode::RayTracing
    );
}
```

**변경 이유**: 사용자가 한 줄 주석 처리로 RT↔래스터화 전환. 기존 씬 설정 코드는 전혀 건드리지 않는다.

---

## 15. Raytracer.comp — 신규 Compute Shader

### Before
없음 (신규 파일). 래스터화는 `Lit.vert` / `Lit.frag` 를 사용.

### After — `Sandbox/assets/shaders/Raytracer.comp` (전체)

```glsl
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

// 출력 이미지 (image unit=0, GL_RGBA32F)
// imageStore()로 픽셀을 직접 씀
layout(rgba32f, binding = 0) uniform image2D u_Output;

// ─────────────────────────────────────────────────────────────────────────
// Diffuse 텍스처 (유닛 5~12)
// uniform sampler2D 로 선언 → World.cpp에서 glUniform1i로 유닛 번호 지정
// ─────────────────────────────────────────────────────────────────────────
uniform sampler2D u_Tex0;
uniform sampler2D u_Tex1;
// ... u_Tex2 ~ u_Tex7 (생략)

// Specular 텍스처 (유닛 13~20)
uniform sampler2D u_SpecTex0;
// ... u_SpecTex1 ~ u_SpecTex7 (생략)

// idx 범위 초과 시 기본 회색 반환
vec4 SampleDiffuse(int idx, vec2 uv)
{
    if (idx == 0) return texture(u_Tex0, uv);
    // ... idx 1~7
    return vec4(0.8, 0.8, 0.8, 1.0); // 폴백
}

// idx < 0: 스팩큘러 맵 없음 → 센티넬 -1.0 반환
// 호출자가 -1.0 체크 후 albedo 기반 기본값 사용
float SampleSpecular(int idx, vec2 uv)
{
    if (idx == 0) return texture(u_SpecTex0, uv).r;
    // ... idx 1~7
    return -1.0;
}

// ─────────────────────────────────────────────────────────────────────────
// Camera SSBO (binding=2)
// ─────────────────────────────────────────────────────────────────────────
layout(std430, binding = 2) readonly buffer CameraBlock
{
    mat4 view;
    mat4 projection;
    mat4 invView;       // 카메라 → 월드
    mat4 invProjection; // 클립 → 뷰
    vec4 camPos;
} Camera;

// ─────────────────────────────────────────────────────────────────────────
// Light SSBO (binding=3)
// lights.length() → 실제 조명 수 (SSBO는 동적 길이)
// ─────────────────────────────────────────────────────────────────────────
struct LightProxy {
    vec4 position;    // xyz=위치, w=타입(0=방향, 1=점, 2=스팟)
    vec4 direction;   // xyz=방향, w=강도
    vec4 color;
    vec4 attenuation; // x=상수, y=선형, z=이차
    vec4 spotParams;  // x=cosInner, y=cosOuter
};
layout(std430, binding = 3) readonly buffer LightBlock
{
    LightProxy lights[];
} Lights;

// ─────────────────────────────────────────────────────────────────────────
// Triangle SSBO (binding=4)
// BVH 빌드 후 재정렬된 월드 공간 삼각형
// ─────────────────────────────────────────────────────────────────────────
struct Triangle {
    vec4 v0, v1, v2;   // 정점 위치 (xyz)
    vec4 n0, n1, n2;   // 정점 법선 (xyz)
    vec4 uv01;          // xy=uv0, zw=uv1
    vec4 uv2mat;        // xy=uv2, z=diffIdx, w=specIdx
};
layout(std430, binding = 4) readonly buffer TriangleBlock
{
    Triangle triangles[];
} Triangles;

// ─────────────────────────────────────────────────────────────────────────
// BVH SSBO (binding=5)
// SSBO binding과 sampler2D 유닛은 별개의 네임스페이스 → 충돌 없음
// ─────────────────────────────────────────────────────────────────────────
struct BVHNode {
    vec4 aabbMin;
    vec4 aabbMax;
    int  left;      // 내부: 왼쪽 자식 / 리프: -1
    int  right;     // 내부: 오른쪽 자식 / 리프: 삼각형 수
    int  triOffset; // 리프: 첫 번째 삼각형 인덱스
    int  pad;
};
layout(std430, binding = 5) readonly buffer BVHBlock
{
    BVHNode nodes[];
} BVH;

// ─────────────────────────────────────────────────────────────────────────
// Möller–Trumbore 레이–삼각형 교차 검사
// ─────────────────────────────────────────────────────────────────────────
float HitTriangle(vec3 orig, vec3 dir, Triangle tri, out vec2 bary)
{
    vec3  e1 = tri.v1.xyz - tri.v0.xyz;
    vec3  e2 = tri.v2.xyz - tri.v0.xyz;
    vec3  h  = cross(dir, e2);
    float a  = dot(e1, h);
    if (abs(a) < 1e-7) return -1.0; // 레이가 삼각형과 평행

    float f = 1.0 / a;
    vec3  s = orig - tri.v0.xyz;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return -1.0;

    vec3  q = cross(s, e1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0) return -1.0;

    float t = f * dot(e2, q);
    // t < 0.001: 자기 교차(self-intersection) 방지
    if (t < 0.001) return -1.0;

    bary = vec2(u, v);
    return t;
}

// ─────────────────────────────────────────────────────────────────────────
// BVH 순회 (반복 스택 방식)
// ─────────────────────────────────────────────────────────────────────────
const int STACK_SIZE = 64;

void TraverseBVH(vec3 orig, vec3 dir,
                 out float tMin, out int hitIdx, out vec2 hitBary)
{
    tMin   = 1e20;
    hitIdx = -1;

    // 역수를 미리 계산해 슬랩 테스트 성능 향상
    vec3 invDir = vec3(
        abs(dir.x) > 1e-8 ? 1.0/dir.x : 1e20,
        abs(dir.y) > 1e-8 ? 1.0/dir.y : 1e20,
        abs(dir.z) > 1e-8 ? 1.0/dir.z : 1e20);

    int stack[STACK_SIZE];
    int top = 0;
    stack[top++] = 0; // 루트 노드

    while (top > 0)
    {
        int     idx  = stack[--top];
        BVHNode node = BVH.nodes[idx];

        // 슬랩 테스트 (AABB 교차 검사)
        vec3 t0 = (node.aabbMin.xyz - orig) * invDir;
        vec3 t1 = (node.aabbMax.xyz - orig) * invDir;
        float tEnter = max(max(min(t0.x,t1.x), min(t0.y,t1.y)), min(t0.z,t1.z));
        float tExit  = min(min(max(t0.x,t1.x), max(t0.y,t1.y)), max(t0.z,t1.z));
        if (tEnter > tExit || tExit < 0.001 || tEnter > tMin) continue;

        if (node.left < 0) // 리프
        {
            for (int i = node.triOffset; i < node.triOffset + node.right; i++)
            {
                vec2  bary;
                float t = HitTriangle(orig, dir, Triangles.triangles[i], bary);
                if (t > 0.001 && t < tMin)
                { tMin = t; hitIdx = i; hitBary = bary; }
            }
        }
        else // 내부 노드
        {
            stack[top++] = node.left;
            stack[top++] = node.right;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Blinn-Phong Shading (NaN 가드 포함)
// ─────────────────────────────────────────────────────────────────────────
const float SHININESS = 64.0;

vec3 Shade(vec3 hitPos, vec3 normal, vec3 albedo, float specStrength)
{
    vec3 viewDir = normalize(Camera.camPos.xyz - hitPos);
    vec3 result  = albedo * 0.12; // ambient

    for (uint i = 0u; i < uint(Lights.lights.length()); i++)
    {
        LightProxy L   = Lights.lights[i];
        int   ltype    = int(L.position.w);
        float intensity = L.direction.w;
        if (intensity < 0.001) continue;

        vec3  lightDir    = vec3(0.0, 1.0, 0.0);
        float attenuation = 1.0;

        if (ltype == 1 || ltype == 2) // Point / Spot
        {
            vec3  toLight = L.position.xyz - hitPos;
            float dist    = length(toLight);
            if (dist < 1e-5) continue;
            lightDir = toLight / dist;
            float c = L.attenuation.x, li = L.attenuation.y, q = L.attenuation.z;
            // max(..., 1.0): 감쇠가 1보다 커지지 않도록 (과노출 방지)
            attenuation = 1.0 / max(c + li*dist + q*dist*dist, 1.0);
        }
        else // Directional
        {
            vec3 d = L.direction.xyz;
            if (dot(d,d) < 1e-10) continue;
            lightDir = -normalize(d);
        }

        // Spot 추가 감쇠
        if (ltype == 2)
        {
            float cosTheta = dot(-lightDir, normalize(L.direction.xyz));
            float eps      = L.spotParams.x - L.spotParams.y;
            // abs(eps) < 1e-5: inner ≈ outer → 하드엣지(0/0 NaN 방지)
            float spotFactor = (abs(eps) < 1e-5)
                ? (cosTheta >= L.spotParams.x ? 1.0 : 0.0)
                : clamp((cosTheta - L.spotParams.y) / eps, 0.0, 1.0);
            attenuation *= spotFactor;
        }

        float diff = dot(normal, lightDir);
        // diff <= 0 또는 NaN → 기여 없음(뒷면, 영벡터 법선 등)
        if (diff <= 0.0 || isnan(diff)) continue;

        vec3 lightColor = L.color.rgb * intensity * attenuation;
        result += albedo * lightColor * diff;

        // Blinn-Phong specular
        vec3  halfVec = normalize(lightDir + viewDir);
        float spec    = pow(max(dot(normal, halfVec), 0.0), SHININESS);
        result += lightColor * spec * specStrength;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
// Main (픽셀당 1회 실행)
// ─────────────────────────────────────────────────────────────────────────
void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(u_Output);
    if (pixel.x >= size.x || pixel.y >= size.y) return; // 경계 외 스레드 조기 종료

    // ── 레이 생성 (NDC → 월드 공간) ────────────────────────────────────
    vec2 uv  = (vec2(pixel) + 0.5) / vec2(size); // [0,1] + 픽셀 중심
    vec2 ndc = uv * 2.0 - 1.0;                   // [-1,1]

    vec4 rayClip  = vec4(ndc, -1.0, 1.0);
    vec4 rayView  = Camera.invProjection * rayClip;
    rayView       = vec4(rayView.xy, -1.0, 0.0); // w=0: 방향벡터
    vec3 rayWorld = normalize((Camera.invView * rayView).xyz);
    vec3 rayOrig  = Camera.camPos.xyz;

    // ── BVH 탐색 ────────────────────────────────────────────────────────
    float tMin; int hitIdx; vec2 hitBary;
    TraverseBVH(rayOrig, rayWorld, tMin, hitIdx, hitBary);

    vec3 color = vec3(0.05, 0.07, 0.12); // 배경(하늘색)

    if (hitIdx >= 0)
    {
        Triangle tri = Triangles.triangles[hitIdx];

        // 무게중심 보간으로 법선 계산
        float w   = 1.0 - hitBary.x - hitBary.y;
        vec3 norm = w*tri.n0.xyz + hitBary.x*tri.n1.xyz + hitBary.y*tri.n2.xyz;
        float lenSq = dot(norm, norm);
        // 퇴화 법선(NaN, 영벡터) → 페이스 법선으로 폴백
        if (isnan(lenSq) || lenSq < 1e-10)
            norm = cross(tri.v1.xyz - tri.v0.xyz, tri.v2.xyz - tri.v0.xyz);
        norm = normalize(norm);

        // UV 보간
        vec2 uvHit = w*tri.uv01.xy + hitBary.x*tri.uv01.zw + hitBary.y*tri.uv2mat.xy;
        if (isnan(uvHit.x) || isnan(uvHit.y)) uvHit = vec2(0.0);

        int  diffIdx = int(tri.uv2mat.z);
        int  specIdx = int(tri.uv2mat.w); // -1 → 스팩큘러 맵 없음

        vec3 albedo = SampleDiffuse(diffIdx, uvHit).rgb;
        if (isnan(albedo.r) || isnan(albedo.g) || isnan(albedo.b))
            albedo = vec3(0.8); // NaN 안전망

        float specStrength = SampleSpecular(specIdx, uvHit);
        if (specStrength < 0.0)
        {
            // 스팩큘러 맵 없음 → albedo 밝기로 광택 추정
            // smoothstep(0.15, 0.75, lum): 어두운 천 → matte, 밝은 금속 → glossy
            float lum = dot(albedo, vec3(0.299, 0.587, 0.114));
            specStrength = mix(0.08, 0.55, smoothstep(0.15, 0.75, lum));
        }

        vec3 hitPos = rayOrig + tMin * rayWorld;
        color = Shade(hitPos, norm, albedo, specStrength);
    }

    // NaN 최종 안전망
    if (isnan(color.r) || isnan(color.g) || isnan(color.b))
        color = vec3(0.0);

    // ── 톤매핑 ──────────────────────────────────────────────────────────
    const float EXPOSURE = 0.5; // 낮을수록 어두움
    color *= EXPOSURE;

    // ACES Filmic: Reinhard보다 채도 보존이 좋고 하이라이트를 자연스럽게 압축
    {
        const float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
        color = clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
    }

    // 감마 보정 (선형 → sRGB)
    color = pow(color, vec3(1.0/2.2));

    imageStore(u_Output, pixel, vec4(color, 1.0));
}
```

**래스터화 셰이더(Lit.vert/Lit.frag)와의 핵심 차이**:

| 항목 | 래스터화 (Lit.vert / Lit.frag) | Ray Tracing (Raytracer.comp) |
|------|-------------------------------|------------------------------|
| 실행 단위 | 정점(vert) + 프래그먼트(frag) | 픽셀 1개 = 스레드 1개 |
| 정점 입력 | VAO (attribute location) | SSBO (직접 읽기) |
| 카메라 | `uniform mat4 u_View` 등 | SSBO CameraBlock (binding=2) |
| 조명 | UBO LightBlock (std140) | SSBO LightBlock (binding=3) |
| 교차 검사 | GPU 하드웨어 래스터라이저 | Möller–Trumbore + BVH 순회 |
| 텍스처 | `uniform sampler2D u_Diffuse` | `uniform sampler2D u_Tex0..7` |
| 출력 | `out vec4 FragColor` | `imageStore(u_Output, pixel, ...)` |
| 톤매핑 | 없음 (선형 출력) | ACES Filmic + 감마 보정 |

---

## 변경 파일 요약

| 파일 | 유형 | 변경 요약 |
|------|------|-----------|
| `AlphaGraphic/AlphaGraphic.h` | 수정 | RT 관련 헤더 5개 추가 |
| `AlphaGraphic/buffer/SSBO.h/cpp` | **신규** | GL_SHADER_STORAGE_BUFFER 래퍼 |
| `AlphaGraphic/core/RenderProxy.h` | **신규** | SceneProxy, CameraProxy 등 구조체 |
| `AlphaGraphic/core/BVHBuilder.h/cpp` | **신규** | Midpoint-split BVH 빌더 |
| `AlphaGraphic/core/BufferManager.h/cpp` | **신규** | SSBO 4개 묶음 관리 |
| `AlphaGraphic/core/FullscreenQuad.h/cpp` | **신규** | 출력 텍스처 풀스크린 블릿 |
| `AlphaGraphic/mesh/StaticMesh.h/cpp` | 수정 | CPU 측 정점/인덱스 보존 |
| `AlphaGraphic/scene/Model.h/cpp` | 수정 | Specular 텍스처 로딩 추가 |
| `AlphaScene/CameraComponent.h/cpp` | 수정 | `ToProxy()` 추가 (역행렬 포함) |
| `AlphaScene/LightComponent.h/cpp` | 수정 | `ToProxy()` 추가 (타입 인코딩) |
| `AlphaScene/MeshComponent.h/cpp` | 수정 | `FillTriangles()`, `GetXxxTextureIDs()` 추가 |
| `AlphaScene/World.h` | 수정 | `RenderMode` enum, RT 멤버 12개 추가 |
| `AlphaScene/World.cpp` | 수정 | RT 파이프라인 통합 (Stage 3b), dirty flag |
| `Sandbox/src/Sandbox.cpp` | 수정 | `EnableRayTracing()` 호출 추가 |
| `Sandbox/assets/shaders/Raytracer.comp` | **신규** | 381줄 Compute Shader 전체 |
