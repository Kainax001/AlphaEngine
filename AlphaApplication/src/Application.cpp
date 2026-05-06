#include "AlphaApplication/Application.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

namespace AAP {

Application::Application()
{
}

Application::~Application()
{
    Shutdown();
}

bool Application::Init(const std::string& assetDir)
{
    m_Window = std::make_unique<AW::Window>();
    std::string windowConfig = assetDir + "config/window.json";
    if (!m_Window->Init(windowConfig))
    {
        std::cerr << "[Application] Window init failed\n";
        return false;
    }

    std::string rendererConfig = assetDir + "config/renderer.json";
    if (!AG::Renderer::Init(rendererConfig))
    {
        std::cerr << "[Application] Renderer init failed\n";
        return false;
    }

    m_Input.Init(m_Window->GetNativeHandle());

    // 임시 종료키 등록해둠
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

    m_IsRunning = true;
    m_CurrentTime = 0.0f;
    m_LastTime = 0.0f;

    if (!OnInit())
    {
        std::cerr << "[Application] OnInit failed\n";
        return false;
    }

    return true;
}

void Application::Shutdown()
{
    OnShutdown();
    CancelAllTasks();
    AG::Renderer::Shutdown();
    m_Window.reset();
}

void Application::Run()
{
    while (m_Window && m_Window->IsRunning())
    {
        float now = static_cast<float>(glfwGetTime());
        m_DeltaTime = now - m_LastTime;
        m_LastTime = now;
        m_CurrentTime = now;

        m_Input.NewFrame();
        m_Window->PollEvents();
        m_Input.Update();

        Update(m_DeltaTime);
        ProcessTaskQueue(m_DeltaTime);
        OnUpdate(m_DeltaTime);

        AG::Renderer::BeginFrame();
        OnRender();
        AG::Renderer::EndFrame();

        m_Window->SwapBuffers();
    }
}

void Application::PostTask(std::function<void()> callback, float delay, bool oneShot)
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    Task task;
    task.callback = callback;
    task.delayTime = delay;
    task.elapsedTime = 0.0f;
    task.oneShot = oneShot;
    task.isActive = true;
    m_TaskQueue.push(task);
}

void Application::CancelAllTasks()
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    while (!m_TaskQueue.empty())
        m_TaskQueue.pop();
}

// update(로직 등) 관리 예정
void Application::Update(float dt)
{
}

// rendering order 관리 예정
void Application::Render()
{
}

void Application::ProcessTaskQueue(float dt)
{
    std::lock_guard<std::mutex> lock(m_TaskMutex);

    std::queue<AAP::Task> newQueue;

    while (!m_TaskQueue.empty())
    {
        AAP::Task task = m_TaskQueue.front();
        m_TaskQueue.pop();

        if (!task.isActive)
            continue;

        task.elapsedTime += dt;

        if (task.elapsedTime >= task.delayTime)
        {
            if (task.callback)
                task.callback();

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
