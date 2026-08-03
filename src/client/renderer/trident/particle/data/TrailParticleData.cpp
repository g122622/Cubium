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

#include "TrailParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <cstdio>
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

TrailParticleData::TrailParticleData(const Vector3d& targetPosition, u32 color, i32 durationInTicks)
    : m_targetPosition(targetPosition)
    , m_color(color)
    , m_durationInTicks(durationInTicks)
{}

std::string TrailParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::Trail);
}

std::string TrailParticleData::getParameters() const
{
    // 轨迹粒子参数格式: targetX targetY targetZ color duration
    char buf[256];
    std::snprintf(buf,
        sizeof(buf),
        "%.2f %.2f %.2f 0x%08X %d",
        m_targetPosition.x,
        m_targetPosition.y,
        m_targetPosition.z,
        m_color,
        m_durationInTicks);
    return std::string(buf);
}

std::unique_ptr<ParticleData> TrailParticleData::clone() const
{
    return std::make_unique<TrailParticleData>(m_targetPosition, m_color, m_durationInTicks);
}

} // namespace mc::client::renderer::trident::particle::data
