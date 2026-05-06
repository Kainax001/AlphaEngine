#pragma once

#include <functional>
#include <unordered_map>
#include <typeindex>
#include "Types.h"

namespace AMG {

// ==============================================================================
// EventBus
// 전역 Pub/Sub 시스템 - 시스템 레벨의 거시적 상태 변화에 사용
// - Subscribe()로 이벤트 구독, Unsubscribe()로 해제, Emit()으로 발행
//
// 사용 예:
//   struct SceneChangedEvent { std::string sceneName; };
//   EventBus::Subscribe<SceneChangedEvent>([](const SceneChangedEvent& e) { ... });
//   EventBus::Emit(SceneChangedEvent{ "Level2" });
// ==============================================================================
class EventBus
{
public:
    // 이벤트 구독 → ConnectionID 반환 (Unsubscribe에 사용)
    template<typename EventT>
    static ConnectionID Subscribe(std::function<void(const EventT&)> callback)
    {
        ConnectionID id = NextID();
        Handlers(typeid(EventT))[id] = [cb = std::move(callback)](const void* e)
        {
            cb(*static_cast<const EventT*>(e));
        };
        return id;
    }

    // 특정 구독 해제
    template<typename EventT>
    static void Unsubscribe(ConnectionID id)
    {
        auto& all = AllHandlers();
        auto it = all.find(typeid(EventT));
        if (it != all.end())
            it->second.erase(id);
    }

    // 이벤트 발행 → 해당 타입을 구독한 모든 리스너 호출
    template<typename EventT>
    static void Emit(const EventT& event)
    {
        auto& all = AllHandlers();
        auto it = all.find(typeid(EventT));
        if (it == all.end())
            return;

        for (auto& [id, fn] : it->second)
        {
            if (fn)
                fn(&event);
        }
    }

    // 모든 구독 해제 (종료 시 또는 씬 전환 시 사용)
    static void Clear()
    {
        AllHandlers().clear();
    }

private:
    using HandlerFn  = std::function<void(const void*)>;
    using HandlerMap = std::unordered_map<ConnectionID, HandlerFn>;

    static std::unordered_map<std::type_index, HandlerMap>& AllHandlers()
    {
        static std::unordered_map<std::type_index, HandlerMap> s_Handlers;
        return s_Handlers;
    }

    static HandlerMap& Handlers(std::type_index type)
    {
        return AllHandlers()[type];
    }

    static ConnectionID NextID()
    {
        static ConnectionID s_ID = 1;
        return s_ID++;
    }
};

} // namespace AMG
