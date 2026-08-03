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

#include "RedstoneParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::renderer::trident::particle::data {

RedstoneParticleData::RedstoneParticleData(const glm::vec3& color)
    : m_color(color)
{
    // 颜色分量应在 0-1 范围内
    m_color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
}

std::string RedstoneParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::Redstone);
}

std::string RedstoneParticleData::getParameters() const
{
    // 红石粒子参数格式: r g b
    // 例如: 1.0 0.0 0.0
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << m_color.r << " " << m_color.g << " " << m_color.b;
    return ss.str();
}

std::unique_ptr<ParticleData> RedstoneParticleData::clone() const
{
    return std::make_unique<RedstoneParticleData>(m_color);
}

} // namespace mc::client::renderer::trident::particle::data
