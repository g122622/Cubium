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
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc {

class IWorld;

namespace server {
class ServerWorld;
} // namespace server

namespace server::spawn {

/**
 * @brief 流浪商人自然生成器
 *
 * 每 1200 tick（1 分钟）递减一次生成倒计时；倒计时归零后重置为 24000 tick，
 * 并按逐渐提高的概率（25% → 50% → 75%，成功后重置 25%）尝试生成。真正尝试生成时
 * 还有 1/10 门槛，且需在距玩家 48 格内的地表找到可站立位置。
 *
 * 生成成功：在玩家附近生成流浪商人，并在其周围生成 2 只拴绳贸易羊驼，
 * 商人妥善消失时间 48000 tick、游荡目标为玩家/会议点位置。
 *
 * 与 vanilla WanderingTraderSpawner 的差异：
 * - POI 会议点检索尚未实现（项目无 POI 管理器），以玩家位置作为游荡目标；
 * - 生成倒计时/概率/商人 UUID 的关卡数据持久化尚未接入，重启世界后从默认值开始。
 *   TODO: 待关卡数据（ServerLevelData 等价物）支持自定义字段后持久化这三项。
 */
class WanderingTraderSpawner {
public:
    /// 递减间隔（tick）
    static constexpr i32 TICK_DELAY = 1200;
    /// 倒计时归零后的重置值（tick，= 1 游戏日）
    static constexpr i32 DEFAULT_SPAWN_DELAY = 24000;
    /// 概率初始值与上下限（百分比）
    static constexpr i32 INITIAL_SPAWN_CHANCE = 25;
    static constexpr i32 MIN_SPAWN_CHANCE = 25;
    static constexpr i32 MAX_SPAWN_CHANCE = 75;
    static constexpr i32 SPAWN_CHANCE_INCREASE = 25;
    /// 真正尝试生成前额外的 1/N 门槛
    static constexpr i32 SPAWN_ONE_IN_X_CHANCE = 10;
    /// 距目标中心的搜索半径（格）
    static constexpr i32 SPAWN_RADIUS = 48;
    /// 选址尝试次数
    static constexpr i32 SPAWN_ATTEMPTS = 10;
    /// 羊驼数量与选址半径
    static constexpr i32 LLAMA_COUNT = 2;
    static constexpr i32 LLAMA_SPAWN_RADIUS = 4;
    /// 商人消失延迟（tick）
    static constexpr i32 DESPAWN_DELAY = 48000;

    WanderingTraderSpawner();

    /**
     * @brief 每 tick 更新
     *
     * 受 doTraderSpawning 游戏规则门控；规则关闭时状态保持不变。
     *
     * @param world 服务端世界（仅主世界应调用）
     */
    void tick(server::ServerWorld& world);

    /**
     * @brief 立即尝试生成一次流浪商人（忽略倒计时，但仍受概率与选址约束）
     *
     * 供测试与调试入口使用，成功与否通过返回值报告。
     *
     * @param world 服务端世界
     * @return true 如果本次生成了流浪商人
     */
    bool trySpawn(server::ServerWorld& world);

    [[nodiscard]] i32 tickDelay() const noexcept
    {
        return m_tickDelay;
    }

    [[nodiscard]] i32 spawnDelay() const noexcept
    {
        return m_spawnDelay;
    }

    [[nodiscard]] i32 spawnChance() const noexcept
    {
        return m_spawnChance;
    }

private:
    /**
     * @brief 在 center 附近寻找可站立的生成位置
     *
     * @param world 世界
     * @param center 搜索中心
     * @param radius 搜索半径
     * @return 可站立位置；未找到返回空
     */
    [[nodiscard]] std::optional<BlockPos> _findSpawnPositionNear(
        server::ServerWorld& world, const BlockPos& center, i32 radius);

    /**
     * @brief 检查以 pos 为角的 2x3x2 区域是否无碰撞
     */
    [[nodiscard]] static bool _hasEnoughSpace(IWorld& world, const BlockPos& pos);

    i32 m_tickDelay;
    i32 m_spawnDelay;
    i32 m_spawnChance;
};

} // namespace server::spawn
} // namespace mc
