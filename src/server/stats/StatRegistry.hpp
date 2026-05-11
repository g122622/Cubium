#pragma once

#include "server/stats/Stat.hpp"
#include "server/stats/StatType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/core/Result.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace server {
namespace stats {

/**
 * @brief 统计注册表
 *
 * 管理所有已注册的统计项模板。玩家统计值存储在 StatisticsManager 中。
 *
 * 参考 MC 1.16.5: net.minecraft.stats.StatType (注册表概念)
 * 以及 Stats 类（内置统计常量）
 */
class StatRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static StatRegistry& instance();

    /**
     * @brief 注册内置统计
     *
     * 包括所有方块挖掘、物品使用、实体击杀等统计
     */
    void registerBuiltinStats();

    /**
     * @brief 清空所有注册的统计
     *
     * 用于测试清理
     */
    void clear();

    // ========== 方块挖掘统计 ==========

    /**
     * @brief 注册方块挖掘统计
     *
     * 统计ID：minecraft.mined:{block_id}
     *
     * @param blockId 方块资源位置
     */
    void registerMinedStat(const ResourceLocation& blockId);

    /**
     * @brief 获取方块挖掘统计ID
     */
    [[nodiscard]] ResourceLocation getMinedStatId(const ResourceLocation& blockId) const;

    // ========== 物品统计 ==========

    /**
     * @brief 注册物品合成统计
     *
     * 统计ID：minecraft.crafted:{item_id}
     */
    void registerCraftedStat(const ResourceLocation& itemId);

    /**
     * @brief 注册物品使用统计
     *
     * 统计ID：minecraft.used:{item_id}
     */
    void registerUsedStat(const ResourceLocation& itemId);

    /**
     * @brief 注册物品损坏统计
     *
     * 统计ID：minecraft.broken:{item_id}
     */
    void registerBrokenStat(const ResourceLocation& itemId);

    /**
     * @brief 注册物品拾取统计
     *
     * 统计ID：minecraft.picked_up:{item_id}
     */
    void registerPickedUpStat(const ResourceLocation& itemId);

    /**
     * @brief 注册物品丢弃统计
     *
     * 统计ID：minecraft.dropped:{item_id}
     */
    void registerDroppedStat(const ResourceLocation& itemId);

    // ========== 实体统计 ==========

    /**
     * @brief 注册实体击杀统计
     *
     * 统计ID：minecraft.killed:{entity_id}
     */
    void registerKilledStat(const ResourceLocation& entityId);

    /**
     * @brief 注册被实体击杀统计
     *
     * 统计ID：minecraft.killed_by:{entity_id}
     */
    void registerKilledByStat(const ResourceLocation& entityId);

    // ========== 自定义统计 ==========

    /**
     * @brief 注册自定义统计
     *
     * 统计ID：minecraft.custom:{stat_id}
     *
     * 自定义统计包括：
     * - play_one_minute: 游戏时间（tick）
     * - walk_one_cm: 行走距离（厘米）
     * - sprint_one_cm: 疾跑距离
     * - swim_one_cm: 游泳距离
     * - fall_one_cm: 摔落距离
     * - climb_one_cm: 攀爬距离
     * - fly_one_cm: 飞行距离
     * - dive_one_cm: 潜水距离
     * - minecart_one_cm: 矿车距离
     * - boat_one_cm: 船距离
     * - pig_one_cm: 骑猪距离
     * - horse_one_cm: 骑马距离
     * - aviate_one_cm: 鞘翅飞行距离
     * - jump: 跳跃次数
     * - deaths: 死亡次数
     * - mob_kills: 击杀生物次数
     * - animals_bred: 繁殖动物次数
     * - player_kills: 击杀玩家次数
     * - fish_caught: 钓鱼次数
     * - talked_to_villager: 与村民交谈次数
     * - traded_with_villager: 与村民交易次数
     * - eat_cake_slice: 吃蛋糕片数
     * - fill_cauldron: 填充炼药锅次数
     * - use_cauldron: 使用炼药锅次数
     * - clean_armor: 在炼药锅清洗盔甲次数
     * ... 等等
     */
    void registerCustomStat(const ResourceLocation& statId);

    /**
     * @brief 检查统计是否已注册
     */
    [[nodiscard]] bool hasStat(StatType type, const ResourceLocation& id) const;

    /**
     * @brief 检查完整统计ID是否已注册
     */
    [[nodiscard]] bool hasStat(const ResourceLocation& fullId) const;

    /**
     * @brief 获取所有已注册的统计ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllStatIds() const;

    /**
     * @brief 获取指定类型的所有统计ID
     */
    [[nodiscard]] std::vector<ResourceLocation> getStatIdsByType(StatType type) const;

private:
    StatRegistry() = default;
    ~StatRegistry() = default;
    StatRegistry(const StatRegistry&) = delete;
    StatRegistry& operator=(const StatRegistry&) = delete;

    void registerAllBlocks();
    void registerAllItems();
    void registerAllEntities();
    void registerAllCustomStats();

    // 使用完整统计ID作为键
    std::unordered_map<ResourceLocation, std::pair<StatType, ResourceLocation>> m_stats;
};

} // namespace stats
} // namespace server
} // namespace mc
