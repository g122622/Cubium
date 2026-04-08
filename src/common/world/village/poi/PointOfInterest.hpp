#pragma once

#include "PointOfInterestType.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../core/Types.hpp"
#include <optional>
#include <vector>

namespace mc {
namespace nbt {
namespace tags {
class compound_tag;
}
}

namespace world {
namespace village {
namespace poi {

/**
 * @brief POI票据信息
 *
 * 记录哪个实体占用了POI以及相关状态
 */
struct POITicket {
    /// 占用者实体ID
    u64 ownerId;
    /// 票据创建时间（游戏tick）
    i64 createdAt;
    /// 是否已释放
    bool released;
};

/**
 * @brief POI（兴趣点）
 *
 * 表示世界中一个可被村民使用或占用的特殊方块位置。
 * POI包括床位、工作站、钟等，村民通过POI进行睡眠、工作、聚集等行为。
 *
 * 参考 MC 1.16.5 PointOfInterest
 */
class PointOfInterest {
public:
    /**
     * @brief 默认构造函数
     */
    PointOfInterest() = default;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     * @param type POI类型
     */
    PointOfInterest(BlockPos pos, PointOfInterestType type);

    // ========== 基本信息 ==========

    /**
     * @brief 获取位置
     */
    [[nodiscard]] BlockPos getPosition() const { return m_position; }

    /**
     * @brief 获取类型
     */
    [[nodiscard]] PointOfInterestType getType() const { return m_type; }

    // ========== 占用状态 ==========

    /**
     * @brief 检查是否已被占用
     * @return 是否有所有者
     */
    [[nodiscard]] bool isOccupied() const { return m_tickets.size() >= static_cast<std::size_t>(m_maxTickets); }

    /**
     * @brief 检查是否可被指定实体占用
     * @param ownerId 实体ID
     * @return 是否可以占用
     */
    [[nodiscard]] bool canAcquire(u64 ownerId) const;

    /**
     * @brief 获取当前占用票据数
     */
    [[nodiscard]] i32 getTicketCount() const { return static_cast<i32>(m_tickets.size()); }

    /**
     * @brief 获取最大票据数
     */
    [[nodiscard]] i32 getMaxTickets() const { return m_maxTickets; }

    // ========== 票据操作 ==========

    /**
     * @brief 尝试占用此POI
     * @param ownerId 占用者实体ID
     * @param gameTime 当前游戏时间
     * @return 是否成功占用
     */
    bool acquire(u64 ownerId, i64 gameTime);

    /**
     * @brief 释放占用
     * @param ownerId 占用者实体ID
     * @return 是否成功释放
     */
    bool release(u64 ownerId);

    /**
     * @brief 检查指定实体是否占用此POI
     * @param ownerId 实体ID
     * @return 是否占用
     */
    [[nodiscard]] bool isOwnedBy(u64 ownerId) const;

    /**
     * @brief 获取所有占用者ID
     */
    [[nodiscard]] std::vector<u64> getOwners() const;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到NBT
     * @param tag NBT标签
     */
    void serialize(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从NBT反序列化
     * @param tag NBT标签
     * @return 反序列化后的POI
     */
    static PointOfInterest deserialize(const nbt::tags::compound_tag& tag);

private:
    /// POI位置
    BlockPos m_position;
    /// POI类型
    PointOfInterestType m_type = PointOfInterestType::None;
    /// 占用票据列表
    std::vector<POITicket> m_tickets;
    /// 最大票据数（默认为1）
    i32 m_maxTickets = 1;
};

/**
 * @brief POI比较器（用于空间索引）
 */
struct POIComparator {
    bool operator()(const PointOfInterest& a, const PointOfInterest& b) const {
        if (a.getPosition().y != b.getPosition().y) {
            return a.getPosition().y < b.getPosition().y;
        }
        if (a.getPosition().x != b.getPosition().x) {
            return a.getPosition().x < b.getPosition().x;
        }
        return a.getPosition().z < b.getPosition().z;
    }
};

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
