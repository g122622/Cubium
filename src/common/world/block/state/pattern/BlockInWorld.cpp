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

#include "BlockInWorld.hpp"

#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"

namespace mc::blockpattern {

const BlockState* BlockInWorld::getState() const
{
    if (!m_stateCached) {
        if (m_loadChunks || m_world.hasChunk(m_pos.chunkX(), m_pos.chunkZ())) {
            m_state = m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
        }
        m_stateCached = true;
    }
    return m_state;
}

BlockEntity* BlockInWorld::getEntity() const
{
    if (!m_entityCached) {
        m_entity = m_world.getBlockEntity(m_pos);
        m_entityCached = true;
    }
    return m_entity;
}

std::function<bool(const BlockInWorld&)> BlockInWorld::hasState(std::function<bool(const BlockState&)> predicate)
{
    return [pred = std::move(predicate)](const BlockInWorld& biw) -> bool {
        const BlockState* state = biw.getState();
        return state != nullptr && pred(*state);
    };
}

} // namespace mc::blockpattern
