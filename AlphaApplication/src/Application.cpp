#include "AlphaApplication/Application.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

namespace AAP {

Application::Application() = default;

Application::~Application()
{
    Shutdown();
}

bool Application::Init(const std::string& assetDir)
{
    m_Window = std::make_unique<AW::Window>();
    if (!m_Window->Init(assetDir + "config/window.json"))
    {
        std::cerr << "[Application] Window init failed\n";
        return false;
    }

    if (!AG::Renderer::Init(assetDir + "config/renderer.json"))
    {
        std::cerr << "[Application] Renderer init failed\n";
        return false;
    }

    m_Input.Init(m_Window->GetNativeHandle());

    m_Window->OnKey([this](int key, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            m_Window->Close();
    });
    m_Window->OnMouseMove([this](float dx, float dy) {
        m_Input.FeedMouseDelta(dx, dy);
    });
    m_Window->OnScroll([this](float dy) {
        m_Input.FeedScroll(dy);
    });

    m_IsRunning   = true;
    m_CurrentTime = 0.0f;
    m_LastTime    = 0.0f;

    // Create world
    m_World = std::make_unique<AS::World>();

    if (!OnInit())
    {
        std::cerr << "[Application] OnInit failed\n";
        return false;
    }

    // Let derived class populate the world
    OnWorldInit(m_World.get());
    m_World->BeginPlay();

    return true;
}

void Application::Shutdown()
{
    OnShutdown();
    CancelAllTasks();
    if (m_World) { m_World->EndPlay(); m_World.reset(); }
    AG::Renderer::Shutdown();
    m_Window.reset();
}

void Application::Run()
{
    while (m_Window && m_Window->IsRunning())
    {
        float now   = static_cast<float>(glfwGetTime());
        m_DeltaTime = now - m_LastTime;
        m_LastTime  = now;
        m_CurrentTime = now;

        m_Input.NewFrame();
        m_Window->PollEvents();
        m_Input.Update();

        // Custom per-frame hook (camera orbit, etc.)
        OnUpdate(m_DeltaTime);
        ProcessTaskQueue(m_DeltaTime);

        // World tick: FixedTick + Tick all components
        if (m_World) m_World->Tick(m_DeltaTime);

        AG::Renderer::BeginFrame();
        if (m_World) m_World->Render();
        OnRender();
        AG::Renderer::EndFrame();

        m_Window->SwapBuffers();
    }
}

void Application::PostTask(std::function<void()> callback, float delay, bool oneShot)
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    Task task;
    task.callback    = callback;
    task.delayTime   = delay;
    task.elapsedTime = 0.0f;
    task.oneShot     = oneShot;
    task.isActive    = true;
    m_TaskQueue.push(task);
}

void Application::CancelAllTasks()
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    while (!m_TaskQueue.empty())
        m_TaskQueue.pop();
}

void Application::Update(float dt) {}
void Application::Render()         {}

void Application::ProcessTaskQueue(float dt)
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);

    std::queue<AAP::Task> newQueue;
    while (!m_TaskQueue.empty())
    {
        AAP::Task task = m_TaskQueue.front();
        m_TaskQueue.pop();

        if (!task.isActive) continue;

        task.elapsedTime += dt;
        if (task.elapsedTime >= task.delayTime)
        {
            if (task.callback) task.callback();
            if (!task.oneShot)
            {
                task.elapsedTime = 0.0f;
                newQueue.push(task);
            }
        }
        else
        {
            newQueue.push(task);
        }
    }

    m_TaskQueue = newQueue;
}

} // namespace AAP
