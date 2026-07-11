/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 */

#include "RuntimeLightTask.hpp"

#include "ServerWorld.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include <fmt/format.h>

namespace mc::server {

RuntimeLightTask::RuntimeLightTask(ServerWorld& world,
    WorldLightManager& manager,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    std::vector<BlockPos> positions)
    : m_world(&world)
    , m_manager(&manager)
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_positions(std::move(positions))
    , m_provider(world, chunkX, chunkZ)
{}

bool RuntimeLightTask::execute(const std::atomic<bool>& abortSignal)
{
    // 任务可能被取消（关服/区块卸载），检查后安全跳过
    if (abortSignal.load(std::memory_order_acquire)) {
        return false;
    }

    // worker 线程执行传播：持 m_mutex 串行化 nibble 写（SWMRNibbleArray 单写者语义）。
    // provider 的 markLightChanged 收集 dirty section 而非触碰主线程回调。
    m_manager->checkBlocksWithProvider(&m_provider, m_chunkX, m_chunkZ, std::move(m_positions));

    // 取出 worker 收集的 dirty section，入主线程 flush 队列。
    // 主线程下一 tick _drainPendingLightFlushes 时调真正的 markLightChanged。
    // 即使任务在传播后被取消，dirty section 仍入队——主线程 markLightChanged 幂等，无副作用。
    auto dirtySections = m_provider.takeDirtySections();
    if (!dirtySections.empty()) {
        m_world->_enqueueLightFlush(std::move(dirtySections));
    }

    return true;
}

std::string RuntimeLightTask::description() const
{
    return fmt::format("RuntimeLightTask(chunk=({},{}), positions={})", m_chunkX, m_chunkZ, m_positions.size());
}

} // namespace mc::server
