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

#include "common/core/Types.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/base/ScheduledTick.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/list/EmptyTickList.hpp"
#include "common/world/tick/list/ITickList.hpp"
#include "common/world/tick/list/ServerTickList.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc::world::tick {

/**
 * @brief Tick管理器
 *
 * 统一管理方块和流体的计划刻调度。
 * 作为外观类封装ServerTickList，提供简洁的API。
 *
 * 架构说明:
 * - 服务端: 使用ServerTickList<Block>和ServerTickList<Fluid>进行实际调度
 * - 客户端: 使用EmptyTickList，tick由服务端同步
 *
 * 用法示例:
 * @code
 * // 调度方块tick
 * tickManager.scheduleBlockTick(pos, block, 10);  // 10tick后执行
 *
 * // 调度流体tick
 * tickManager.scheduleFluidTick(pos, fluid, 5, TickPriority::High);
 *
 * // 每游戏刻调用
 * tickManager.tick(currentTick);
 * @endcode
 */
class TickManager {
public:
    /**
     * @brief 构造TickManager
     *
     * @param world IWorld引用
     */
    explicit TickManager(IWorld& world);

    /**
     * @brief 析构函数
     */
    ~TickManager() noexcept;

    // ========== 方块tick调度 ==========

    /**
     * @brief 调度方块tick（普通优先级）
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @param delay 延迟tick数
     */
    void scheduleBlockTick(const BlockPos& pos, const Block& block, i32 delay);

    /**
     * @brief 调度方块tick（指定优先级）
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @param delay 延迟tick数
     * @param priority 执行优先级
     */
    void scheduleBlockTick(const BlockPos& pos, const Block& block, i32 delay, TickPriority priority);

    /**
     * @brief 检查方块tick是否已调度
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @return 是否已调度
     */
    [[nodiscard]] bool isBlockTickScheduled(const BlockPos& pos, const Block& block) const;

    /**
     * @brief 检查方块tick是否在本tick待执行
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @return 是否本tick待执行
     */
    [[nodiscard]] bool isBlockTickPending(const BlockPos& pos, const Block& block) const;

    /**
     * @brief 取消方块tick
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @return 是否成功取消
     */
    bool cancelBlockTick(const BlockPos& pos, const Block& block);

    // ========== 流体tick调度 ==========

    /**
     * @brief 调度流体tick（普通优先级）
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @param delay 延迟tick数
     */
    void scheduleFluidTick(const BlockPos& pos, const fluid::Fluid& fluid, i32 delay);

    /**
     * @brief 调度流体tick（指定优先级）
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @param delay 延迟tick数
     * @param priority 执行优先级
     */
    void scheduleFluidTick(const BlockPos& pos, const fluid::Fluid& fluid, i32 delay, TickPriority priority);

    /**
     * @brief 检查流体tick是否已调度
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @return 是否已调度
     */
    [[nodiscard]] bool isFluidTickScheduled(const BlockPos& pos, const fluid::Fluid& fluid) const;

    /**
     * @brief 检查流体tick是否在本tick待执行
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @return 是否本tick待执行
     */
    [[nodiscard]] bool isFluidTickPending(const BlockPos& pos, const fluid::Fluid& fluid) const;

    /**
     * @brief 取消流体tick
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @return 是否成功取消
     */
    bool cancelFluidTick(const BlockPos& pos, const fluid::Fluid& fluid);

    // ========== 执行tick ==========

    /**
     * @brief 执行当前游戏刻的所有待处理tick
     *
     * 按以下顺序执行:
     * 1. 方块计划刻（最多65536个）
     * 2. 流体计划刻（最多65536个）
     *
     * @param currentTick 当前游戏刻
     */
    void tick(u64 currentTick);

    // ========== 区块序列化 ==========

    /**
     * @brief 获取区块范围内的待处理方块tick
     *
     * 用于区块序列化保存。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param remove 是否从列表中移除
     * @return tick列表
     */
    [[nodiscard]] std::vector<ScheduledTick<Block>> getPendingBlockTicks(i32 chunkX, i32 chunkZ, bool remove);

    /**
     * @brief 获取区块范围内的待处理流体tick
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param remove 是否从列表中移除
     * @return tick列表
     */
    [[nodiscard]] std::vector<ScheduledTick<fluid::Fluid>> getPendingFluidTicks(i32 chunkX, i32 chunkZ, bool remove);

    // ========== 统计 ==========

    /**
     * @brief 获取待处理方块tick数量
     */
    [[nodiscard]] size_t pendingBlockTickCount() const;

    /**
     * @brief 获取待处理流体tick数量
     */
    [[nodiscard]] size_t pendingFluidTickCount() const;

    /**
     * @brief 获取本tick已执行的方块tick数量
     */
    [[nodiscard]] size_t executedBlockTickCount() const;

    /**
     * @brief 获取本tick已执行的流体tick数量
     */
    [[nodiscard]] size_t executedFluidTickCount() const;

    // ========== 直接访问（高级用法） ==========

    /**
     * @brief 获取方块tick列表
     */
    [[nodiscard]] ServerTickList<Block>& blockTicks() { return *m_blockTicks; }
    [[nodiscard]] const ServerTickList<Block>& blockTicks() const { return *m_blockTicks; }

    /**
     * @brief 获取流体tick列表
     */
    [[nodiscard]] ServerTickList<fluid::Fluid>& fluidTicks() { return *m_fluidTicks; }
    [[nodiscard]] const ServerTickList<fluid::Fluid>& fluidTicks() const { return *m_fluidTicks; }

private:
    IWorld& m_world;
    std::unique_ptr<ServerTickList<Block>> m_blockTicks;
    std::unique_ptr<ServerTickList<fluid::Fluid>> m_fluidTicks;
};

} // namespace mc::world::tick
