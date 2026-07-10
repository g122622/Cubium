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

#include <memory>

#include "common/core/Types.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/tick/base/ScheduledTick.hpp"
#include "common/world/tick/list/ITickList.hpp"

#include <functional>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace mc {

namespace world::tick {

/**
 * @brief 服务端Tick调度列表
 *
 * 管理所有待执行的tick，按时间和优先级排序。
 * 支持序列化以保存到区块数据。
 *
 * 参考: net.minecraft.world.server.ServerTickList
 *
 * 设计要点：
 * 1. 使用std::set按(scheduledTick, priority, tickEntryId)排序
 * 2. 使用std::unordered_set快速查找是否存在调度
 * 3. 每tick最多处理65536个tick
 * 4. 未加载区块的tick会重新调度(delay=0)
 * 5. 线程安全：所有操作应在主线程执行
 *
 * 用法示例:
 * @code
 * // 创建方块tick列表
 * auto blockTicks = std::make_unique<ServerTickList<Block>>(
 *     world,
 *     [](Block& b) { return false; },  // 过滤器
 *     [](Block& b) { return b.blockLocation(); },  // 序列化
 *     [](const ResourceLocation& id) { return Block::getBlock(id); },  // 反序列化
 *     [](IWorld& w, const BlockPos& pos, Block& b) { b.tick(w, pos); }  // tick回调
 * );
 *
 * // 调度一个tick
 * blockTicks->scheduleTick(pos, block, 10, TickPriority::Normal);
 *
 * // 每游戏刻调用
 * blockTicks->tick();
 * @endcode
 *
 * @tparam T 目标类型（Block、Fluid等）
 */
template <typename T>
class ServerTickList : public ITickList<T> {
public:
    /**
     * @brief Tick执行回调类型
     */
    using TickCallback = std::function<void(IWorld&, const BlockPos&, const T&)>;

    /**
     * @brief 过滤器类型，返回true表示忽略该目标
     */
    using Filter = std::function<bool(const T&)>;

    /**
     * @brief 序列化函数类型（目标 -> ResourceLocation）
     */
    using Serializer = std::function<const ResourceLocation&(const T&)>;

    /**
     * @brief 反序列化函数类型（ResourceLocation -> 目标指针）
     */
    using Deserializer = std::function<T*(const ResourceLocation&)>;

    /**
     * @brief 构造服务端tick列表
     *
     * @param world 世界引用
     * @param filter 过滤器，返回true则忽略该目标（如空气方块）
     * @param serializer 序列化函数
     * @param deserializer 反序列化函数
     * @param tickFunction tick执行回调
     */
    ServerTickList(
        IWorld& world, Filter filter, Serializer serializer, Deserializer deserializer, TickCallback tickFunction);

    // ========== ITickList接口实现 ==========

    // 引入基类的默认优先级版本
    using ITickList<T>::scheduleTick;

    [[nodiscard]] bool isTickScheduled(const BlockPos& pos, const T& target) const override;
    [[nodiscard]] bool isTickPending(const BlockPos& pos, const T& target) const override;
    void scheduleTick(const BlockPos& pos, const T& target, i32 delay, TickPriority priority) override;
    bool cancelTick(const BlockPos& pos, const T& target) override;
    [[nodiscard]] size_t pendingCount() const override;

    // ========== 核心方法 ==========

    /**
     * @brief 执行当前游戏刻的所有待处理tick
     *
     * 处理流程：
     * 1. 从TreeSet取出scheduledTick <= currentGameTime的条目（最多65536个）
     * 2. 检查区块是否加载
     * 3. 如果加载，执行tick回调
     * 4. 如果未加载，重新调度(delay=0)
     *
     * @param currentTick 当前游戏刻
     * @param maxTicks 最大处理数量（默认65536）
     */
    void tick(u64 currentTick, size_t maxTicks = 65536);

    /**
     * @brief 获取区块范围内的待处理tick
     *
     * 用于区块序列化保存。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param remove 是否从列表中移除
     * @param skipCompleted 是否跳过已执行的tick
     * @return tick列表
     */
    [[nodiscard]] std::vector<ScheduledTick<T>> getPendingTicks(
        i32 chunkX, i32 chunkZ, bool remove, bool skipCompleted);

    /**
     * @brief 复制tick到新位置
     *
     * 用于区块复制或结构放置。
     *
     * @param minX 最小X
     * @param minY 最小Y
     * @param minZ 最小Z
     * @param maxX 最大X
     * @param maxY 最大Y
     * @param maxZ 最大Z
     * @param offsetX X偏移
     * @param offsetY Y偏移
     * @param offsetZ Z偏移
     */
    void copyTicks(i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 offsetX, i32 offsetY, i32 offsetZ);

    // ========== 统计 ==========

    /**
     * @brief 获取当前tick正在处理的数量
     */
    [[nodiscard]] size_t currentTickCount() const { return m_ticksThisTick.size(); }

