/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "VibrationParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include <cstdio>

namespace mc::client::renderer::trident::particle::data {

VibrationParticleData::VibrationParticleData(const Vector3d& targetPosition, i32 arrivalInTicks)
    : m_kind(TargetKind::Block)
    , m_targetPosition(targetPosition)
    , m_arrivalInTicks(arrivalInTicks)
{}

VibrationParticleData::VibrationParticleData(EntityInstanceId targetEntityId, f32 yOffset, i32 arrivalInTicks)
    : m_kind(TargetKind::Entity)
    , m_targetEntityId(targetEntityId)
    , m_yOffset(yOffset)
    , m_arrivalInTicks(arrivalInTicks)
{}

std::string VibrationParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::Vibration);
}

std::string VibrationParticleData::getParameters() const
{
    // 振动粒子参数格式取决于目标来源类型：
    // - 方块来源: targetX targetY targetZ arrivalInTicks
    // - 实体来源: entity yOffset arrivalInTicks
    // 用于 /particle 命令
    char buf[256];
    if (m_kind == TargetKind::Block) {
        std::snprintf(buf,
            sizeof(buf),
            "%.2f %.2f %.2f %d",
            m_targetPosition.x,
            m_targetPosition.y,
            m_targetPosition.z,
            m_arrivalInTicks);
    } else {
        std::snprintf(buf,
            sizeof(buf),
            "entity %llu %.2f %d",
            static_cast<unsigned long long>(m_targetEntityId),
            m_yOffset,
            m_arrivalInTicks);
    }
    return std::string(buf);
}

std::unique_ptr<ParticleData> VibrationParticleData::clone() const
{
    if (m_kind == TargetKind::Block) {
        return std::make_unique<VibrationParticleData>(m_targetPosition, m_arrivalInTicks);
    }
    return std::make_unique<VibrationParticleData>(m_targetEntityId, m_yOffset, m_arrivalInTicks);
}

} // namespace mc::client::renderer::trident::particle::data
