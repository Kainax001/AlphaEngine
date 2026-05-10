# AlphaEngine API 설명서

## 목차

1. [모듈 구조](#모듈-구조)
2. [AlphaWindow](#alphawindow-namespace-aw)
3. [AlphaControler](#alphacontroler-namespace-ac)
4. [AlphaGraphic](#alphagraphic-namespace-ag)
   - [버퍼](#버퍼)
   - [메시](#메시)
   - [텍스처](#텍스처)
   - [셰이더](#셰이더) — Shader, ComputeShader, ShaderLibrary
   - [씬](#씬) — Transform, Camera, Light, Material, MaterialInstance, MaterialLibrary, Model
   - [렌더러](#렌더러)
5. [AlphaMessenger](#alphamessenger-namespace-amg)
6. [AlphaApplication](#alphaapplication-namespace-aap)

---

## 모듈 구조

```
AlphaEngine/
├── AlphaWindow/        (AW)  — 윈도우 생성, OpenGL 컨텍스트, 이벤트 콜백
├── AlphaControler/     (AC)  — 프레임 기반 키보드/마우스 입력
├── AlphaGraphic/       (AG)  — OpenGL 렌더링 시스템 전체
├── AlphaMessenger/     (AMG) — Signal(로컬) + EventBus(전역) 이벤트 시스템
├── AlphaApplication/   (AAP) — 앱 라이프사이클, 게임 루프, Task 큐
└── Sandbox/                  — 테스트 앱 (AlphaApplication 상속 예시)
```

**의존성:**
```
Sandbox
  └── AlphaApplication
        ├── AlphaWindow
        ├── AlphaControler
        ├── AlphaGraphic
        └── AlphaMessenger
```

---

## AlphaWindow `namespace AW`

윈도우 생성, OpenGL 컨텍스트 초기화, GLFW 이벤트 콜백을 담당합니다.

### WindowState

윈도우 생명주기 상태입니다.

```
Uninitialized → Created → Running ⇄ Minimized → Closing → Destroyed
```

| 상태 | 설명 |
|------|------|
| `AW_Uninitialized` | Init() 호출 전 초기 상태 |
| `AW_Created` | 윈도우 생성됨, 루프 시작 전 |
| `AW_Running` | PollEvents() 첫 호출 후 정상 실행 중 |
| `AW_Minimized` | 최소화 상태 |
| `AW_Closing` | Close() 호출됨, 루프 종료 예정 |
| `AW_Destroyed` | Shutdown() 완료 |

### WindowConfig

`config/window.json`에서 로드되는 설정 구조체입니다.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `title` | string | `"AlphaEngine"` | 윈도우 제목 |
| `width` | int | `1280` | 가로 해상도 |
| `height` | int | `720` | 세로 해상도 |
| `vsync` | bool | `true` | 수직동기화 |
| `fullscreen` | bool | `false` | 전체화면 |
| `captureCursor` | bool | `true` | 커서 캡처 (FPS 카메라용) |
| `resizable` | bool | `true` | 크기 조절 가능 여부 |
| `glMajor` / `glMinor` | int | `3` / `3` | OpenGL 버전 |

```cpp
static WindowConfig LoadFromFile(const std::string& path);
```

### Window

```cpp
// 생명주기
bool Init(const std::string& configPath);  // JSON 로드 → glfwCreateWindow → gladLoadGL
void Shutdown();                           // GLFW 정리
void Close();                              // 종료 요청 (IsRunning() → false)

// 루프
void PollEvents();    // GLFW 이벤트 수집, Created → Running 자동 전이
void SwapBuffers();   // 더블버퍼 교체 (프레임 완료 시 호출)

// 상태 조회
bool        IsRunning() const;  // Created | Running | Minimized 이면 true
WindowState GetState()  const;
int         GetWidth()  const;
int         GetHeight() const;

// 런타임 설정
void SetTitle(const std::string& title);
void SetVSync(bool enabled);

// 콜백 등록 (Application::Init에서 설정됨)
void OnResize   (std::function<void(int w, int h)>);
void OnKey      (std::function<void(int key, int action, int mods)>);
void OnMouseMove(std::function<void(float dx, float dy)>);  // 델타값 전달
void OnScroll   (std::function<void(float dy)>);

// OpenGL 네이티브 핸들 (Input 초기화에 사용)
GLFWwindow* GetNativeHandle() const;
```

---

## AlphaControler `namespace AC`

프레임 단위로 키보드/마우스 상태를 추적합니다. 현재 프레임과 이전 프레임 상태를 비교해 `Pressed` / `Released` 이벤트를 감지합니다.

### Key

GLFW 키코드와 1:1 대응하는 열거형입니다.

```cpp
// 알파벳
Key::A ~ Key::Z

// 숫자
Key::Num0 ~ Key::Num9

// 특수키
Key::Space, Key::Escape, Key::Enter, Key::Tab, Key::Backspace
Key::Left, Key::Right, Key::Up, Key::Down
Key::LeftShift, Key::LeftControl, Key::LeftAlt
Key::F1 ~ Key::F12
```

### MouseButton

```cpp
MouseButton::Left    // 0
MouseButton::Right   // 1
MouseButton::Middle  // 2
MouseButton::B4 ~ B7
```

### Keyboard

```cpp
bool IsKeyDown    (Key key);  // 누르고 있는 동안 매 프레임 true
bool IsKeyPressed (Key key);  // 누른 첫 프레임만 true
bool IsKeyReleased(Key key);  // 뗀 첫 프레임만 true
```

### Mouse

```cpp
bool IsButtonDown    (MouseButton btn);
bool IsButtonPressed (MouseButton btn);
bool IsButtonReleased(MouseButton btn);

float GetX()           const;  // 현재 커서 X
float GetY()           const;  // 현재 커서 Y
float GetDeltaX()      const;  // 이번 프레임 X 이동량
float GetDeltaY()      const;  // 이번 프레임 Y 이동량
float GetScrollDelta() const;  // 스크롤 이동량
```

### Input

```cpp
void Init(GLFWwindow* window);

void NewFrame();  // 이전 프레임 상태 백업 (루프 맨 앞에서 호출)
void Update();    // 현재 입력 상태 갱신 (PollEvents 이후 호출)

Keyboard& GetKeyboard();
Mouse&    GetMouse();

// Window 콜백에서 호출 (Application이 자동으로 연결)
void FeedMouseDelta(float dx, float dy);
void FeedScroll(float dy);
```

**프레임 처리 순서 (Application이 자동으로 관리):**

```
Input.NewFrame()      // 1. 이전 상태 백업
Window.PollEvents()   // 2. GLFW 이벤트 수집 → 콜백 실행
Input.Update()        // 3. 현재 상태 확정
```

---

## AlphaGraphic `namespace AG`

OpenGL 렌더링 시스템 전체를 담당합니다.

---

### 버퍼

OpenGL 버퍼 오브젝트 래퍼들입니다. 모두 동일한 패턴을 따릅니다: 생성자에서 `glGen*`, 소멸자에서 `Delete()` 자동 호출.

#### VAO (Vertex Array Object)

정점 속성 레이아웃을 정의합니다.

```cpp
VAO vao;

void Bind()   const;
void Unbind() const;
void Delete();

// 정점 속성 연결 (VBO의 데이터를 셰이더 layout location에 매핑)
void LinkAttrib(VBO& vbo, GLuint layout, GLuint numComponents,
                GLenum type, GLsizeiptr stride, void* offset);
```

#### VBO (Vertex Buffer Object)

정점 데이터를 GPU에 업로드합니다.

```cpp
VBO(std::vector<Vertex>& vertices);  // Vertex 배열로 생성

void Bind()   const;
void Unbind() const;
void Delete();
```

#### EBO (Element Buffer Object)

인덱스 데이터를 GPU에 업로드합니다 (중복 정점 제거).

```cpp
EBO(std::vector<GLuint>& indices);

void Bind()   const;
void Unbind() const;
void Delete();
```

#### UBO (Uniform Buffer Object)

여러 셰이더가 공유하는 Uniform 블록입니다.

```cpp
UBO(GLsizeiptr size, GLuint bindingPoint);  // 버퍼 크기, 바인딩 포인트

void Bind()   const;
void Unbind() const;
void Delete();

void UpdateData  (GLintptr offset, GLsizeiptr size, const void* data);
void LinkToShader(GLuint shaderID, const char* blockName, GLuint bindingPoint);
```

#### FBO (Framebuffer Object)

OpenGL 프레임버퍼 오브젝트 래퍼입니다. 고수준 `Framebuffer` 클래스의 내부에서 사용됩니다.

```cpp
FBO();  // glGenFramebuffers

void Bind()   const;  // GL_FRAMEBUFFER에 바인딩
void Unbind() const;  // 기본 프레임버퍼(0)로 복원
void Delete();        // glDeleteFramebuffers, m_ID → 0

GLuint m_ID;
```

#### Framebuffer

오프스크린 렌더링 타겟입니다. `FBO`를 내부 멤버로 사용하고, 컬러/깊이 어태치먼트를 관리합니다.

```cpp
// 사용 예
FramebufferSpec spec;
spec.Width       = 1280;
spec.Height      = 720;
spec.Attachments = {
    { FramebufferTextureFormat::RGBA8 },           // 컬러 버퍼
    { FramebufferTextureFormat::Depth24Stencil8 }  // 깊이+스텐실 버퍼
};
spec.Samples = 1;  // MSAA 샘플 수

Framebuffer fb(spec);

fb.Bind();
// ... 이 Framebuffer에 렌더링 ...
fb.Unbind();

unsigned int colorTex = fb.GetColorAttachment(0);  // 컬러 어태치먼트 텍스처 ID
unsigned int depthTex = fb.GetDepthAttachment();    // 깊이 어태치먼트 텍스처 ID

fb.Resize(newW, newH);  // 해상도 변경 (텍스처 재생성, FBO 재사용)
```

**어태치먼트 포맷:**

| 포맷 | 용도 |
|------|------|
| `RGBA8` | 일반 컬러 렌더링 |
| `RedInteger` | 오브젝트 ID 피킹 (R32I) |
| `Depth24Stencil8` | 깊이 + 스텐실 |

---

### 메시

#### Vertex

```cpp
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};
```

#### Mesh (추상)

```cpp
virtual void Draw()   const = 0;
virtual void Bind()   const = 0;
virtual void Unbind() const = 0;
virtual unsigned int GetVAO()         const = 0;
unsigned int         GetVertexCount() const;
unsigned int         GetIndexCount()  const;
```

#### StaticMesh

불변 정적 메시입니다. 파일 또는 직접 데이터로 생성합니다.

```cpp
StaticMesh(const std::string& path);                                    // 파일에서 로드
StaticMesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices); // 데이터에서 생성

void         Draw()   const override;
void         Bind()   const override;
void         Unbind() const override;
unsigned int GetVAO() const override;

const std::string& GetPath() const;
```

---

### 텍스처

#### Texture (추상)

```cpp
virtual void Bind(unsigned int slot = 0) const = 0;
virtual void Unbind()                    const = 0;

unsigned int GetID()     const;
int          GetWidth()  const;
int          GetHeight() const;
```

#### Texture2D

```cpp
Texture2D(const std::string& path, bool flipVertically = true);

void Bind  (unsigned int slot = 0) const override;
void Unbind()                      const override;

int                GetChannels() const;
const std::string& GetPath()     const;
```

#### TextureCubemap

스카이박스 등 큐브맵 텍스처입니다.

```cpp
// faces 순서: right, left, top, bottom, front, back
TextureCubemap(const std::array<std::string, 6>& faces, bool flipVertically = false);

void Bind  (unsigned int slot = 0) const override;
void Unbind()                      const override;
```

#### RenderTexture

렌더링 가능한 텍스처입니다. 내부에 FBO와 RBO를 가지며, 텍스처처럼 샘플링도 가능합니다.

```cpp
RenderTexture(int width, int height,
              RenderTextureFormat format = RenderTextureFormat::RGBA);

void Bind  (unsigned int slot = 0) const override;  // 텍스처로 바인딩 (샘플링용)
void Unbind()                      const override;

void BindFramebuffer()   const;  // 렌더 타겟으로 설정
void UnbindFramebuffer() const;

void Resize(int width, int height);

unsigned int        GetFBO()    const;
RenderTextureFormat GetFormat() const;
```

**포맷:**

| 포맷 | 설명 |
|------|------|
| `RGB` | 24비트 컬러 |
| `RGBA` | 32비트 컬러 (알파 포함) |
| `Depth` | 깊이 전용 |
| `DepthStencil` | 깊이 + 스텐실 |

---

### 셰이더

#### Shader

```cpp
Shader();  // 기본 생성자 (m_ID = 0, 멤버 선언용)
Shader(const char* vertPath, const char* fragPath,
       const char* geomPath = nullptr);  // 파일에서 컴파일 및 링크

void Use() const;

bool Reload();    // 저장된 경로로 재컴파일. 실패 시 이전 셰이더 유지, false 반환
bool IsValid() const;  // m_ID != 0

// 경로 조회 (Reload() 및 외부에서 경로 참조 시 사용)
const std::string& GetVertPath() const;
const std::string& GetFragPath() const;
const std::string& GetGeoPath()  const;

// Uniform 설정
void SetBool (const std::string& name, bool value)        const;
void SetInt  (const std::string& name, int value)         const;
void SetFloat(const std::string& name, float value)       const;
void SetVec2 (const std::string& name, const glm::vec2&)  const;
void SetVec2 (const std::string& name, float x, float y)  const;
void SetVec3 (const std::string& name, const glm::vec3&)  const;
void SetVec3 (const std::string& name, float x, float y, float z) const;
void SetVec4 (const std::string& name, const glm::vec4&)  const;
void SetVec4 (const std::string& name, float x, float y, float z, float w) const;
void SetMat2 (const std::string& name, const glm::mat2&)  const;
void SetMat3 (const std::string& name, const glm::mat3&)  const;
void SetMat4 (const std::string& name, const glm::mat4&)  const;

unsigned int m_ID;  // 셰이더 프로그램 ID (0이면 유효하지 않음)
```

#### ComputeShader

GPU 컴퓨트 셰이더입니다.

```cpp
ComputeShader(const char* computePath);

void Use()     const;
void Dispatch(unsigned int x, unsigned int y, unsigned int z) const;  // 워크그룹 수

void SetInt  (const std::string& name, int value)   const;
void SetFloat(const std::string& name, float value) const;

unsigned int m_ID;
```

#### ShaderLibrary

이름으로 셰이더를 관리하는 전역 캐시입니다.

```cpp
static Shader& Load(const std::string& name,
                    const char* vertexPath,
                    const char* fragmentPath,
                    const char* geometryPath = nullptr);  // 로드 후 등록

static Shader& Get   (const std::string& name);   // 이름으로 조회
static bool    Exists(const std::string& name);   // 등록 여부 확인
static void    Clear();                           // 전체 해제

// Hot-Reload
static bool Reload   (const std::string& name);  // 특정 셰이더 재컴파일, 없으면 false
static void ReloadAll();                          // 등록된 모든 셰이더 일괄 재컴파일
```

```cpp
// 사용 예
ShaderLibrary::Load("Lit", "shaders/Lit.vert", "shaders/Lit.frag");
Shader& lit = ShaderLibrary::Get("Lit");

// 키 입력으로 수동 Hot-Reload
if (input.IsKeyPressed(Key::F5))
    AG::ShaderLibrary::Reload("Lit");     // 특정 셰이더만

if (input.IsKeyPressed(Key::R))
    AG::ShaderLibrary::ReloadAll();       // 전체 재컴파일

// ShaderLibrary를 사용하지 않는 직접 소유 Shader도 동일 인터페이스
if (input.IsKeyPressed(Key::F5))
    m_Shader.Reload();
```

---

### 씬

#### Transform

오브젝트의 위치/회전/스케일을 관리하고 TRS 모델 행렬을 생성합니다. 회전은 쿼터니언으로 저장합니다.

```cpp
glm::mat4 GetModelMatrix() const;  // Translation * Rotation * Scale

// Getter
const glm::vec3& GetPosition() const;
const glm::quat& GetRotation() const;
const glm::vec3& GetScale()    const;

// Setter
void SetPosition   (const glm::vec3& position);
void SetRotation   (const glm::quat& rotation);
void SetScale      (const glm::vec3& scale);
void SetEulerAngles(const glm::vec3& eulerDegrees);  // 오일러 각도 → 쿼터니언 변환

// 상대 변환
void Translate(const glm::vec3& delta);
void Rotate   (float angleDeg, const glm::vec3& axis);
void Scale    (const glm::vec3& factor);
```

#### Camera

Fly 카메라 (1인칭 자유 이동)입니다.

```cpp
Camera(glm::vec3 position = {0,0,3},
       glm::vec3 up       = {0,1,0},
       float yaw = -90.0f, float pitch = 0.0f);

glm::mat4 GetViewMatrix()                  const;
glm::mat4 GetProjectionMatrix(float aspect) const;  // FOV 45°, Near 0.1, Far 100

// 입력 처리
void ProcessKeyboard   (CameraMovement dir, float dt);
void ProcessMouseMove  (float dx, float dy, bool constrainPitch = true);
void ProcessMouseScroll(float dy);  // FOV ±

// Getter / Setter
const glm::vec3& GetPosition() const;
const glm::vec3& GetFront()    const;
float            GetZoom()     const;

void SetPosition(const glm::vec3& pos);
void SetSpeed   (float speed);  // 기본값 2.5
```

```cpp
enum class CameraMovement { Forward, Backward, Left, Right, Up, Down };
```

#### Light (추상)

```cpp
// 공통
const glm::vec3& GetColor()     const;
float            GetIntensity() const;
LightType        GetType()      const;  // Directional | Point | Spot

void SetColor    (const glm::vec3& color);
void SetIntensity(float intensity);
```

#### DirectionalLight

태양광 같은 방향광입니다. 위치 없이 방향만 가집니다.

```cpp
DirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);

const glm::vec3& GetDirection() const;
void             SetDirection(const glm::vec3& direction);
```

#### PointLight

전구처럼 모든 방향으로 빛을 방출하는 점광원입니다. 거리에 따라 감쇠합니다.

```cpp
PointLight(const glm::vec3& position, const glm::vec3& color, float intensity);

const glm::vec3& GetPosition()  const;
float            GetConstant()  const;  // 기본값 1.0
float            GetLinear()    const;  // 기본값 0.09
float            GetQuadratic() const;  // 기본값 0.032

void SetPosition   (const glm::vec3& position);
void SetAttenuation(float constant, float linear, float quadratic);
```

감쇠 공식: `1.0 / (constant + linear * d + quadratic * d²)`

#### SpotLight

특정 방향을 향하는 원뿔형 광원입니다.

```cpp
SpotLight(const glm::vec3& position, const glm::vec3& direction,
          const glm::vec3& color, float intensity);

const glm::vec3& GetPosition()    const;
const glm::vec3& GetDirection()   const;
float            GetCutOff()      const;  // 내부 원뿔 각도 (기본 12.5°)
float            GetOuterCutOff() const;  // 외부 원뿔 각도 (기본 17.5°)

void SetPosition   (const glm::vec3&);
void SetDirection  (const glm::vec3&);
void SetCutOff     (float degrees);
void SetOuterCutOff(float degrees);
```

#### Material

Shader + Texture 슬롯 + Uniform 값을 하나로 묶는 컨테이너입니다. 텍스처는 삽입 순서대로 슬롯 번호가 고정됩니다 (`vector<pair>` 내부 구현으로 순서 보장).

```cpp
Material(std::shared_ptr<Shader> shader);

void Bind()   const;  // Shader 활성화 + Texture 바인딩 + 모든 Uniform 전송
void Unbind() const;

// --- Texture (삽입 순서 = slot 번호, 같은 이름 재설정 시 slot 유지) ---
void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);

// --- Scalar / Vector / Matrix ---
void SetBool (const std::string& name, bool value);
void SetInt  (const std::string& name, int value);
void SetFloat(const std::string& name, float value);
void SetVec3 (const std::string& name, const glm::vec3& value);
void SetVec4 (const std::string& name, const glm::vec4& value);
void SetMat4 (const std::string& name, const glm::mat4& value);

// --- Getter ---
bool      GetBool (const std::string& name) const;
int       GetInt  (const std::string& name) const;
float     GetFloat(const std::string& name) const;
glm::vec3 GetVec3 (const std::string& name) const;
glm::vec4 GetVec4 (const std::string& name) const;

bool HasTexture (const std::string& name) const;
bool HasProperty(const std::string& name) const;  // Bool/Int/Float/Vec3/Vec4/Mat4 중 하나라도 있으면 true

std::shared_ptr<Shader> GetShader()       const;
int                     GetTextureCount() const;  // 등록된 텍스처 수 (= 다음 할당 슬롯 번호)
```

```cpp
// 사용 예
auto mat = std::make_shared<AG::Material>(shader);
mat->SetTexture("u_AlbedoMap",  albedoTex);   // slot 0
mat->SetTexture("u_NormalMap",  normalTex);   // slot 1
mat->SetVec3("u_Color", glm::vec3(1.0f));
mat->SetMat4("u_Model", transform.GetModelMatrix());
mat->Bind();
```

#### MaterialInstance

같은 베이스 Material을 공유하되 일부 값만 오버라이드합니다. `Bind()` 시 Base → Override 순으로 적용됩니다. 오버라이드하지 않은 값은 Base 값이 그대로 사용됩니다.

```cpp
MaterialInstance(std::shared_ptr<Material> base);

void Bind()   const;  // Base::Bind() 후 override 값 덮어씌움
void Unbind() const;

void SetTexture(const std::string& name, std::shared_ptr<Texture> texture);
void SetBool (const std::string& name, bool value);
void SetInt  (const std::string& name, int value);
void SetFloat(const std::string& name, float value);
void SetVec3 (const std::string& name, const glm::vec3& value);
void SetVec4 (const std::string& name, const glm::vec4& value);
void SetMat4 (const std::string& name, const glm::mat4& value);

std::shared_ptr<Material> GetBase() const;
```

```cpp
// 사용 예
auto base = std::make_shared<AG::Material>(litShader);
base->SetTexture("u_Diffuse", defaultTex);
base->SetVec3("u_Color", glm::vec3(1.0f));

auto redInst = std::make_shared<AG::MaterialInstance>(base);
redInst->SetVec3("u_Color", glm::vec3(1, 0, 0));  // 색만 다름

auto blueInst = std::make_shared<AG::MaterialInstance>(base);
blueInst->SetTexture("u_Diffuse", blueTex);        // 텍스처만 다름
```

#### MaterialLibrary

이름 기반 전역 Material 캐시입니다. `ShaderLibrary`와 동일한 패턴입니다.

```cpp
static std::shared_ptr<Material> Load(const std::string& name,
                                      std::shared_ptr<Shader> shader);  // 생성 + 등록
static std::shared_ptr<Material> Get   (const std::string& name);  // 없으면 nullptr + 에러 출력
static bool                      Exists(const std::string& name);
static void                      Clear();  // 전체 해제 (씬 전환 시 등)
```

```cpp
// 사용 예
auto mat = AG::MaterialLibrary::Load("Lit", litShader);
mat->SetTexture("u_Diffuse", tex);

auto same = AG::MaterialLibrary::Get("Lit");  // 어디서든 동일 인스턴스 반환
```

#### Model

Assimp를 통해 `.obj`, `.fbx` 등 3D 모델 파일을 로드합니다. 메시별 diffuse 텍스처를 자동으로 바인딩합니다.

```cpp
Model();                          // 기본 생성자 (나중에 로드)
Model(const std::string& path);   // 파일에서 로드

// Material이 설정된 경우: Material 시스템만 사용 (슬롯 충돌 방지)
// Material이 없는 경우: Assimp가 읽어온 메시별 diffuse 텍스처를 슬롯 0에 바인딩
void Draw() const;

glm::vec3        GetCenter()   const;
Transform&       GetTransform();
const Transform& GetTransform() const;

void SetMaterial(std::shared_ptr<Material> material);
```

---

### 렌더러

#### RendererConfig

`config/renderer.json`에서 로드되는 렌더링 파이프라인 설정입니다.

| 섹션 | 필드 | 기본값 | 설명 |
|------|------|--------|------|
| Clear | `clearColor[4]` | `{0.07, 0.07, 0.10, 1.0}` | 배경색 |
| | `clearDepth` | `1.0` | 깊이 클리어 값 |
| | `clearStencil` | `0` | 스텐실 클리어 값 |
| Depth | `depthTest` | `true` | 깊이 테스트 활성화 |
| | `depthFunc` | `"less"` | `less \| lequal \| equal \| greater \| always \| never` |
| | `depthMask` | `true` | 깊이 버퍼 쓰기 |
| Blend | `blend` | `false` | 블렌딩 활성화 |
| | `blendSrc` | `"src_alpha"` | 소스 블렌드 팩터 |
| | `blendDst` | `"one_minus_src_alpha"` | 대상 블렌드 팩터 |
| Cull | `cullFace` | `false` | 면 컬링 활성화 |
| | `cullMode` | `"back"` | `back \| front \| front_and_back` |
| | `frontFace` | `"ccw"` | `ccw \| cw` |
| Stencil | `stencilTest` | `false` | 스텐실 테스트 활성화 |
| Polygon | `polygonMode` | `"fill"` | `fill \| line \| point` |
| Misc | `multisample` | `true` | MSAA |
| | `lineWidth` | `1.0` | 선 굵기 |
| | `pointSize` | `1.0` | 점 크기 |

#### Renderer (정적 클래스)

```cpp
static bool Init(const std::string& configPath = "");  // renderer.json 로드 및 OpenGL 상태 적용
static void Shutdown();

static void BeginFrame();  // 화면 클리어 (색상 + 깊이 + 스텐실)
static void EndFrame();    // 렌더링 후처리 정리

static void SetClearColor(float r, float g, float b, float a = 1.0f);
static void Clear();       // 현재 설정으로 즉시 클리어

static void ApplyConfig(const RendererConfig& cfg);   // 설정 즉시 적용
static const RendererConfig& GetConfig();
```

---

## AlphaMessenger `namespace AMG`

객체 간 통신을 위한 두 가지 이벤트 시스템을 제공합니다.

| 시스템 | 규모 | 패턴 | 용도 |
|--------|------|------|------|
| `Signal<T...>` | 로컬 (객체 간) | Observer | 컴포넌트 이벤트 (HP 변화, 충돌 등) |
| `EventBus` | 전역 (시스템 간) | Pub/Sub | 시스템 상태 변화 (씬 전환, 게임 일시정지 등) |

```cpp
using ConnectionID = uint64_t;  // 구독 해제용 ID
```

### Signal\<Args...\>

객체 멤버로 선언하는 타입 안전 콜백 시스템입니다.

```cpp
// 선언
AMG::Signal<int>        onHealthChanged;
AMG::Signal<glm::vec3>  onPositionChanged;
AMG::Signal<>           onDied;            // 인자 없음

// 구독 → ConnectionID 반환
ConnectionID id = onHealthChanged.Connect([](int hp) {
    UpdateHealthBar(hp);
});

// 발송 → 등록된 모든 콜백 호출
onHealthChanged.Emit(80);

// 해제
onHealthChanged.Disconnect(id);
onHealthChanged.DisconnectAll();

// 등록 수 확인
size_t count = onHealthChanged.ListenerCount();
```

### EventBus (정적 클래스)

이벤트는 일반 `struct`로 정의합니다. 타입으로 구독/발행합니다.

```cpp
// 이벤트 정의 (어디서든)
struct SceneChangedEvent  { std::string sceneName; };
struct GamePausedEvent    { bool isPaused; };
struct ResourceLoadedEvent{ std::string path; bool success; };

// 구독 → ConnectionID 반환
ConnectionID id = AMG::EventBus::Subscribe<GamePausedEvent>(
    [](const GamePausedEvent& e) {
        if (e.isPaused) PausePhysics();
    }
);

// 발행 → 해당 타입 구독자 전체 호출
AMG::EventBus::Emit(GamePausedEvent{ true });

// 특정 구독 해제
AMG::EventBus::Unsubscribe<GamePausedEvent>(id);

// 전체 해제 (씬 전환 시 등)
AMG::EventBus::Clear();
```

---

## AlphaApplication `namespace AAP`

게임/앱의 라이프사이클, 메인 루프, Task 큐를 관리합니다.

### Task

지연 실행 작업의 데이터 구조입니다.

```cpp
struct Task {
    std::function<void()> callback;
    float elapsedTime = 0.0f;  // 경과 시간 누적
    float delayTime   = 0.0f;  // 실행까지 대기 시간 (초)
    bool  oneShot     = true;  // true: 한 번 실행 후 제거 / false: 반복
    bool  isActive    = true;  // false: 무시
};
```

### Application

상속을 통해 커스텀 앱을 만드는 기반 클래스입니다.

```cpp
// 공개 인터페이스
bool Init(const std::string& assetDir = "");  // 초기화 (윈도우+렌더러+입력+OnInit)
void Run();                                    // 메인 루프 (종료까지 블로킹)
void Shutdown();                               // 정리 (OnShutdown+리소스 해제)

// Task 큐 (스레드 안전 — mutex 보호)
void PostTask(std::function<void()> callback,
              float delay   = 0.0f,  // 초 단위 지연
              bool  oneShot = true);
void CancelAllTasks();

// protected — 자식 클래스에서 사용
AW::Window& GetWindow();
AC::Input&  GetInput();
float       GetDeltaTime() const;  // 이번 프레임 경과 시간 (초)
float       GetTime()      const;  // 앱 시작 후 경과 시간 (초)
```

### 상속 패턴

```cpp
class MyApp : public AAP::Application {
protected:
    bool OnInit()           override;  // 리소스 로드, 셰이더/모델 초기화
    void OnShutdown()       override;  // 리소스 정리
    void OnUpdate(float dt) override;  // 매 프레임 게임 로직
    void OnRender()         override;  // 매 프레임 렌더링
};

int main() {
    MyApp app;
    if (!app.Init(ASSET_DIR)) return -1;
    app.Run();
    return 0;
}
```

### 메인 루프 실행 순서

```
while (윈도우 열려있음):
  ┌─ 시간 계산
  │    now       = glfwGetTime()
  │    DeltaTime = now - lastTime
  │
  ├─ 입력 처리
  │    Input.NewFrame()
  │    Window.PollEvents()    → OnKey / OnMouseMove / OnScroll 콜백 실행
  │    Input.Update()
  │
  ├─ 업데이트
  │    ProcessTaskQueue(dt)   → 지연 완료된 Task 실행
  │    OnUpdate(dt)           → 게임 로직 (카메라, 라이트 등)
  │
  └─ 렌더링
       Renderer.BeginFrame()  → 화면 클리어
       OnRender()             → 셰이더 설정 + Draw 호출
       Renderer.EndFrame()
       Window.SwapBuffers()   → 더블버퍼 교체
```

### PostTask 예시

```cpp
// 즉시 실행 (다음 프레임)
PostTask([]{ std::cout << "Hello\n"; });

// 3초 후 한 번 실행
PostTask([]{ LoadNextLevel(); }, 3.0f, true);

// 매 1초마다 반복 실행
PostTask([]{ SpawnEnemy(); }, 1.0f, false);

// 외부 스레드에서도 안전하게 호출 가능 (mutex 보호)
std::thread([this]{ PostTask([]{ OnResourceLoaded(); }); }).detach();
```
