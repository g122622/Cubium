#pragma once

#include "../../../../core/Types.hpp"
#include "../../passive/water/WaterMobEntity.hpp"
#include "ZombieEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 溺尸实体
 *
 * 在水中生成的僵尸变种。
 *
 * 特性：
 * - 水中生成：在海洋和河流中生成
 * - 水中生活：可以在水中呼吸
 * - 三叉戟：有概率手持三叉戟
 * - 溺水：玩家溺水后可能转化为溺尸
 *
 * 参考 MC 1.16.5 DrownedEntity
 */
class DrownedEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    DrownedEntity(LegacyEntityType type, EntityId id);

    ~DrownedEntity() override = default;

    // 禁止拷贝
    DrownedEntity(const DrownedEntity&) = delete;
    DrownedEntity& operator=(const DrownedEntity&) = delete;

    // 允许移动
    DrownedEntity(DrownedEntity&&) = default;
    DrownedEntity& operator=(DrownedEntity&&) = default;

    /**
     * @brief 创建溺尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 水中生活 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否可以游泳
     */
    [[nodiscard]] bool canSwim() const { return true; }

    // ========== 装备 ==========

    /**
     * @brief 是否手持三叉戟
     */
    [[nodiscard]] bool hasTrident() const { return m_hasTrident; }

    /**
     * @brief 设置手持三叉戟
     */
    void setHasTrident(bool trident) { m_hasTrident = trident; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 溺尸不在阳光下燃烧（如果在水中）
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerAttributes() override;

private:
    bool m_hasTrident = false;
};

} // namespace mc
