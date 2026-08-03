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

#include "SimpleSprite.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle {

SimpleSprite::SimpleSprite(const glm::vec2& uvMin, const glm::vec2& uvMax)
    : m_uvMin(uvMin)
    , m_uvMax(uvMax)
{}

glm::vec4 SimpleSprite::getFrameUV(f64 age, f64 maxAge) const
{
    MC_UNUSED(age);
    MC_UNUSED(maxAge);
    return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
}

glm::vec4 SimpleSprite::getRandomFrameUV(u32 seed) const
{
    MC_UNUSED(seed);
    return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
}

} // namespace mc::client::renderer::trident::particle
