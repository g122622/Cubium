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

#include "RedstoneContext.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <atomic>
#include <cstddef>
#include <mutex>

namespace mc {
namespace world {
namespace redstone {

bool RedstoneContext::isUpdating(const BlockPos& pos) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_updatingPositions.count(pos) > 0;
}

void RedstoneContext::beginUpdate(const BlockPos& pos)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.insert(pos);
}

void RedstoneContext::endUpdate(const BlockPos& pos)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.erase(pos);
}

bool RedstoneContext::canPushDepth() const noexcept
{
    return m_depth.load(std::memory_order::relaxed) < MAX_DEPTH;
}

void RedstoneContext::pushDepth() noexcept
{
    m_depth.fetch_add(1, std::memory_order::relaxed);
}

void RedstoneContext::popDepth() noexcept
{
    i32 current = m_depth.load(std::memory_order::relaxed);
    while (current > 0) {
        if (m_depth.compare_exchange_weak(current, current - 1, std::memory_order::relaxed)) {
            break;
        }
    }
}

void RedstoneContext::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.clear();
    m_depth.store(0, std::memory_order::relaxed);
}

size_t RedstoneContext::updatingCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_updatingPositions.size();
}

} // namespace redstone
} // namespace world
} // namespace mc
