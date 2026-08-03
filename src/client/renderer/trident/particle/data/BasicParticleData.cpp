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

#include "BasicParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

BasicParticleData::BasicParticleData(ParticleTypeId type)
    : m_type(type)
{
    MC_ASSERT(isValidParticleType(type));
}

BasicParticleData::BasicParticleData(const std::string& typeName)
{
    auto id = ParticleRegistry::instance().getTypeId(typeName);
    if (id.has_value()) {
        m_type = id.value();
    } else {
        m_type = ParticleTypeId::Invalid;
    }
}

std::string BasicParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::unique_ptr<ParticleData> BasicParticleData::clone() const
{
    return std::make_unique<BasicParticleData>(m_type);
}

} // namespace mc::client::renderer::trident::particle::data
