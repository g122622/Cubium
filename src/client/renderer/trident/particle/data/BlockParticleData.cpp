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

#include "BlockParticleData.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/ParticleData.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc::client::renderer::trident::particle::data {

BlockParticleData::BlockParticleData(ParticleTypeId type, BlockState blockState)
    : m_type(type)
    , m_blockState(std::move(blockState))
{
    MC_ASSERT_RELEASE_MSG(requiresBlockState(type), "BlockParticleData requires a block-type particle");
}

std::string BlockParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::string BlockParticleData::getParameters() const
{
    // 方块粒子参数格式: block_state
    // 例如: minecraft:stone
    std::string result = m_blockState.blockLocation().toString();
    const std::string modelKey = m_blockState.toModelKey();
    if (!modelKey.empty()) {
        result += "[";
        result += modelKey;
        result += "]";
    }
    return result;
}

std::unique_ptr<ParticleData> BlockParticleData::clone() const
{
    return std::make_unique<BlockParticleData>(m_type, m_blockState);
}

} // namespace mc::client::renderer::trident::particle::data
