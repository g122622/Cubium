/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "client/application/ClientApplication.hpp"

#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfo.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoResolver.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/application/IntegratedServer.hpp"
#include <atomic>
#include <string>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {

void ClientApplication::updateTargetInfoUi()
{
    // 更新调试屏幕和目标信息
    auto* debugWidget = m_kageroEngine
        ? static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId))
        : nullptr;
    auto* targetInfoWidget = m_kageroEngine
        ? static_cast<ui::minecraft::targetinfo::TargetInfoWidget*>(m_kageroEngine->getLayer(m_targetInfoLayerId))
        : nullptr;

    if (m_player && m_mouseCaptured && targetInfoWidget) {
        glm::vec3 eyePos = m_camera.position();
        glm::vec3 forward = m_camera.forward();
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);

        targetInfoWidget->setTargetInfo(ui::minecraft::targetinfo::TargetInfoResolver::resolve(origin,
            direction,
            m_world,
            m_world.entityManager(),
            m_raycastResult,
            5.0f,
            [this](EntityInstanceId entityId) -> std::string {
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

        // 从集成服务器更新调试统计信息（原子读取，线程安全）
        if (m_integratedServer != nullptr) {
            const auto& stats = m_integratedServer->debugStats();
            debugWidget->setServerTickTimeMs(stats.smoothedTickTimeMs.load(std::memory_order::relaxed));
            debugWidget->setServerTargetMsPerTick(stats.targetMsPerTick.load(std::memory_order::relaxed));
            debugWidget->setForcedChunkCount(stats.forcedChunkCount.load(std::memory_order::relaxed));
        }
    }
}

} // namespace mc::client