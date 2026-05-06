#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <memory>
#include <AlphaWindow/AlphaWindow.h>
#include <AlphaControler/AlphaControler.h>

namespace AAP {

struct Task {
    std::function<void()> callback;
    float elapsedTime = 0.0f;
    float delayTime = 0.0f;
    bool oneShot = true;
    bool isActive = true;
};

class Application {
public:
    Application();
    virtual ~Application();

    bool Init(const std::string& assetDir = "");
    void Shutdown();
    void Run();

    void PostTask(std::function<void()> callback, float delay = 0.0f, bool oneShot = true);
    void CancelAllTasks();

protected:
    virtual bool OnInit() { return true; }
    virtual void OnShutdown() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnRender() {}

    AW::Window& GetWindow() { return *m_Window; }
    AC::Input& GetInput() { return m_Input; }
    float GetDeltaTime() const { return m_DeltaTime; }
    float GetTime() const { return m_CurrentTime; }

private:
    void Update(float dt);
    void Render();
    void ProcessTaskQueue(float dt);

    std::unique_ptr<AW::Window> m_Window;
    AC::Input m_Input;

    std::queue<Task> m_TaskQueue;
    std::mutex m_TaskMutex;

    float m_CurrentTime = 0.0f;
    float m_LastTime = 0.0f;
    float m_DeltaTime = 0.0f;
    bool m_IsRunning = false;
};

} // namespace AAP
