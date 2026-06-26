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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
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
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {

// 前向声明
class IWorld;

/**
 * @brief 末影龙战斗管理器
 *
 * 协调末影龙击杀后的奖励分发，包括龙蛋放置、末地折跃门生成和经验掉落区分。
 * 管理末影龙存活状态检测、旧存档状态扫描和末影龙 UUID 追踪。
 *
 * 生命周期：
 * - 由末地维度的 ServerWorld 持有
 * - 构造时从世界种子初始化折跃门列表
 * - 可从存档数据恢复 previouslyKilled、dragonKilled 和 dragonUUID 状态
 * - 龙死亡时通过 setDragonKilled() 触发奖励逻辑
 * - 每个游戏 tick 通过 tick() 更新状态（含旧存档状态扫描）
 *
 * 对应 MC Java: EndDragonFight
 */
class EndDragonFight {
public:
    /**
     * @brief 战斗数据，用于存档保存/加载
     */
    struct Data {
        bool needsStateScanning = true;           ///< 是否需要扫描旧世界状态
        bool dragonKilled = false;                ///< 龙当前是否已死
        bool previouslyKilled = false;            ///< 是否曾经击杀过龙
        std::optional<std::string> dragonUUID;    ///< 末影龙的 UUID（nullopt 表示无龙或未追踪）
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

    /// 折跃门总数
    static constexpr i32 GATEWAY_COUNT = 20;

    /// 折跃门距离原点的水平距离
    static constexpr i32 GATEWAY_DISTANCE = 96;

    /// 折跃门的 Y 坐标
    static constexpr i32 GATEWAY_Y = 75;

    /// 竞技场区块扫描半径（原点周围 -8 到 +8 区块）
    static constexpr i32 ARENA_CHUNK_RADIUS = 8;

    // ========== 构造/析构 ==========

    /**
     * @brief 构造末影龙战斗管理器
     *
     * @param worldSeed 世界种子，用于初始化折跃门列表的随机顺序
     * @param data 存档数据，nullopt 表示新世界首次创建
     */
    explicit EndDragonFight(u64 worldSeed, const std::optional<Data>& data);

    ~EndDragonFight() = default;

    // 禁止拷贝
    EndDragonFight(const EndDragonFight&) = delete;
    EndDragonFight& operator=(const EndDragonFight&) = delete;

    // 允许移动
    EndDragonFight(EndDragonFight&&) noexcept = default;
    EndDragonFight& operator=(EndDragonFight&&) noexcept = default;

    // ========== 核心逻辑 ==========

    /**
     * @brief 每游戏 tick 调用
     *
     * 处理旧存档状态扫描：当 needsStateScanning 为 true 且竞技场区块已加载时，
     * 扫描出口传送门是否存在以推断 previouslyKilled 状态，并检测末影龙是否存活。
     *
     * @param world 末地世界引用
     */
    void tick(IWorld& world);

    /**
     * @brief 末影龙被击杀时调用
     *
     * 执行以下逻辑：
     * 1. 创建激活态出口传送门
     * 2. 生成一个末地折跃门（如果还有剩余）
     * 3. 首次击杀时在祭坛顶部放置龙蛋
     * 4. 设置 previouslyKilled = true
     * 5. 清空末影龙 UUID（龙已死亡，不再追踪）
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
     * @brief 扫描旧存档状态
     *
     * 检查出口传送门是否存在来推断 previouslyKilled：
     * - 如果存在活跃的出口传送门（END_PORTAL 方块），则 previouslyKilled = true
     * - 如果不存在活跃出口传送门，则 previouslyKilled = false，
     *   并检查是否存在讲台结构；如果不存在则创建非激活讲台
     *
     * 同时检查末影龙实体是否仍存活：
     * - 如果没有末影龙实体：dragonKilled = true
     * - 如果有末影龙实体：记录 dragonUUID，dragonKilled = false
     *   - 若同时无活跃传送门，则丢弃该龙（discard），因为无传送门的龙是无效状态
     * - 最终安全检查：如果 !previouslyKilled && dragonKilled，则 dragonKilled = false
     *
     * 对应 MC Java: EndDragonFight.scanState()
     *
     * @param world 末地世界引用
     */
    void _scanState(IWorld& world);

    /**
     * @brief 检查是否存在活跃的出口传送门
     *
     * 扫描原点周围区块，查找 END_PORTAL 方块。
     * 活跃出口传送门表明龙曾被击杀过。
     *
     * @param world 末地世界引用
     * @return true 如果找到活跃出口传送门
     */
    [[nodiscard]] static bool _hasActiveExitPortal(IWorld& world);

    /**
     * @brief 检查竞技场区块是否已全部加载
     *
     * 检查原点周围 ARENA_CHUNK_RADIUS 范围内的区块是否已加载，
     * 只有区块加载后才能进行状态扫描。
     *
     * @param world 末地世界引用
     * @return true 如果竞技场区块已加载
     */
    [[nodiscard]] static bool _isArenaLoaded(IWorld& world);

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
    std::string m_dragonUUID;         ///< 末影龙的 UUID（空字符串表示无龙或未追踪）
    std::vector<i32> m_gateways;      ///< 剩余折跃门索引列表（随机打乱）

    // TODO: 末影龙重生系统尚未实现。需要：
    // 1. 末影水晶管理：检测末地黑曜石柱上的末影水晶放置/破坏
    // 2. 重生序列：4个末影水晶同时存在时启动重生动画（ crystals -> beam -> dragon spawn ）
    // 3. 重生计时器：重生过程约 10 秒，期间需播放粒子效果和光柱动画
    // 4. 重生后状态重置：dragonKilled = false, needsStateScanning = false
};

} // namespace mc
