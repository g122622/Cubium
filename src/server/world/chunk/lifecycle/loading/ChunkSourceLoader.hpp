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

#pragma once
#include "server/world/chunk/base/completion/ChunkCompletionInbox.hpp"
#include "server/world/chunk/lifecycle/demand/ChunkDemandController.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/spawn/WorldGenSpawner.hpp"
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "common/core/Result.hpp"
#include "server/world/chunk/base/completion/IChunkWorldLifecycle.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"

namespace mc::server {
/** @brief 合并区块来源解析，并把读档结果交接至主线程。 */
class ChunkSourceLoader {
public:
    using Holder = world::chunk::ChunkHolder;
    using Advance = std::function<void(Holder&, const Holder::EnqueueDecision&)>;
    /** @brief 绑定存储访问、发布目标和后续动作。 */
    ChunkSourceLoader(ChunkRegistry& registry,
        IChunkWorldLifecycle& integration,
        std::function<world::storage::SingleLevelStorageManager*()> storage,
        DimensionId dimension,
        Advance advance,
        std::function<void(Holder&)> ready,
        std::function<void(Holder&)> fail);
    /** @brief 对已有需求发起一次异步来源解析。 */
    void resolve(Holder& holder);
    /** @brief 接收入口快照；重入时交给最外层接收者继续推进。 */
    void drain();
    /** @brief 旧实例退场时撤销其加载登记。 */
    void forget(ChunkCoord x, ChunkCoord z, const std::shared_ptr<Holder>& holder);
    /** @brief 停止接收后台结果。 */
    void stop();
    /** @brief 后台任务排空后释放队列和登记。 */
    void clear();
    /** @brief 主线程尚有待交接结果。 */
    bool pending() const;

private:
    struct PendingLoadComplete {
        std::shared_ptr<Holder> holder;
        Result<std::optional<ChunkData>> result;
    };

    void _onChunkLoadComplete(PendingLoadComplete&& completion);

    static u64 posToKey(ChunkCoord x, ChunkCoord z)
    {
        return ChunkPos(x, z).toId();
    }

    ChunkRegistry& m_registry;
    IChunkWorldLifecycle& m_integration;
    std::function<world::storage::SingleLevelStorageManager*()> m_storage;
    DimensionId m_dimension;
    Advance m_advance;
    std::function<void(Holder&)> m_ready, m_fail;
    std::shared_ptr<ChunkCompletionInbox<PendingLoadComplete>> m_completions =
        std::make_shared<ChunkCompletionInbox<PendingLoadComplete>>();
    std::mutex m_pendingLoadTasksMutex;
    std::unordered_map<u64, std::shared_ptr<Holder>> m_pendingLoadTasks;
    bool m_drainingLoadCompletes = false;
};
} // namespace mc::server
