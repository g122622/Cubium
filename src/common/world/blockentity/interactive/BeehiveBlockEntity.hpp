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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include "util/math/random/Random.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Player;
class BeeEntity;
class BlockState;

namespace blockentity {

/**
 * @brief 蜜蜂释放状态
 *
 * 蜜蜂从蜂巢释放时的状态，决定释放后的行为。
 */
enum class BeeReleaseStatus : u8 {
    HoneyDelivered, ///< 蜜蜂交付花蜜后释放（增加蜂蜜等级）
    BeeReleased,    ///< 普通释放
    Emergency       ///< 紧急释放（火灾、玩家采蜜等）
};

/**
 * @brief 蜂巢/蜂箱方块实体
 *
 * 管理蜂巢中的蜜蜂数据，处理蜜蜂进入/离开蜂巢、
 * 蜂蜜等级增长、火灾检测和营火烟雾安抚等逻辑。
 *
 * 核心机制：
 * - 最多容纳 3 只蜜蜂
 * - 有花粉的蜜蜂在巢内停留 2400 tick 后释放并增加蜂蜜等级
 * - 无花粉的蜜蜂在巢内停留 600 tick 后释放
 * - 蜂巢着火时紧急释放所有蜜蜂
 * - 营火烟熏下的蜂巢释放蜜蜂不会激怒它们
 */
class BeehiveBlockEntity : public BlockEntity {
public:
    /// 蜂巢最大蜜蜂数量
    static constexpr i32 MAX_OCCUPANTS = 3;

    /// 重新进入蜂巢的最小间隔（tick）
    static constexpr i32 MIN_TICKS_BEFORE_REENTERING_HIVE = 400;

    /// 有花粉时最少在巢时间（tick），约 2 分钟
    static constexpr i32 MIN_OCCUPATION_TICKS_NECTAR = 2400;

    /// 无花粉时最少在巢时间（tick），约 30 秒
    static constexpr i32 MIN_OCCUPATION_TICKS_NECTARLESS = 600;

    /**
     * @brief 蜜蜂居住数据
     *
     * 存储蜜蜂在蜂巢内的状态信息。
     */
    struct BeeOccupant {
        /// 蜜蜂是否有花粉
        bool hasNectar = false;
        /// 已在巢内的 tick 数
        i32 ticksInHive = 0;
        /// 最少停留 tick 数（有花粉=2400，无花粉=600）
        i32 minTicksInHive = MIN_OCCUPATION_TICKS_NECTARLESS;

        /**
         * @brief 递增停留时间
         * @return 是否已超过最少停留时间（可以释放）
         */
        bool tick() { return ++ticksInHive > minTicksInHive; }
    };

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BeehiveBlockEntity(const BlockPos& pos);

    ~BeehiveBlockEntity() override = default;

    // ========== BlockEntity 接口 ==========

    /**
     * @brief 每tick更新
     * @param world 所在世界
     *
     * 更新蜜蜂停留时间，超时则释放蜜蜂。
     */
    void tick(IWorld& world) override;

    /**
     * @brief 检查是否需要tick
     * @return 有蜜蜂时返回true
     */
    [[nodiscard]] bool needsTick() const noexcept override { return !m_bees.empty(); }

    /**
     * @brief 从JSON加载数据
     */
    bool load(const nlohmann::json& data) override;

    /**
     * @brief 保存数据到JSON
     */
    void save(nlohmann::json& data) const override;

    /**
     * @brief 创建副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 蜜蜂管理 ==========

    /**
     * @brief 蜜蜂进入蜂巢
     * @param bee 蜜蜂实体
     * @return 是否成功进入（蜂巢已满则返回false）
     *
     * 将蜜蜂数据存入蜂巢，从世界移除蜜蜂实体。
     * 有花粉时 minTicksInHive = 2400，无花粉时 = 600。
     */
    bool addOccupant(BeeEntity& bee);