    /**
     * @brief 获取已执行的数量（用于调试）
     */
    [[nodiscard]] size_t executedThisTickCount() const { return m_executedThisTick.size(); }

    /**
     * @brief 设置当前游戏刻（用于调度时计算目标时间）
     *
     * @param tick 当前游戏刻
     */
    void setCurrentTick(u64 tick) { mCurrentTick = tick; }

    /**
     * @brief 获取当前游戏刻
     */
    [[nodiscard]] u64 currentTick() const { return mCurrentTick; }

private:
    /**
     * @brief 添加tick条目（内部方法）
     *
     * 如果已存在相同位置和目标的tick，会被新的替换。
     */
    void _addEntry(const ScheduledTick<T>& entry);

    /**
     * @brief 检查位置是否可tick（区块已加载）
     */
    [[nodiscard]] bool _canTick(const BlockPos& pos) const;

    /**
     * @brief 生成新的tick条目ID
     */
    [[nodiscard]] static u64 _nextTickEntryId() noexcept
    {
        static u64 s_nextId = 0;
        return ++s_nextId;
    }

private:
    IWorld& m_world;
    Filter m_filter;             ///< 过滤器
    Serializer m_serializer;     ///< 序列化
    Deserializer m_deserializer; ///< 反序列化
    TickCallback m_tickFunction; ///< tick回调

    // 存储: set用于排序，unordered_set用于快速查找
    std::set<ScheduledTick<T>> m_pendingTicksTree;                                ///< 按时间/优先级排序
    std::unordered_set<ScheduledTick<T>, ScheduledTickHash<T>> m_pendingTicksSet; ///< 快速存在检查

    // 当前tick处理
    std::queue<ScheduledTick<T>> m_ticksThisTick;     ///< 本tick待处理
    std::vector<ScheduledTick<T>> m_executedThisTick; ///< 本tick已执行

    // 当前游戏刻
    u64 mCurrentTick = 0;

    // 调试统计
    size_t m_totalProcessed = 0;
};

// ========== 模板实现 ==========

template <typename T>
ServerTickList<T>::ServerTickList(
    IWorld& world, Filter filter, Serializer serializer, Deserializer deserializer, TickCallback tickFunction)
    : m_world(world)
    , m_filter(std::move(filter))
    , m_serializer(std::move(serializer))
    , m_deserializer(std::move(deserializer))
    , m_tickFunction(std::move(tickFunction))
{}

template <typename T>
bool ServerTickList<T>::isTickScheduled(const BlockPos& pos, const T& target) const
{
    ScheduledTick<T> key(pos, &target, 0, TickPriority::Normal, 0);
    return m_pendingTicksSet.find(key) != m_pendingTicksSet.end();
}

template <typename T>
bool ServerTickList<T>::isTickPending(const BlockPos& pos, const T& target) const
{
    // 检查当前tick待处理队列
    std::queue<ScheduledTick<T>> tempQueue = m_ticksThisTick;
    while (!tempQueue.empty()) {
        const auto& tick = tempQueue.front();
        if (tick.position == pos && tick.target == &target) {
            return true;
        }
        tempQueue.pop();
    }
    return false;
}

template <typename T>
void ServerTickList<T>::scheduleTick(const BlockPos& pos, const T& target, i32 delay, TickPriority priority)
{
    // 检查过滤器
    if (m_filter && m_filter(target)) {
        return;
    }

    // 计算调度时间 = 当前tick + 延迟
    u64 scheduledTick = mCurrentTick + static_cast<u64>(delay > 0 ? delay : 0);

    // 创建新的tick条目
    ScheduledTick<T> entry(pos, &target, scheduledTick, priority, _nextTickEntryId());

    // 如果已存在相同 的 tick，保留旧的，忽略新的
    auto existingIt = m_pendingTicksSet.find(entry);
    if (existingIt != m_pendingTicksSet.end()) {
        return;
    }

    // 添加新的
    _addEntry(entry);
}

template <typename T>
bool ServerTickList<T>::cancelTick(const BlockPos& pos, const T& target)
{
    ScheduledTick<T> key(pos, &target, 0, TickPriority::Normal, 0);

    auto it = m_pendingTicksSet.find(key);
    if (it != m_pendingTicksSet.end()) {
        m_pendingTicksTree.erase(*it);
        m_pendingTicksSet.erase(it);
        return true;
    }
    return false;
}

template <typename T>
size_t ServerTickList<T>::pendingCount() const
{
    return m_pendingTicksTree.size();
}

template <typename T>
void ServerTickList<T>::tick(u64 currentTick, size_t maxTicks)
{
    // 更新当前tick
    mCurrentTick = currentTick;

    // 检查一致性
    if (m_pendingTicksTree.size() != m_pendingTicksSet.size()) {
        // 数据不一致，需要修复
        m_pendingTicksSet.clear();
        for (const auto& tick : m_pendingTicksTree) {
            m_pendingTicksSet.insert(tick);
        }
    }

    // 清空上一tick的数据
    while (!m_ticksThisTick.empty()) {
        m_ticksThisTick.pop();
    }
    m_executedThisTick.clear();

    // 从TreeSet取出scheduledTick <= currentTick的条目
    size_t count = 0;
    auto it = m_pendingTicksTree.begin();
    while (it != m_pendingTicksTree.end() && count < maxTicks) {
        const ScheduledTick<T> tick = *it;

        if (tick.scheduledTick > currentTick) {
            break;
        }

        if (_canTick(tick.position)) {
            m_ticksThisTick.push(tick);
            it = m_pendingTicksTree.erase(it);
            m_pendingTicksSet.erase(tick);
            ++count;
        } else {
            ++it;
        }
    }

    // 执行tick回调
    // 执行前再次检查区块加载状态，未加载则重新调度(delay=0)
    while (!m_ticksThisTick.empty()) {
        ScheduledTick<T> tick = m_ticksThisTick.front();
        m_ticksThisTick.pop();

        if (tick.target != nullptr) {
            if (_canTick(tick.position)) {
                m_executedThisTick.push_back(tick);

                MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Server.Tick,
                    "ExecutingTick",
                    "position",
                    tick.position.toString(),
                    "scheduledTick",
                    tick.scheduledTick,
                    "priority",
                    static_cast<i8>(tick.priority),
                    [flow = ::perfetto::Flow::ProcessScoped(tick.position.toId())](
                        ::perfetto::EventContext ctx) { flow(ctx); });

                m_tickFunction(m_world, tick.position, *tick.target);
                ++m_totalProcessed;
            } else {
                scheduleTick(tick.position, *tick.target, 0, tick.priority);
            }
        }
    }
}

