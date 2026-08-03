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
#include "common/world/village/VillageGossip.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

// 前向声明
class IWorld;
class Entity;
class Player;

namespace world {
namespace village {

/**
 * @brief 村庄配置常量
 */
struct VillageConfig {
    /// 村庄基础半径
    static constexpr f32 BASE_RADIUS = 64.0f;

    /// 村庄半径扩展（每个床位增加）
    static constexpr f32 RADIUS_PER_BED = 2.0f;

    /// 最大村庄半径
    static constexpr f32 MAX_RADIUS = 128.0f;

    /// 村民搜索床位范围
    static constexpr f32 BED_SEARCH_RANGE = 48.0f;

    /// 村民搜索工作站范围
    static constexpr f32 WORKSTATION_SEARCH_RANGE = 48.0f;

    /// 袭击触发范围
    static constexpr f32 RAID_TRIGGER_RANGE = 96.0f;

    /// 流言衰减间隔（tick）
    static constexpr i64 GOSSIP_DECAY_INTERVAL = 24000;

    /// 村民繁殖阈值（每个村民需要的床位）
    static constexpr f32 BEDS_PER_VILLAGER = 1.0f;
};

/**
 * @brief 村庄ID类型
 */
using VillageId = u64;

/**
 * @brief 村庄
 *
 * 表示世界中的一个村庄实例，管理：
 * - 村庄边界（基于床位和工作站动态计算）
 * - 村民列表
 * - 流言/声誉系统
 * - 铃铛（聚集点）
 * - 袭击状态
 */
class Village {
public:
    /**
     * @brief 构造函数
     * @param center 村庄中心位置
     */
    explicit Village(BlockPos center);

    // ========== ID 管理 ==========

    /**
     * @brief 获取村庄ID
     */
    [[nodiscard]] VillageId getId() const noexcept { return m_id; }

    /**
     * @brief 设置村庄ID（仅由 VillageManager 调用）
     * @param id 村庄ID
     */
    void setId(VillageId id) noexcept { m_id = id; }

    // ========== 边界管理 ==========

    /**
     * @brief 获取村庄中心
     */
    [[nodiscard]] BlockPos getCenter() const noexcept { return m_center; }

    /**
     * @brief 获取村庄半径
     */
    [[nodiscard]] f32 getRadius() const noexcept { return m_radius; }

    /**
     * @brief 检查位置是否在村庄内
     *
     * 当前使用 3D 欧几里得距离判断（中心点+半径模型），
     * TODO: 实现 Section 距离传播系统，将 isWithinVillage 改为基于 POI 密度
     * 的区域判定，使村庄边界更准确地反映 POI 分布而非简单圆形范围。
     *
     * @param pos 方块位置
     * @return 是否在村庄范围内
     */
    [[nodiscard]] bool isWithinVillage(BlockPos pos) const;

    /**
     * @brief 检查位置是否在袭击触发范围内
     */
    [[nodiscard]] bool isWithinRaidTrigger(BlockPos pos) const;

    /**
     * @brief 重新计算村庄边界
     * @param poiStorage POI存储（用于计算床位和工作站分布）
     */
    void recalculateBounds(const poi::PointOfInterestStorage& poiStorage);

    // ========== 村民管理 ==========

    /**
     * @brief 添加村民到村庄
     * @param villagerId 村民实体ID
     */
    void addVillager(u64 villagerId);

    /**
     * @brief 从村庄移除村民
     * @param villagerId 村民实体ID
     */
    void removeVillager(u64 villagerId);

    /**
     * @brief 检查村民是否属于此村庄
     */
    [[nodiscard]] bool hasVillager(u64 villagerId) const;

    /**
     * @brief 获取所有村民ID
     */
    [[nodiscard]] const std::unordered_set<u64>& getVillagers() const noexcept { return m_villagers; }

    /**
     * @brief 获取村民数量
     */
    [[nodiscard]] i32 getPopulation() const noexcept { return static_cast<i32>(m_villagers.size()); }

    // ========== 床位和繁殖 ==========

    /**
     * @brief 获取床位数量
     */
    [[nodiscard]] i32 getBedCount() const noexcept { return m_bedCount; }

    /**
     * @brief 获取可用床位数（床位数 - 村民数）
     */
    [[nodiscard]] i32 getAvailableBeds() const;

    /**
     * @brief 更新床位计数
     * @param count 新的床位数量
     */
    void setBedCount(i32 count) noexcept { m_bedCount = count; }

    /**
     * @brief 检查村庄是否可以繁殖更多村民
     */
    [[nodiscard]] bool canBreed() const;

    // ========== 工作站 ==========

    /**
     * @brief 获取工作站数量
     */
    [[nodiscard]] i32 getWorkstationCount() const noexcept { return m_workstationCount; }

    /**
     * @brief 更新工作站计数
     */
    void setWorkstationCount(i32 count) noexcept { m_workstationCount = count; }

    // ========== 聚集点 ==========

    /**
     * @brief 获取聚集点（钟的位置）
     */
    [[nodiscard]] std::optional<BlockPos> getMeetingPoint() const noexcept { return m_meetingPoint; }

    /**
     * @brief 设置聚集点
     */
    void setMeetingPoint(BlockPos pos) noexcept { m_meetingPoint = pos; }

    /**
     * @brief 检查是否有聚集点
     */
    [[nodiscard]] bool hasMeetingPoint() const noexcept { return m_meetingPoint.has_value(); }

    // ========== 流言/声誉系统 ==========

