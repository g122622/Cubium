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

#include "DustParticleData.hpp"
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
// DustParticleData
// ============================================================================

DustParticleData::DustParticleData(u32 color, f32 scale)
    : m_color(color)
    , m_scale(std::clamp(scale, 0.01f, 4.0f))
{}

std::string DustParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::Dust);
}

std::string DustParticleData::getParameters() const
{
    // 灰尘粒子参数格式: color(ARGB) scale
    // 例如: 0xFFFF0000 1.0
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << m_color << std::dec << " "
       << std::fixed << std::setprecision(2) << m_scale;
    return ss.str();
}

std::unique_ptr<ParticleData> DustParticleData::clone() const
{
    return std::make_unique<DustParticleData>(m_color, m_scale);
}

glm::vec4 DustParticleData::toRGBAVector() const
{
    // ARGB -> RGBA float: A = (color >> 24) / 255, R = ((color >> 16) & 0xFF) / 255, etc.
    f32 a = static_cast<f32>((m_color >> 24) & 0xFF) / 255.0f;
    f32 r = static_cast<f32>((m_color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((m_color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(m_color & 0xFF) / 255.0f;
    return glm::vec4(r, g, b, a);
}

// static
DustParticleData DustParticleData::fromRGBAVector(const glm::vec4& rgba, f32 scale)
{
    u8 a = static_cast<u8>(std::clamp(rgba.a * 255.0f, 0.0f, 255.0f));
    u8 r = static_cast<u8>(std::clamp(rgba.r * 255.0f, 0.0f, 255.0f));
    u8 g = static_cast<u8>(std::clamp(rgba.g * 255.0f, 0.0f, 255.0f));
    u8 b = static_cast<u8>(std::clamp(rgba.b * 255.0f, 0.0f, 255.0f));
    u32 color =
        (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
    return DustParticleData(color, scale);
}

// ============================================================================
// DustColorTransitionParticleData
// ============================================================================

DustColorTransitionParticleData::DustColorTransitionParticleData(u32 fromColor, u32 toColor, f32 scale)
    : m_fromColor(fromColor)
    , m_toColor(toColor)
    , m_scale(std::clamp(scale, 0.01f, 4.0f))
{}

std::string DustColorTransitionParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::DustColorTransition);
}

std::string DustColorTransitionParticleData::getParameters() const
{
    // 颜色过渡粒子参数格式: fromColor(ARGB) toColor(ARGB) scale
    std::ostringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << m_fromColor << " 0x"
       << std::setw(8) << m_toColor << std::dec << " " << std::fixed << std::setprecision(2) << m_scale;
    return ss.str();
}

std::unique_ptr<ParticleData> DustColorTransitionParticleData::clone() const
{
    return std::make_unique<DustColorTransitionParticleData>(m_fromColor, m_toColor, m_scale);
}

glm::vec4 DustColorTransitionParticleData::fromColorToRGBAVector() const
{
    f32 a = static_cast<f32>((m_fromColor >> 24) & 0xFF) / 255.0f;
    f32 r = static_cast<f32>((m_fromColor >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((m_fromColor >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(m_fromColor & 0xFF) / 255.0f;
    return glm::vec4(r, g, b, a);
}

glm::vec4 DustColorTransitionParticleData::toColorToRGBAVector() const
{
    f32 a = static_cast<f32>((m_toColor >> 24) & 0xFF) / 255.0f;
    f32 r = static_cast<f32>((m_toColor >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((m_toColor >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(m_toColor & 0xFF) / 255.0f;
    return glm::vec4(r, g, b, a);
}

} // namespace mc::client::renderer::trident::particle::data