template <typename T>
std::vector<ScheduledTick<T>> ServerTickList<T>::getPendingTicks(
    i32 chunkX, i32 chunkZ, bool remove, bool skipCompleted)
{

    std::vector<ScheduledTick<T>> result;

    // 边界外扩 2 格
    i32 minX = chunkX * CHUNK_WIDTH - 2;
    i32 maxX = (chunkX + 1) * CHUNK_WIDTH + 2;
    i32 minZ = chunkZ * CHUNK_WIDTH - 2;
    i32 maxZ = (chunkZ + 1) * CHUNK_WIDTH + 2;

    // 从TreeSet中收集
    for (auto it = m_pendingTicksTree.begin(); it != m_pendingTicksTree.end();) {
        const ScheduledTick<T>& tick = *it;
        if (tick.position.x >= minX && tick.position.x < maxX && tick.position.z >= minZ && tick.position.z < maxZ) {

            result.push_back(tick);
            if (remove) {
                m_pendingTicksSet.erase(tick);
                it = m_pendingTicksTree.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    // 如果不跳过已完成的，也检查当前tick队列
    if (!skipCompleted) {
        std::queue<ScheduledTick<T>> tempQueue = m_ticksThisTick;
        while (!tempQueue.empty()) {
            const auto& tick = tempQueue.front();
            if (tick.position.x >= minX && tick.position.x < maxX && tick.position.z >= minZ &&
                tick.position.z < maxZ) {
                result.push_back(tick);
            }
            tempQueue.pop();
        }
    }

    return result;
}

template <typename T>
void ServerTickList<T>::copyTicks(
    i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 offsetX, i32 offsetY, i32 offsetZ)
{
    std::vector<ScheduledTick<T>> ticksToCopy;

    // 收集边界框内的tick
    for (const auto& tick : m_pendingTicksTree) {
        if (tick.position.x >= minX && tick.position.x <= maxX && tick.position.y >= minY && tick.position.y <= maxY &&
            tick.position.z >= minZ && tick.position.z <= maxZ) {
            ticksToCopy.push_back(tick);
        }
    }

    // 添加偏移后重新调度
    for (const auto& tick : ticksToCopy) {
        BlockPos newPos(tick.position.x + offsetX, tick.position.y + offsetY, tick.position.z + offsetZ);
        ScheduledTick<T> newTick(newPos, tick.target, tick.scheduledTick, tick.priority, _nextTickEntryId());
        _addEntry(newTick);
    }
}

template <typename T>
void ServerTickList<T>::_addEntry(const ScheduledTick<T>& entry)
{
    m_pendingTicksTree.insert(entry);
    m_pendingTicksSet.insert(entry);
}

template <typename T>
bool ServerTickList<T>::_canTick(const BlockPos& pos) const
{
    // 检查区块是否加载
    return m_world.hasChunk(toChunkCoord(pos.x), toChunkCoord(pos.z));
}

} // namespace world::tick
} // namespace mc