    /**
     * @brief 获取流言管理器
     */
    [[nodiscard]] VillageGossipManager& getGossipManager() noexcept { return m_gossipManager; }
    [[nodiscard]] const VillageGossipManager& getGossipManager() const noexcept { return m_gossipManager; }

    /**
     * @brief 添加流言（便捷方法）
     * @param playerId 玩家ID
     * @param type 流言类型
     * @param value 流言值
     */
    void addGossip(u64 playerId, VillageGossipType type, i32 value)
    {
        m_gossipManager.addGossip(playerId, type, value);
    }

    /**
     * @brief 获取玩家声誉
     */
    [[nodiscard]] i32 getPlayerReputation(u64 playerId) const { return m_gossipManager.getReputation(playerId); }

    /**
     * @brief 获取价格修正因子
     */
    [[nodiscard]] f32 getPriceModifier(u64 playerId) const { return m_gossipManager.getPriceModifier(playerId); }

    // ========== 袭击状态 ==========

    /**
     * @brief 检查村庄是否正在被袭击
     */
    [[nodiscard]] bool isUnderRaid() const noexcept { return m_underRaid; }

    /**
     * @brief 设置袭击状态
     */
    void setUnderRaid(bool underRaid) noexcept { m_underRaid = underRaid; }

    /**
     * @brief 获取上次袭击时间
     */
    [[nodiscard]] i64 getLastRaidTime() const noexcept { return m_lastRaidTime; }

    /**
     * @brief 设置上次袭击时间
     */
    void setLastRaidTime(i64 time) noexcept { m_lastRaidTime = time; }

    // ========== Tick更新 ==========

    /**
     * @brief 每游戏tick更新
     * @param world 世界接口
     * @param gameTime 当前游戏时间
     * @param poiStorage POI存储（用于更新POI统计和工作站绑定）
     */
    void tick(IWorld& world, i64 gameTime, poi::PointOfInterestStorage* poiStorage);

    // ========== 常量 ==========

    /// POI 统计更新间隔（每 1200 tick = 1 分钟更新一次）
    static constexpr i64 POI_STAT_UPDATE_INTERVAL = 1200;

    /// 袭击状态验证间隔（每 20 tick 验证一次）
    static constexpr i64 RAID_CHECK_INTERVAL = 20;

    /// 村民超时时间（6000 tick = 5 分钟不在村庄范围内视为离开）
    /// 村民不会因为距离远而消失，但本项目的 Village 类维护显式村民列表，
    /// 超时机制用于清理已离开村庄的村民关联。
    /// 村民主动死亡/移除时通过 VillagerEntity::releaseAllPois() 立即通知村庄。
    static constexpr i64 VILLAGER_TIMEOUT = 6000;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     */
    static Village deserialize(const nbt::tags::compound_tag& tag);

private:
    /**
     * @brief 检查并更新村民列表
     *
     * 移除离开村庄范围超过指定时间的村民，并释放其占用的 POI。
     *
     * @param world 世界接口（用于获取实体）
     * @param gameTime 当前游戏时间
     * @param poiStorage POI存储（用于释放离开村民的占用）
     */
    void _tickVillagerCheck(IWorld& world, i64 gameTime, poi::PointOfInterestStorage* poiStorage);

    /**
     * @brief 更新 POI 统计
     *
     * 更新床位和工作站计数，以及聚集点（钟）。
     *
     * @param poiStorage POI存储
     */
    void _tickPOIStats(const poi::PointOfInterestStorage& poiStorage);

    /**
     * @brief 检查并同步袭击状态
     *
     * 通过 RaidManager 验证村庄的 m_underRaid 标志与实际袭击状态一致。
     * 正常情况下 RaidManager::onRaidEnd() 会同步更新此标志，
     * 此方法作为防御性冗余检查，防止因异常导致标志不同步。
     *
     * @param world 世界接口（用于获取 RaidManager）
     * @param gameTime 当前游戏时间
     */
    void _tickRaidCheck(IWorld& world, i64 gameTime);

    /**
     * @brief 查找聚集点（钟）
     *
     * 在村庄范围内搜索钟 POI。
     *
     * @param poiStorage POI存储
     * @return 如果找到返回钟的位置，否则返回空
     */
    [[nodiscard]] std::optional<BlockPos> _findMeetingPoint(const poi::PointOfInterestStorage& poiStorage) const;

    /// 村庄ID（由 VillageManager 分配）
    VillageId m_id = 0;

    /// 村庄中心位置
    BlockPos m_center;

    /// 村庄半径
    f32 m_radius = VillageConfig::BASE_RADIUS;

    /// 村民ID集合
    std::unordered_set<u64> m_villagers;

    /// 床位数量
    i32 m_bedCount = 0;

    /// 工作站数量
    i32 m_workstationCount = 0;

    /// 聚集点（钟的位置）
    std::optional<BlockPos> m_meetingPoint;

    /// 流言管理器
    VillageGossipManager m_gossipManager;

    /// 是否正在被袭击
    bool m_underRaid = false;

    /// 上次袭击时间
    i64 m_lastRaidTime = 0;

    /// 村庄创建时间
    i64 m_createdTime = 0;

    /// 村民最后出现时间（用于范围检查）
    /// 保留此映射用于优化，避免每tick都检查所有村民
    std::unordered_map<u64, i64> m_villagerLastSeenTime;

    /// 上次 POI 统计更新时间
    i64 m_lastPOIStatUpdateTime = 0;

    /// 上次袭击状态验证时间
    i64 m_lastRaidCheckTime = 0;
};

} // namespace village
} // namespace world
} // namespace mc
