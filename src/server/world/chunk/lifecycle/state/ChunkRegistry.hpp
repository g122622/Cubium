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
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/chunk/lifecycle/state/ChunkHolder.hpp"
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::server {
/** @brief 当前区块实例及 FULL 查询索引的唯一拥有者。索引共享同一份 ChunkData。 */
class ChunkRegistry {
public:
    using Holder = mc::world::chunk::ChunkHolder;
    using Data = mc::world::chunk::ChunkData;
    /** @brief 注入创建时的需求级别和安全性变化通知；构造期间不调用。 */
    ChunkRegistry(std::function<i32(ChunkCoord, ChunkCoord)> level, Holder::UnloadStateObserver observer);
    /** @brief 在当前实例表中创建或取得 holder。 */
    Holder& getOrCreate(ChunkCoord x, ChunkCoord z);
    /** @brief 取得当前生命周期实例的共享所有权。 */
    std::shared_ptr<Holder> findShared(ChunkCoord x, ChunkCoord z) const;
    /** @brief 在调用方持有生命周期保护时借用实例。 */
    Holder* find(ChunkCoord x, ChunkCoord z) const;
    /** @brief 查询已发布的 FULL 区块；共享所有权仅保证存活。 */
    std::shared_ptr<Data> chunk(ChunkCoord x, ChunkCoord z) const;
    /** @brief 校验身份后发布 FULL；旧实例的结果不能覆盖新实例。 */
    Data* publish(ChunkCoord x, ChunkCoord z, std::shared_ptr<Data> data, Holder& owner);
    /** @brief 在调度锁保护下移除指定实例及其 FULL 索引。 */
    bool retire(ChunkCoord x, ChunkCoord z, const std::shared_ptr<Holder>& owner);
    /** @brief 在锁外遍历时保留所有对象的共享所有权。 */
    std::vector<std::shared_ptr<Holder>> holders() const;
    /** @brief 已发布 FULL 对象的保活列表。 */
    std::vector<std::shared_ptr<Data>> chunks() const;
    /** @brief 当前 FULL 区块数量。 */
    size_t loadedCount() const;
    /** @brief 当前生命周期实例数量。 */
    size_t holderCount() const;
    /** @brief 交付 holder 已达到目标阶段的请求；回调允许重入。 */
    void completeReady(Holder& holder);
    /** @brief 执行器排空后移走实例和索引，锁外失败剩余请求。 */
    void clear();

private:
    std::function<i32(ChunkCoord, ChunkCoord)> m_level;
    Holder::UnloadStateObserver m_observer;
    mutable std::mutex m_mutex;
    std::unordered_map<u64, std::shared_ptr<Holder>> m_holders;
    std::unordered_map<u64, std::shared_ptr<Data>> m_chunks;
};
} // namespace mc::server
