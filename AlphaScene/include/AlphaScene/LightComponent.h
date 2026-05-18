#pragma once
#include "Component.h"
#include "RenderContext.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <memory>

namespace AS {

class LightComponent : public Component {
public:
    explicit LightComponent(Actor* owner);                                    // for ComponentRegistry
    LightComponent(Actor* owner, std::shared_ptr<AG::Light> light);

    LightData      GetLightData() const;
    AG::LightProxy ToProxy()     const;
    AG::Light* GetLight()     const { return m_Light.get(); }

    void SetLight(std::shared_ptr<AG::Light> light) { m_Light = std::move(light); }

    const char*     GetTypeName()  const override { return "LightComponent"; }
    std::type_index GetTypeIndex() const override { return typeid(LightComponent); }
    void Serialize  (rapidjson::Value& out,
                     rapidjson::Document::AllocatorType& alloc) const override;
    void Deserialize(const rapidjson::Value& in) override;

private:
    std::shared_ptr<AG::Light> m_Light;
};

} // namespace AS
