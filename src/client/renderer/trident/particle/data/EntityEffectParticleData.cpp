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

#include "EntityEffectParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::data {

// ============================================================================
// EntityEffectParticleData
// ============================================================================

EntityEffectParticleData::EntityEffectParticleData(u32 color)
    : m_color(color)
{}

std::string EntityEffectParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::EntityEffect);
}

std::string EntityEffectParticleData::getParameters() const
{
    // 实体效果粒子参数格式: color(ARGB)
    // 例如: 0xFFFF0000
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << m_color;
    return ss.str();
}

std::unique_ptr<ParticleData> EntityEffectParticleData::clone() const
{
    return std::make_unique<EntityEffectParticleData>(m_color);
}

glm::vec4 EntityEffectParticleData::toRGBAVector() const
{
    // ARGB -> RGBA float: A = (color >> 24) / 255, R = ((color >> 16) & 0xFF) / 255, etc.
    f32 a = static_cast<f32>((m_color >> 24) & 0xFF) / 255.0f;
    f32 r = static_cast<f32>((m_color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((m_color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(m_color & 0xFF) / 255.0f;
    return glm::vec4(r, g, b, a);
}

// static
EntityEffectParticleData EntityEffectParticleData::fromRGBAVector(const glm::vec4& rgba)
{
    u8 a = static_cast<u8>(std::clamp(rgba.a * 255.0f, 0.0f, 255.0f));
    u8 r = static_cast<u8>(std::clamp(rgba.r * 255.0f, 0.0f, 255.0f));
    u8 g = static_cast<u8>(std::clamp(rgba.g * 255.0f, 0.0f, 255.0f));
    u8 b = static_cast<u8>(std::clamp(rgba.b * 255.0f, 0.0f, 255.0f));
    u32 color =
        (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
    return EntityEffectParticleData(color);
}

} // namespace mc::client::renderer::trident::particle::data
