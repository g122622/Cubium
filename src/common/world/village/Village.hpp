#pragma once

#include "VillageGossip.hpp"
#include "poi/PointOfInterestStorage.hpp"
#include "../block/BlockPos.hpp"
#include "../../core/Types.hpp"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>

namespace mc {
namespace nbt {
namespace tags {
class compound_tag;
}
}

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
 * @brief 村庄
 *
 * 表示世界中的一个村庄实例，管理：
 * - 村庄边界（基于床位和工作站动态计算）
 * - 村民列表
 * - 流言/声誉系统
 * - 铃铛（聚集点）
 * - 袭击状态
 *
 * 参考 MC 1.16.5 Village
 */
class Village {
public:
    /**
     * @brief 构造函数
     * @param center 村庄中心位置
     */
    explicit Village(BlockPos center);

    // ========== 边界管理 ==========

    /**
     * @brief 获取村庄中心
     */
    [[nodiscard]] BlockPos getCenter() const { return m_center; }

    /**
     * @brief 获取村庄半径
     */
    [[nodiscard]] f32 getRadius() const { return m_radius; }

    /**
     * @brief 检查位置是否在村庄内
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
    [[nodiscard]] const std::unordered_set<u64>& getVillagers() const { return m_villagers; }

    /**
     * @brief 获取村民数量
     */
    [[nodiscard]] i32 getPopulation() const { return static_cast<i32>(m_villagers.size()); }

    // ========== 床位和繁殖 ==========

    /**
     * @brief 获取床位数量
     */
    [[nodiscard]] i32 getBedCount() const { return m_bedCount; }

    /**
     * @brief 获取可用床位数（床位数 - 村民数）
     */
    [[nodiscard]] i32 getAvailableBeds() const;

    /**
     * @brief 更新床位计数
     * @param count 新的床位数量
     */
    void setBedCount(i32 count) { m_bedCount = count; }

    /**
     * @brief 检查村庄是否可以繁殖更多村民
     */
    [[nodiscard]] bool canBreed() const;

    // ========== 工作站 ==========

    /**
     * @brief 获取工作站数量
     */
    [[nodiscard]] i32 getWorkstationCount() const { return m_workstationCount; }

    /**
     * @brief 更新工作站计数
     */
    void setWorkstationCount(i32 count) { m_workstationCount = count; }

    // ========== 聚集点 ==========

    /**
     * @brief 获取聚集点（钟的位置）
     */
    [[nodiscard]] std::optional<BlockPos> getMeetingPoint() const { return m_meetingPoint; }

    /**
     * @brief 设置聚集点
     */
    void setMeetingPoint(BlockPos pos) { m_meetingPoint = pos; }

    /**
     * @brief 检查是否有聚集点
     */
    [[nodiscard]] bool hasMeetingPoint() const { return m_meetingPoint.has_value(); }

    // ========== 流言/声誉系统 ==========

    /**
     * @brief 获取流言管理器
     */
    [[nodiscard]] VillageGossipManager& getGossipManager() { return m_gossipManager; }
    [[nodiscard]] const VillageGossipManager& getGossipManager() const { return m_gossipManager; }

    /**
     * @brief 添加流言（便捷方法）
     */
    void addGossip(u64 playerId, VillageGossipType type, i32 value = 1) {
        m_gossipManager.addGossip(playerId, type, value);
    }

    /**
     * @brief 获取玩家声誉
     */
    [[nodiscard]] i32 getPlayerReputation(u64 playerId) const {
        return m_gossipManager.getReputation(playerId);
    }

    /**
     * @brief 获取价格修正因子
     */
    [[nodiscard]] f32 getPriceModifier(u64 playerId) const {
        return m_gossipManager.getPriceModifier(playerId);
    }

    // ========== 袭击状态 ==========

    /**
     * @brief 检查村庄是否正在被袭击
     */
    [[nodiscard]] bool isUnderRaid() const { return m_underRaid; }

    /**
     * @brief 设置袭击状态
     */
    void setUnderRaid(bool underRaid) { m_underRaid = underRaid; }

    /**
     * @brief 获取上次袭击时间
     */
    [[nodiscard]] i64 getLastRaidTime() const { return m_lastRaidTime; }

    /**
     * @brief 设置上次袭击时间
     */
    void setLastRaidTime(i64 time) { m_lastRaidTime = time; }

    // ========== Tick更新 ==========

    /**
     * @brief 每游戏tick更新
     * @param world 世界接口
     * @param gameTime 当前游戏时间
     */
    void tick(IWorld& world, i64 gameTime);

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
};

} // namespace village
} // namespace world
} // namespace mc