    /**
     * @brief 释放所有蜜蜂
     * @param world 世界引用
     * @param player 触发释放的玩家（可能为nullptr）
     * @param state 当前方块状态
     * @param releaseStatus 释放状态
     *
     * 释放蜂巢内所有蜜蜂。如果玩家在 4 格内且蜂巢未被营火安抚，
     * 释放的蜜蜂会攻击玩家。
     */
    void emptyAllLivingFromHive(IWorld& world, Player* player, const BlockState& state, BeeReleaseStatus releaseStatus);

    /**
     * @brief 蜂巢是否已满
     * @return 蜜蜂数量是否达到上限（3只）
     */
    [[nodiscard]] bool isFull() const { return static_cast<i32>(m_bees.size()) >= MAX_OCCUPANTS; }

    /**
     * @brief 蜂巢是否为空
     */
    [[nodiscard]] bool isEmpty() const { return m_bees.empty(); }

    /**
     * @brief 获取蜜蜂数量
     */
    [[nodiscard]] i32 getOccupantCount() const { return static_cast<i32>(m_bees.size()); }

    /**
     * @brief 获取保存的花朵位置
     */
    [[nodiscard]] const BlockPos& getSavedFlowerPos() const { return m_savedFlowerPos; }

    /**
     * @brief 设置保存的花朵位置
     */
    void setSavedFlowerPos(const BlockPos& pos) { m_savedFlowerPos = pos; }

    // ========== 环境检测 ==========

    /**
     * @brief 检查蜂巢周围是否有火
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @return 3x3x3 范围内是否有火
     */
    [[nodiscard]] static bool isFireNearby(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查蜂巢是否被营火烟熏安抚
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @return 蜂巢下方是否有点燃的营火
     *
     * 被烟熏的蜜蜂释放时不会攻击玩家。
     */
    [[nodiscard]] static bool isSedated(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查蜂巢中是否有蜜蜂
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @return 蜂巢中是否至少有一只蜜蜂
     */
    [[nodiscard]] static bool hiveContainsBees(IWorld& world, const BlockPos& pos);

    /**
     * @brief 激怒附近的蜜蜂
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @param player 触发的玩家
     *
     * 在 8x6x8 范围内搜索蜜蜂实体，使其攻击玩家。
     */
    static void angerNearbyBees(IWorld& world, const BlockPos& pos, Player& player);

private:
    /**
     * @brief 释放单只蜜蜂
     * @param world 世界引用
     * @param state 当前方块状态
     * @param occupantIndex 蜜蜂在列表中的索引
     * @param releaseStatus 释放状态
     * @return 是否成功释放（天气/夜间阻止或出口被阻挡时返回false）
     *
     * 根据释放状态处理不同行为：
     * - HoneyDelivered: 交付花蜜，增加蜂蜜等级
     * - BeeReleased: 普通释放
     * - Emergency: 紧急释放，忽略天气/出口阻挡
     *
     * 天气/夜间检查：非紧急释放时，雨天/雷暴/夜间蜜蜂留在巢内，
     * 返回 false 表示未释放，蜜蜂保留在列表中待下次重试。
     */
    bool _releaseOccupant(IWorld& world, const BlockState& state, i32 occupantIndex, BeeReleaseStatus releaseStatus);

    /**
     * @brief 更新所有蜜蜂的停留时间
     * @param world 世界引用
     *
     * 遍历所有蜜蜂，ticksInHive++，超过 minTicksInHive 时释放。
     */
    void _tickOccupants(IWorld& world);

    /**
     * @brief 计算蜜蜂释放位置
     * @param world 世界引用
     * @param pos 蜂巢位置
     * @param state 方块状态
     * @return 释放位置，如果被阻挡返回空
     *
     * 根据蜂巢朝向计算出口位置，如果出口被阻挡则尝试其他方向。
     */
    [[nodiscard]] static std::optional<BlockPos> _getReleasePosition(
        IWorld& world, const BlockPos& pos, const BlockState& state);

    /// 存储的蜜蜂列表
    std::vector<BeeOccupant> m_bees;

    /// 保存的花朵位置（蜜蜂带来的花朵信息）
    BlockPos m_savedFlowerPos;

    /// 随机数生成器
    mutable math::Random m_rng;
};

} // namespace blockentity
} // namespace mc
