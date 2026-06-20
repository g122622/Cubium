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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cmath>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {

// 前向声明
class IWorld;

/**
 * @brief 末影龙战斗管理器
 *
 * 协调末影龙击杀后的奖励分发，包括龙蛋放置、末地折跃门生成和经验掉落区分。
 * 对齐 MC Java net.minecraft.world.level.dimension.end.EndDragonFight。
 *
 * 生命周期：
 * - 由末地维度的 ServerDimension 持有
 * - 构造时从世界种子初始化折跃门列表
 * - 可从存档数据恢复 previouslyKilled 和 gateways 状态
 * - 龙死亡时通过 setDragonKilled() 触发奖励逻辑
 */
class EndDragonFight {
public:
    /**
     * @brief 战斗数据，用于存档保存/加载
     *
     * 对齐 MC Java EndDragonFight.Data record。
     */
    struct Data {
        bool needsStateScanning = true;           ///< 是否需要扫描旧世界状态
        bool dragonKilled = false;                ///< 龙当前是否已死
        bool previouslyKilled = false;            ///< 是否曾经击杀过龙
        std::optional<std::vector<i32>> gateways; ///< 剩余折跃门索引列表（空=全部消耗，nullopt=首次初始化）

        /**
         * @brief 从 JSON 反序列化
         */
        static Data fromJson(const nlohmann::json& json);

        /**
         * @brief 序列化为 JSON
         */
        [[nodiscard]] nlohmann::json toJson() const;
    };

    // ========== 常量 ==========

    /// 折跃门总数（MC 原版 GATEWAY_COUNT = 20）
    static constexpr i32 GATEWAY_COUNT = 20;

    /// 折跃门距离原点的水平距离（MC 原版 GATEWAY_DISTANCE = 96）
    static constexpr i32 GATEWAY_DISTANCE = 96;

    /// 折跃门的 Y 坐标（MC 原版固定为 75）
    static constexpr i32 GATEWAY_Y = 75;

    // ========== 构造/析构 ==========

    /**
     * @brief 构造末影龙战斗管理器
     *
     * @param worldSeed 世界种子，用于初始化折跃门列表的随机顺序
     * @param data 存档数据，nullopt 表示新世界首次创建
     */
    explicit EndDragonFight(u64 worldSeed, const std::optional<Data>& data = std::nullopt);

    ~EndDragonFight() = default;

    // 禁止拷贝
    EndDragonFight(const EndDragonFight&) = delete;
    EndDragonFight& operator=(const EndDragonFight&) = delete;

    // 允许移动
    EndDragonFight(EndDragonFight&&) noexcept = default;
    EndDragonFight& operator=(EndDragonFight&&) noexcept = default;

    // ========== 核心逻辑 ==========

    /**
     * @brief 末影龙被击杀时调用
     *
     * 执行以下逻辑（对齐 MC Java EndDragonFight.setDragonKilled）：
     * 1. 创建激活态出口传送门
     * 2. 生成一个末地折跃门（如果还有剩余）
     * 3. 首次击杀时在祭坛顶部放置龙蛋
     * 4. 设置 previouslyKilled = true
     *
     * @param world 末地世界引用
     */
    void setDragonKilled(IWorld& world);

    // ========== 状态查询 ==========

    /**
     * @brief 是否曾经击杀过龙
     *
     * 首次击杀时为 false，击杀后永久为 true。
     * 控制龙蛋放置和经验掉落数量。
     */
    [[nodiscard]] bool hasPreviouslyKilled() const { return m_previouslyKilled; }

    /**
     * @brief 龙当前是否已死
     */
    [[nodiscard]] bool isDragonKilled() const { return m_dragonKilled; }

    /**
     * @brief 获取剩余折跃门数量
     */
    [[nodiscard]] i32 remainingGatewayCount() const { return static_cast<i32>(m_gateways.size()); }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 worldSeed() const { return m_worldSeed; }

    // ========== 数据保存 ==========

    /**
     * @brief 保存战斗数据
     *
     * 用于维度存档时序列化战斗状态。
     */
    [[nodiscard]] Data saveData() const;

private:
    /**
     * @brief 从存档数据恢复状态
     */
    void _loadData(const Data& data);

    /**
     * @brief 生成一个新的末地折跃门
     *
     * 从 gateways 列表中取出一个索引，按极坐标公式计算位置，
     * 在该位置放置折跃门结构。
     *
     * @param world 末地世界引用
     */
    void _spawnNewGateway(IWorld& world);

    /**
     * @brief 在指定位置生成折跃门结构
     *
     * @param world 末地世界引用
     * @param pos 折跃门底部中心位置
     */
    static void _spawnNewGatewayAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 在祭坛顶部放置龙蛋
     *
     * 在讲台中心位置的 MOTION_BLOCKING 高度图最高方块上方放置龙蛋方块。
     *
     * @param world 末地世界引用
     */
    static void _placeDragonEgg(IWorld& world);

    // ========== 成员变量 ==========

    u64 m_worldSeed;                  ///< 世界种子
    bool m_previouslyKilled = false;  ///< 是否曾经击杀过龙
    bool m_dragonKilled = false;      ///< 龙当前是否已死
    bool m_needsStateScanning = true; ///< 是否需要扫描旧世界状态
    std::vector<i32> m_gateways;      ///< 剩余折跃门索引列表（随机打乱）
};

} // namespace mc
