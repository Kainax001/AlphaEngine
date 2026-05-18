# Ray Tracing 브랜치 — 용어 및 개념 정리

**대상**: `feature/ray-tracing` 브랜치에서 새로 등장한 용어, 알고리즘, C++ 패턴, 작명 규칙

---

## 목차

1. [렌더링 개념](#1-렌더링-개념)
2. [OpenGL 버퍼 / 리소스](#2-opengl-버퍼--리소스)
3. [공간 자료구조 — BVH](#3-공간-자료구조--bvh)
4. [수학 / 알고리즘](#4-수학--알고리즘)
5. [셰이더 개념](#5-셰이더-개념)
6. [C++ 패턴 및 문법](#6-c-패턴-및-문법)
7. [작명 규칙 (Naming Convention)](#7-작명-규칙-naming-convention)
8. [Sentinel 값과 Magic Number](#8-sentinel-값과-magic-number)

---

## 1. 렌더링 개념

### Ray Tracing (레이 트레이싱)
래스터화(Rasterization)와 대비되는 렌더링 방식.  
래스터화는 "삼각형을 화면에 투영해서 픽셀을 채운다"이고,  
레이 트레이싱은 "픽셀에서 광선을 쏘아 씬과 교차점을 찾는다"는 역방향 접근이다.

```
래스터화: 삼각형 → 픽셀  (오브젝트 공간 → 화면 공간)
레이 트레이싱: 픽셀 → 씬  (화면 공간 → 월드 공간)
```

장점: 그림자, 반사, 굴절을 물리적으로 정확하게 표현 가능  
단점: 픽셀 수 × 씬 복잡도만큼의 연산이 필요 → BVH로 가속

---

### RenderMode 열거형
```cpp
enum class RenderMode {
    Rasterization,  // 기존 포워드 렌더링
    RayTracing,     // Compute Shader 전용
    Hybrid,         // 둘을 합성 (미구현)
};
```
`enum class`는 일반 `enum`과 달리 스코프가 강제된다.  
`RenderMode::RayTracing` 처럼 반드시 타입명을 붙여야 하므로  
`if (mode == 1)` 같은 실수를 컴파일 타임에 방지한다.

---

### SceneProxy / RenderProxy 패턴
`AlphaScene`(게임 로직 레이어)과 `AlphaGraphic`(GPU 레이어) 사이에서  
데이터를 전달하기 위한 **순수 데이터 구조체(POD)**.

```
AlphaScene                AlphaGraphic
  │                           │
  │  CameraProxy              │
  │  LightProxy    ─────────► │  BufferManager → SSBO → 셰이더
  │  TriangleProxy             │
  │  BVHNodeProxy              │
```

- OpenGL 핸들(GLuint)이나 스마트 포인터를 포함하지 않는다
- 레이어 간 의존성을 최소화해 테스트와 직렬화가 쉬워진다
- `Proxy` 접미사는 "대리자/복사본"을 뜻하는 디자인 패턴 용어

---

### Tonemapping (톤매핑)
HDR(High Dynamic Range) 선형 색상 값을 디스플레이가 표현할 수 있는 [0, 1] 범위로 압축하는 과정.

**Reinhard (이전 방식)**
```glsl
color = color / (color + vec3(1.0));
// 단점: 전체 채도가 균등하게 낮아진다 (파스텔톤화)
```

**ACES Filmic (현재)**
```glsl
// 언리얼 엔진, 영화 파이프라인 표준
color = clamp((color * (2.51*color + 0.03)) /
              (color * (2.43*color + 0.59) + 0.14), 0.0, 1.0);
// 중간 톤 채도를 유지하면서 하이라이트만 부드럽게 압축
```

---

### Gamma Correction (감마 보정)
모니터는 선형 밝기값을 그대로 출력하지 않고 `^2.2`로 어둡게 표시한다.  
셰이더가 계산한 선형 값을 `^(1/2.2)`로 보정해야 실제 눈에 맞게 보인다.

```glsl
color = pow(color, vec3(1.0 / 2.2));  // 선형 → sRGB
```

감마 보정 없이 렌더링하면 전체적으로 어둡고 탁해 보인다.

---

### Blinn-Phong Shading
퐁(Phong) 셰이딩의 최적화 버전. 반사 벡터 `reflect()` 대신 **하프 벡터**를 사용한다.

```glsl
// Phong: reflect(-lightDir, normal) 계산 필요
// Blinn-Phong: 라이트 방향과 시선 방향의 중간 벡터
vec3 halfVec = normalize(lightDir + viewDir);
float spec   = pow(max(dot(normal, halfVec), 0.0), shininess);
```

하프 벡터는 매 픽셀 계산이 Phong보다 저렴하고, 그레이징 앵글에서 더 자연스럽다.  
`shininess` 값이 높을수록 하이라이트가 작고 날카로워진다 (금속 = 128+, 플라스틱 = 32).

---

### Ambient / Diffuse / Specular
조명의 세 가지 구성 요소:

| 항목 | 설명 | 코드 |
|------|------|------|
| **Ambient** | 간접광 근사. 모든 방향에서 오는 배경 조명 | `albedo * 0.12` |
| **Diffuse** | 표면이 빛을 균등하게 산란시키는 성분 (Lambert) | `albedo * lightColor * max(dot(n,l), 0)` |
| **Specular** | 정반사 하이라이트. 시선 방향과 반사 방향이 일치할 때 강해짐 | `lightColor * pow(dot(n,h), shininess) * specStrength` |

---

## 2. OpenGL 버퍼 / 리소스

### SSBO (Shader Storage Buffer Object)
`GL_SHADER_STORAGE_BUFFER` 타입의 버퍼. OpenGL 4.3에서 도입.

| 비교 | UBO | SSBO |
|------|-----|------|
| 최대 크기 | ~64KB | 수백 MB (GPU VRAM 한도) |
| 크기 결정 | 컴파일 타임 고정 | 런타임 동적 조정 가능 |
| 쓰기 | 읽기 전용 | 읽기/쓰기 모두 가능 |
| 속도 | 빠름 | 상대적으로 느림 |
| 용도 | 행렬, 라이트 수 고정 데이터 | 삼각형 배열, BVH 노드 등 가변 데이터 |

GLSL에서의 선언:
```glsl
layout(std430, binding = 4) readonly buffer TriangleBlock {
    Triangle triangles[];  // 런타임 크기 — .length()로 개수 조회
} Triangles;
```

---

### std430 레이아웃
SSBO에서 사용하는 메모리 정렬 규칙.  
CPU의 `struct`와 GPU의 `buffer` 필드 오프셋이 일치해야 데이터가 깨지지 않는다.

```
std140 (UBO): vec3은 vec4 크기(16바이트)로 패딩됨 → vec3 사용 시 낭비
std430 (SSBO): vec3은 12바이트, 더 촘촘함 → 하지만 여전히 정렬 규칙 주의
```

이 브랜치에서 모든 `TriangleProxy`, `BVHNodeProxy` 필드가 `vec4`(16바이트 정렬)만 사용하는 이유:  
**`vec3`을 쓰면 CPU 구조체와 GPU 구조체의 패딩이 달라져서 데이터가 밀린다.**

```cpp
// ❌ 위험: CPU에서 12바이트, GPU std430에서 16바이트로 해석될 수 있음
struct Bad { glm::vec3 pos; float w; };

// ✅ 안전: CPU와 GPU 모두 16바이트
struct Good { glm::vec4 pos; };  // xy=pos.xy, z=pos.z, w=extra data
```

---

### Compute Shader
그래픽 파이프라인(정점 → 프래그먼트)과 무관하게 실행되는 범용 GPU 프로그램.  
OpenGL 4.3에서 도입. CUDA나 OpenCL과 유사하지만 OpenGL 컨텍스트 안에서 동작한다.

```glsl
layout(local_size_x = 16, local_size_y = 16) in;
// "작업 그룹" 1개가 16×16=256개의 스레드를 동시에 실행한다
// Dispatch(width/16, height/16) → 화면 전체 픽셀을 병렬 처리
```

**왜 16×16인가?**  
NVIDIA GPU의 warp 크기는 32 스레드. 256 = 32 × 8 → 낭비 없이 warp를 채운다.

---

### image2D vs sampler2D

| | `sampler2D` | `image2D` |
|---|-------------|-----------|
| 접근 방식 | `texture(sampler, uv)` — 필터링, 밉맵 적용 | `imageStore/imageLoad` — 직접 픽셀 읽기/쓰기 |
| 좌표 | 정규화 UV (0.0~1.0) | 정수 픽셀 좌표 |
| Compute 쓰기 | 불가 | 가능 |
| 용도 | 텍스처 샘플링 | RT 출력 버퍼, 후처리 |

```glsl
// Compute Shader가 출력을 직접 쓴다
layout(rgba32f, binding = 0) uniform image2D u_Output;
imageStore(u_Output, ivec2(px, py), vec4(color, 1.0));
```

---

### glBindImageTexture
Compute Shader가 텍스처를 `image2D`로 접근할 수 있게 바인딩하는 함수.  
`glActiveTexture + glBindTexture`(sampler용)와는 다른 바인딩 포인트를 사용한다.

```cpp
glBindImageTexture(
    0,                    // image binding unit (셰이더의 binding=0)
    m_RTOutputTexture,    // GL 텍스처 ID
    0,                    // mip level
    GL_FALSE,             // 레이어드 아님
    0,                    // layer
    GL_WRITE_ONLY,        // 쓰기 전용
    GL_RGBA32F            // 포맷 (셰이더 선언과 반드시 일치)
);
```

---

### glMemoryBarrier
Compute Shader의 `imageStore` 완료를 보장하고 이후 샘플링이 올바른 데이터를 읽도록 동기화한다.

```cpp
// Dispatch 후 — GPU 파이프라인이 imageStore를 완전히 완료할 때까지 다음 명령 지연
glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
// 이 호출 없이 FullscreenQuad::Draw()가 실행되면 이전 프레임 데이터를 읽을 수 있다
```

---

## 3. 공간 자료구조 — BVH

### AABB (Axis-Aligned Bounding Box)
좌표축에 정렬된 직육면체. 최솟점(min)과 최댓점(max) 두 벡터로 표현한다.  
"Axis-Aligned" = 회전 없이 X/Y/Z 축에 평행한 면만 존재.

```cpp
glm::vec3 aabbMin = { -1.0f, 0.0f, -1.0f };
glm::vec3 aabbMax = {  1.0f, 2.0f,  1.0f };
// 임의 방향 OBB(Oriented BB)보다 교차 검사가 훨씬 빠르다
```

---

### BVH (Bounding Volume Hierarchy)
삼각형들을 AABB로 묶어 계층적으로 분할한 이진 트리.  
레이가 부모 AABB에 맞지 않으면 자식 전체를 스킵 → O(N) → O(log N).

```
         [전체 씬 AABB]
        /              \
  [왼쪽 절반 AABB]  [오른쪽 절반 AABB]
   /        \            /          \
[4개 삼각형] [4개]    [4개]      [4개]
```

**노드 종류:**
- **내부 노드(Internal)**: 두 자식 인덱스(`left`, `right`)를 가짐
- **리프 노드(Leaf)**: `left == -1`, `right == triCount`, `triOffset == 시작 인덱스`

```cpp
struct BVHNodeProxy {
    glm::vec4 aabbMin, aabbMax;
    int left;      // 내부: 왼쪽 자식 idx  /  리프: -1
    int right;     // 내부: 오른쪽 자식 idx /  리프: 삼각형 개수
    int triOffset; // 리프: 첫 삼각형 배열 인덱스
    int pad;
};
```

---

### Midpoint Split
BVH 구성 전략 중 하나. 가장 긴 축의 중간점을 기준으로 삼각형을 둘로 나눈다.

```cpp
// 가장 긴 축 선택
glm::vec3 extent = bMax - bMin;
int axis = (extent.x > extent.y) ? (extent.x > extent.z ? 0 : 2)
                                  : (extent.y > extent.z ? 1 : 2);
float mid = (bMin[axis] + bMax[axis]) * 0.5f;

// std::partition: 조건을 만족하는 원소를 앞으로 이동 (정렬 아님, O(N))
std::partition(tris.begin() + start, tris.begin() + end,
    [axis, mid](const TriangleProxy& t) {
        return TriCentroid(t)[axis] < mid;
    });
```

다른 전략으로 SAH(Surface Area Heuristic)가 있으나 계산 비용이 높다.  
Midpoint Split은 구현이 단순하고 일반적인 씬에서 충분히 좋다.

---

### Slab Test (슬랩 테스트)
레이와 AABB의 교차를 검사하는 알고리즘. X/Y/Z 각 축의 두 평면(slab)에 대한 진입/탈출 거리를 구한다.

```glsl
vec3 t0 = (aabbMin - rayOrig) * invRayDir;  // 각 축 진입 거리
vec3 t1 = (aabbMax - rayOrig) * invRayDir;  // 각 축 탈출 거리

float tEnter = max(max(min(t0,t1).x, min(t0,t1).y), min(t0,t1).z);
float tExit  = min(min(max(t0,t1).x, max(t0,t1).y), max(t0,t1).z);

// tEnter > tExit: 박스를 통과하지 않음
// tExit < 0:      박스가 레이 뒤에 있음
bool hit = (tEnter <= tExit) && (tExit > 0.0);
```

`invRayDir` (역방향 벡터)를 미리 계산하는 이유: 나눗셈보다 곱셈이 GPU에서 빠르기 때문.

---

## 4. 수학 / 알고리즘

### Möller-Trumbore 알고리즘
레이-삼각형 교차 검사의 표준 알고리즘. 삼각형의 두 모서리 벡터와 레이 방향으로 연립방정식을 푼다.

```
레이: P = O + t*D
삼각형: P = (1-u-v)*V0 + u*V1 + v*V2
→ 연립방정식 풀기: t, u, v 계산
```

반환값의 의미:
- `t` : 레이 오리진에서 교차점까지의 거리 (t > 0이면 앞, t < 0이면 뒤)
- `u, v` : 무게중심 좌표 (삼각형 내 교차점 위치)
- `0 ≤ u, v, u+v ≤ 1` 조건을 만족해야 삼각형 내부

```glsl
float t = f * dot(e2, q);
if (t < 0.001) return -1.0;
// 0.001: 자기 교차 방지(self-intersection).
// 표면에서 출발한 레이가 같은 삼각형에 다시 맞는 것을 막는 epsilon
```

---

### Barycentric Coordinates (무게중심 좌표)
삼각형 내 임의의 점을 세 꼭짓점의 가중합으로 표현하는 좌표계.

```
P = w*V0 + u*V1 + v*V2   (w = 1 - u - v)
```

법선/UV 보간에 사용:
```glsl
float w   = 1.0 - hitBary.x - hitBary.y;  // 세 번째 가중치
vec3 norm = w * n0 + hitBary.x * n1 + hitBary.y * n2;
vec2 uvHit = w * uv0 + hitBary.x * uv1 + hitBary.y * uv2;
```

꼭짓점 속성(법선, UV, 색상)을 교차점에서 부드럽게 보간하는 것이 스무드 셰이딩의 핵심이다.

---

### Normal Matrix (법선 행렬)
모델 행렬로 법선을 변환하면 비균등 스케일 시 법선 방향이 틀어진다.  
올바른 법선 변환은 `transpose(inverse(modelMatrix))`의 3×3 부분을 사용해야 한다.

```cpp
// ❌ 비균등 스케일 시 틀림
vec3 worldNormal = vec3(model * vec4(localNormal, 0.0));

// ✅ 항상 올바름
glm::mat3 normalMat = glm::mat3(glm::inverseTranspose(glm::mat3(model)));
glm::vec3 worldNormal = glm::normalize(normalMat * localNormal);
```

균등 스케일(모든 축 배율 동일)일 때는 모델 행렬로 변환해도 결과가 같다.  
비균등 스케일(예: x만 2배 키움)일 때 차이가 발생한다.

---

### NDC (Normalized Device Coordinates)
클립 공간을 정규화한 좌표계. X, Y 모두 -1 ~ +1 범위.  
픽셀 좌표 → NDC → 뷰 공간 → 월드 공간 순서로 레이를 역변환한다.

```glsl
vec2 uv  = (vec2(pixel) + 0.5) / vec2(imageSize);  // 0~1
vec2 ndc = uv * 2.0 - 1.0;                         // -1~1

// NDC → 클립 → 뷰 → 월드
vec4 rayClip  = vec4(ndc, -1.0, 1.0);              // w=1: 위치벡터
vec4 rayView  = Camera.invProjection * rayClip;
rayView       = vec4(rayView.xy, -1.0, 0.0);        // w=0: 방향벡터
vec3 rayWorld = normalize((Camera.invView * rayView).xyz);
```

`+ 0.5`를 더하는 이유: 픽셀 좌표는 정수(좌측 상단 코너)이므로 픽셀 **중심**을 샘플링하기 위해 0.5를 더한다.

---

### Dirty Flag 패턴
매 프레임 불필요한 재연산을 막기 위한 캐시 무효화 전략.

```cpp
bool m_RTGeoDirty = true;  // "이 데이터는 재계산이 필요하다"

// 데이터가 바뀔 때만 true 설정
void FlushPendingActors() {
    if (!m_PendingAdd.empty() || !m_PendingRemove.empty())
        m_RTGeoDirty = true;  // 씬이 변했으니 BVH 재빌드 필요
}

// 소비 측에서 재계산 후 false로 초기화
if (m_RTGeoDirty) {
    // BVH 재빌드...
    m_RTGeoDirty = false;
}
```

이 패턴 없이는 60fps에서 BVH를 초당 60번 빌드 → 성능 파괴.

---

## 5. 셰이더 개념

### Texture Unit vs Image Unit vs SSBO Binding
세 가지는 완전히 독립된 바인딩 네임스페이스다.

```
텍스처 유닛 (GL_TEXTURE0 ~ GL_TEXTURE31):  sampler2D 용
Image   유닛 (binding=0, 1, 2...):          image2D 용 (읽기/쓰기)
SSBO    바인딩 (binding=0, 1, 2...):         buffer 블록 용
```

```glsl
layout(rgba32f, binding = 0) uniform image2D u_Output;   // image unit 0
layout(std430, binding = 5) readonly buffer BVHBlock {};  // SSBO binding 5
uniform sampler2D u_Tex0;  // 텍스처 유닛 → glUniform1i로 지정
```

`u_Tex0`에 텍스처 유닛 5를 배정하고 BVH가 SSBO binding 5를 써도 **전혀 충돌하지 않는다**.

---

### `layout(binding=N)` vs `glUniform1i`

```glsl
// 방법 A: 레이아웃 바인딩 (일부 드라이버에서 compute shader에서 무시됨)
layout(binding = 5) uniform sampler2D u_Tex0;

// 방법 B: 명시적 uniform 설정 (이 브랜치에서 채택)
uniform sampler2D u_Tex0;  // 선언만
// C++에서:
shader.Use();
glUniform1i(glGetUniformLocation(shader.id, "u_Tex0"), 5);
```

이 브랜치에서 방법 B를 선택한 이유: NVIDIA / AMD / Intel 드라이버 간 호환성 문제 때문.

---

### `isnan()` 가드
GPU에서 `normalize(vec3(0,0,0))` = NaN. NaN이 `dot()`에 들어가면 결과도 NaN.  
NVIDIA에서는 `max(NaN, 0.0) = NaN`이고, `imageStore(NaN)` = 0으로 저장돼 검은 픽셀이 된다.

```glsl
float lenSq = dot(norm, norm);
if (isnan(lenSq) || lenSq < 1e-10) {
    // 퇴화 삼각형 (면적=0) → 면 법선으로 폴백
    norm = cross(v1 - v0, v2 - v0);
}
norm = (dot(norm,norm) > 1e-10) ? normalize(norm) : vec3(0,1,0);
```

NaN은 모든 비교에서 false를 반환한다: `NaN < x`, `NaN > x`, `NaN == NaN` 모두 false.  
따라서 `isnan()` 함수로 명시적으로 검사해야 한다.

---

## 6. C++ 패턴 및 문법

### `std::move` — 이동 의미론
대용량 데이터(67,000개 삼각형 배열)를 **복사 없이** 소유권만 이전한다.

```cpp
// ❌ 복사: ~8.7MB를 매 프레임 메모리 복사
m_CachedTriangles = proxy.triangles;

// ✅ 이동: 내부 포인터만 교환, O(1)
m_CachedTriangles = std::move(proxy.triangles);
// proxy.triangles는 이후 빈 상태(valid but unspecified)가 된다
```

`std::move`는 실제로 이동하지 않는다. "이 값은 더 이상 필요 없으니 훔쳐가도 된다"는  
컴파일러에게 보내는 힌트다. 이동 생성자가 정의된 타입(std::vector 등)에서만 효과가 있다.

---

### Lambda + `std::partition`

```cpp
// std::partition: 조건을 만족하는 원소를 앞으로, 아닌 것을 뒤로 이동
// 반환값: 두 번째 그룹의 시작 iterator
auto it = std::partition(
    tris.begin() + start,
    tris.begin() + end,
    [axis, mid](const TriangleProxy& t) {  // 람다 캡처: [axis, mid] 값으로 복사
        return TriCentroid(t)[axis] < mid;
    }
);
int splitIdx = static_cast<int>(it - tris.begin());
```

`[axis, mid]`: 람다 내부에서 외부 변수를 사용하기 위한 **캡처 리스트**.  
`[=]`로 모두 복사, `[&]`로 모두 참조 캡처도 가능하지만 명시적 캡처가 실수를 줄인다.

---

### `static bool registered = false` 패턴
함수/블록 스코프의 `static` 변수는 **최초 한 번만 초기화**된다.

```cpp
World::World() {
    static bool registered = false;  // 프로그램 수명 동안 한 번만 false로 초기화됨
    if (!registered) {
        ComponentRegistry::Register<CameraComponent>("CameraComponent");
        // ...
        registered = true;
    }
}
```

`World`를 여러 번 생성해도 컴포넌트 등록이 중복되지 않는다.  
멀티스레드 환경에서는 C++11 이후 `static` 초기화가 thread-safe하게 보장된다.

---

### `auto buildDedup = [&](...)` — 로컬 람다 함수
함수 내부에서만 쓰이는 작은 로직을 람다로 캡슐화해 코드 중복을 줄인다.

```cpp
// 함수를 새로 선언하기엔 짧고, 인라인으로 쓰기엔 두 번 반복되는 로직
auto buildDedup = [](const std::vector<GLuint>& ids,
                     std::unordered_map<GLuint,int>& slotMap,
                     std::vector<GLuint>& unique)
{
    for (GLuint id : ids)
        if (slotMap.find(id) == slotMap.end()) {
            slotMap[id] = static_cast<int>(unique.size());
            unique.push_back(id);
        }
};

buildDedup(proxy.diffuseTexIDs,  diffSlot, uniqueDiff);  // diffuse용
buildDedup(proxy.specularTexIDs, specSlot, uniqueSpec);  // specular용
```

`[&]` 대신 `[]`(캡처 없음)를 쓴 이유: 이 람다는 파라미터로만 동작하므로 외부 변수를 캡처할 필요가 없다. 캡처를 최소화하는 것이 의도를 명확히 한다.

---

### `static_cast<float>` vs C 스타일 캐스트

```cpp
// C 스타일 (권장하지 않음)
float matIdx = (float)meshIdx;

// C++ 스타일 (이 코드베이스 표준)
float matIdxF = static_cast<float>(matBase + static_cast<int>(meshIdx));
```

`static_cast`가 선호되는 이유:
1. **검색 가능**: `static_cast<float>` 는 grep으로 찾기 쉬움
2. **의도 명확**: 어떤 종류의 변환인지 명시
3. **컴파일 타임 검사**: 불가능한 변환을 컴파일 오류로 잡아줌

---

### `std::unordered_map` — 해시 맵
삽입/조회 평균 O(1). 텍스처 ID(GLuint) → 슬롯 인덱스(int) 매핑에 사용.

```cpp
std::unordered_map<GLuint, int> texIdToSlot;

// 삽입
texIdToSlot[texId] = slotIndex;  // operator[] : 없으면 삽입, 있으면 덮어씀

// 조회 (키 없으면 기본값 0 삽입됨 — 주의)
int slot = texIdToSlot[texId];

// 안전한 조회 (키 없으면 예외)
int slot = texIdToSlot.at(texId);

// 존재 확인
if (texIdToSlot.find(texId) != texIdToSlot.end()) { ... }
```

---

## 7. 작명 규칙 (Naming Convention)

### 멤버 변수 접두사 `m_`
```cpp
GLuint m_RTOutputTexture = 0;
bool   m_RTGeoDirty      = true;
```
`m_` = "member". 지역 변수와 구분되며, `this->` 없이도 멤버임을 즉시 알 수 있다.

---

### `Proxy` 접미사
```cpp
struct CameraProxy { ... };
struct LightProxy  { ... };
struct TriangleProxy { ... };
```
"Proxy" = 원본 오브젝트의 대리 데이터 구조체. OpenGL 자원이나 컴포넌트 로직 없이  
순수 데이터만 담는다. 레이어 경계를 넘어 데이터를 전달할 때 쓰는 패턴.

---

### `matBase` vs `matIdx` vs `matIdxF`

| 변수명 | 타입 | 의미 |
|--------|------|------|
| `matBase` | `int` | 현재 Actor의 첫 번째 메시가 전체 씬에서 몇 번째 슬롯부터 시작하는지 |
| `matIdx` | `int` | `matBase + 메시 인덱스` = 씬 전체에서의 고유 메시 번호 |
| `matIdxF` | `float` | `matIdx`를 GLSL `vec4.z`에 저장하기 위한 float 캐스트 |

예: 배낭 모델(79개 메시)이 Actor 1이고 구체 모델(1개 메시)이 Actor 2라면  
배낭: matBase=0, matIdx 0~78 / 구체: matBase=79, matIdx 79

---

### `tMin`, `hitIdx`, `hitBary`
레이 트레이싱 분야의 관습적인 변수명:

| 변수명 | 의미 |
|--------|------|
| `tMin` | 레이 오리진으로부터 최근접 교차점까지의 파라메트릭 거리 `t` |
| `hitIdx` | 교차한 삼각형의 배열 인덱스 (-1이면 미스) |
| `hitBary` | 교차점의 무게중심 좌표 `(u, v)` — `w = 1-u-v` |

`t`는 "레이 방정식 `P = O + t*D`"에서 온 변수명이다.

---

### `u_` 접두사 (GLSL Uniform)
```glsl
uniform sampler2D u_Tex0;
uniform sampler2D u_SpecTex0;
```
`u_` = "uniform". GLSL 컨벤션으로, C++쪽에서 `glGetUniformLocation(id, "u_Tex0")` 할 때  
오타를 줄이는 데 도움이 된다.

---

### `GL_TEXTURE5 + i` 패턴
```cpp
for (int i = 0; i < numDiff; i++) {
    glActiveTexture(GL_TEXTURE5 + i);  // GL_TEXTURE5, 6, 7, ...
    glBindTexture(GL_TEXTURE_2D, m_CachedDiffuseTexIDs[i]);
}
```
`GL_TEXTURE0 ~ GL_TEXTURE4`를 비워두는 이유:  
래스터화 패스의 `MeshComponent::Render()`가 슬롯 0을 `u_Diffuse`로 사용한다.  
RT 패스와 겹치지 않도록 5번부터 배정한다.

---

## 8. Sentinel 값과 Magic Number

### Sentinel -1.0f (specular 없음)
```cpp
tri.uv2mat.w = -1.0f;  // specular 맵 없음을 표시하는 특수값
```
셰이더에서:
```glsl
float specStrength = SampleSpecular(specIdx, uvHit);
if (specStrength < 0.0) {
    // specular 맵 없음 → albedo 밝기 기반 기본값 사용
}
```
`0.0`을 sentinel로 쓰지 않는 이유: specular 강도 0.0은 "광택 없음"이라는 유효한 값이기 때문.  
의미상 "데이터 없음"과 "값이 0"을 구분해야 할 때 음수 sentinel을 사용한다.

---

### `1e-7`, `1e-5`, `1e-10` — Epsilon 값들
부동소수점 비교에서 "사실상 0"을 판별하기 위한 허용 오차값.

| 값 | 사용처 | 이유 |
|----|--------|------|
| `1e-7` | Möller-Trumbore `abs(a) < 1e-7` | 레이-삼각형 평행 판별 |
| `0.001` | 교차 거리 `t < 0.001` | 자기 교차 방지 (surface bias) |
| `1e-5` | 라이트 거리 `dist < 1e-5` | 광원과 같은 위치 처리 |
| `1e-10` | 법선 길이 `lenSq < 1e-10` | 퇴화 삼각형 감지 |

이 값들을 너무 크게 잡으면 정상 케이스까지 스킵하고,  
너무 작게 잡으면 수치 오류를 막지 못한다.

---

### `(m_RTWidth + 15) / 16` — 올림 나누기
```cpp
m_ComputeShader->Dispatch(
    (m_RTWidth  + 15) / 16,  // ⌈width / 16⌉
    (m_RTHeight + 15) / 16
);
```
정수 나누기는 버림이므로 `1280 / 16 = 80`이지만 `1281 / 16 = 80`이 되어 마지막 열이 빠진다.  
`(x + 15) / 16`은 `⌈x/16⌉`(올림 나누기)과 동일하며, 이미지 크기가 16의 배수가 아니어도  
화면 전체를 커버하는 워크그룹 수를 보장한다.  
셰이더 내부에서 `if (pixel.x >= size.x) return;`으로 초과 스레드를 제거한다.
