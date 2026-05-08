#include "../ClientApplication.hpp"

#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoResolver.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"

namespace mc::client {

void ClientApplication::updateTargetInfoUi()
{
    // 更新调试屏幕和目标信息
    auto* debugWidget = m_kageroEngine ?
        static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId)) : nullptr;
    auto* targetInfoWidget = m_kageroEngine ?
        static_cast<ui::minecraft::targetinfo::TargetInfoWidget*>(m_kageroEngine->getLayer(m_targetInfoLayerId)) : nullptr;

    if (m_player && m_mouseCaptured && targetInfoWidget) {
        glm::vec3 eyePos = m_camera.position();
        glm::vec3 forward = m_camera.forward();
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);

        targetInfoWidget->setTargetInfo(
            ui::minecraft::targetinfo::TargetInfoResolver::resolve(
                origin,
                direction,
                m_world,
                m_world.entityManager(),
                m_raycastResult,
                5.0f,
                [this](EntityId entityId) -> std::string {
                    const auto it = m_knownPlayerNames.find(static_cast<PlayerId>(entityId));
                    if (it == m_knownPlayerNames.end()) {
                        return {};
                    }
                    return it->second;
                }));
    } else if (targetInfoWidget) {
        targetInfoWidget->setTargetInfo(ui::minecraft::targetinfo::TargetInfoSnapshot::none());
    }

    if (debugWidget) {
        debugWidget->setTargetBlock(m_player && m_mouseCaptured ? &m_raycastResult : nullptr);
    }
}

} // namespace mc::client