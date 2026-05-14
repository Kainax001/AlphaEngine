#pragma once
#include "Component.h"
#include "RenderContext.h"
#include <AlphaGraphic/AlphaGraphic.h>
#include <memory>

namespace AS {

class LightComponent : public Component {
public:
    LightComponent(Actor* owner, std::shared_ptr<AG::Light> light);

    LightData  GetLightData() const;
    AG::Light* GetLight()     const { return m_Light.get(); }

    void SetLight(std::shared_ptr<AG::Light> light) { m_Light = std::move(light); }

private:
    std::shared_ptr<AG::Light> m_Light;
};

} // namespace AS
