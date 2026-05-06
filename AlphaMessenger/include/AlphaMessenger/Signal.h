#pragma once

#include <functional>
#include <unordered_map>
#include "Types.h"

namespace AMG {

// ==============================================================================
// Signal<Args...>
// 객체 멤버로 선언하는 로컬 콜백 시스템
// - Connect()로 리스너 등록, Disconnect()로 해제, Emit()으로 발송
// ==============================================================================
template<typename... Args>
class Signal
{
public:
    // 리스너 등록 → 나중에 Disconnect에 사용할 ID 반환
    ConnectionID Connect(std::function<void(Args...)> callback)
    {
        ConnectionID id = m_NextID++;
        m_Listeners[id] = std::move(callback);
        return id;
    }

    // 특정 리스너 해제
    void Disconnect(ConnectionID id)
    {
        m_Listeners.erase(id);
    }

    // 등록된 모든 리스너 해제
    void DisconnectAll()
    {
        m_Listeners.clear();
    }

    // 신호 발송 → 등록된 모든 콜백 호출
    void Emit(Args... args) const
    {
        for (auto& [id, callback] : m_Listeners)
        {
            if (callback)
                callback(args...);
        }
    }

    // 등록된 리스너 수
    size_t ListenerCount() const { return m_Listeners.size(); }

private:
    std::unordered_map<ConnectionID, std::function<void(Args...)>> m_Listeners;
    ConnectionID m_NextID = 1;
};

} // namespace AMG
