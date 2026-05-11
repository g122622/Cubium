#pragma once

#include "server/stats/Stat.hpp"
#include "server/stats/StatType.hpp"
#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <unordered_map>
#include <string>
#include <functional>
#include <mutex>

namespace mc {
namespace server {

// Forward declarations
class ServerPlayer;
namespace core { class PlayerManager; }

namespace stats {

/**
 * @brief 玩家统计管理器
 *
 * 管理单个玩家的所有统计数据。统计数据存储在内存中，
 * 并支持 NBT 序列化以便持久化。
 *
 * 参考 MC 1.16.5: net.minecraft.stats.StatisticsManager
 */
class StatisticsManager {
public:
    /**
     * @brief 统计值类型
     */
    using ValueType = Stat::ValueType;

    /**
     * @brief 默认构造函数
     */
    StatisticsManager() = default;

    /**
     * @brief 从 NBT 加载统计数据
     */
    static Result<StatisticsManager> fromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 序列化到 NBT
     */
    [[nodiscard]] nbt::tags::compound_tag toNbt() const;

    // ========== 统计值操作 ==========

    /**
     * @brief 获取统计值
     *
     * @param type 统计类型
     * @param id 统计ID（方块、物品、实体或自定义统计ID）
     * @return 统计值，如果不存在返回 0
     */
    [[nodiscard]] ValueType getValue(StatType type, const ResourceLocation& id) const;

    /**
     * @brief 获取统计值（通过完整ID）
     *
     * @param fullId 完整统计ID（如 minecraft.mined:minecraft.stone）
     * @return 统计值，如果不存在返回 0
     */
    [[nodiscard]] ValueType getValue(const ResourceLocation& fullId) const;

    /**
     * @brief 设置统计值
     *
     * @param type 统计类型
     * @param id 统计ID
     * @param value 新值
     */
    void setValue(StatType type, const ResourceLocation& id, ValueType value);

    /**
     * @brief 增加统计值
     *
     * @param type 统计类型
     * @param id 统计ID
     * @param delta 增量（默认为1）
     */
    void increment(StatType type, const ResourceLocation& id, ValueType delta = 1);

    /**
     * @brief 减少统计值
     *
     * @param type 统计类型
     * @param id 统计ID
     * @param delta 减量（默认为1）
     */
    void decrement(StatType type, const ResourceLocation& id, ValueType delta = 1) {
        increment(type, id, -delta);
    }

    /**
     * @brief 重置统计值
     *
     * @param type 统计类型
     * @param id 统计ID
     */
    void reset(StatType type, const ResourceLocation& id);

    /**
     * @brief 重置所有统计
     */
    void resetAll();

    // ========== 查询 ==========

    /**
     * @brief 检查统计是否存在
     */
    [[nodiscard]] bool hasStat(StatType type, const ResourceLocation& id) const;

    /**
     * @brief 检查统计是否存在（通过完整ID）
     */
    [[nodiscard]] bool hasStat(const ResourceLocation& fullId) const;

    /**
     * @brief 获取所有非零统计
     *
     * @return 统计ID到值的映射
     */
    [[nodiscard]] std::unordered_map<ResourceLocation, ValueType> getAllStats() const;

    /**
     * @brief 获取指定类型的所有统计
     *
     * @param type 统计类型
     * @return 统计ID到值的映射
     */
    [[nodiscard]] std::unordered_map<ResourceLocation, ValueType> getStatsByType(StatType type) const;

    /**
     * @brief 遍历所有统计
     *
     * @param callback 回调函数，参数为（完整ID，值），返回 false 停止遍历
     */
    void forEach(const std::function<bool(const ResourceLocation&, ValueType)>& callback) const;

    // ========== 快捷方法 ==========

    /**
     * @brief 增加挖掘统计
     *
     * @param blockId 方块ID
     */
    void incrementMined(const ResourceLocation& blockId) {
        increment(StatType::Mined, blockId);
    }

    /**
     * @brief 增加合成统计
     *
     * @param itemId 物品ID
     * @param count 数量
     */
    void incrementCrafted(const ResourceLocation& itemId, ValueType count = 1) {
        increment(StatType::Crafted, itemId, count);
    }

    /**
     * @brief 增加使用统计
     *
     * @param itemId 物品ID
     */
    void incrementUsed(const ResourceLocation& itemId) {
        increment(StatType::Used, itemId);
    }

    /**
     * @brief 增加损坏统计
     *
     * @param itemId 物品ID
     */
    void incrementBroken(const ResourceLocation& itemId) {
        increment(StatType::Broken, itemId);
    }

    /**
     * @brief 增加拾取统计
     *
     * @param itemId 物品ID
     * @param count 数量
     */
    void incrementPickedUp(const ResourceLocation& itemId, ValueType count = 1) {
        increment(StatType::PickedUp, itemId, count);
    }

    /**
     * @brief 增加丢弃统计
     *
     * @param itemId 物品ID
     * @param count 数量
     */
    void incrementDropped(const ResourceLocation& itemId, ValueType count = 1) {
        increment(StatType::Dropped, itemId, count);
    }

    /**
     * @brief 增加击杀统计
     *
     * @param entityId 实体ID
     */
    void incrementKilled(const ResourceLocation& entityId) {
        increment(StatType::Killed, entityId);
    }

    /**
     * @brief 增加被击杀统计
     *
     * @param entityId 实体ID
     */
    void incrementKilledBy(const ResourceLocation& entityId) {
        increment(StatType::KilledBy, entityId);
    }

    /**
     * @brief 增加自定义统计
     *
     * @param statId 统计ID
     * @param delta 增量
     */
    void incrementCustom(const ResourceLocation& statId, ValueType delta = 1) {
        increment(StatType::Custom, statId, delta);
    }

    /**
     * @brief 标记为已修改
     */
    void markDirty() { m_dirty = true; }

    /**
     * @brief 清除已修改标记
     */
    void clearDirty() { m_dirty = false; }

    /**
     * @brief 检查是否已修改
     */
    [[nodiscard]] bool isDirty() const { return m_dirty; }

private:
    // 使用完整统计ID作为键
    std::unordered_map<ResourceLocation, ValueType> m_stats;
    bool m_dirty = false;
};

} // namespace stats
} // namespace server
} // namespace mc
