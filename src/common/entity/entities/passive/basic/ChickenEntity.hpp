#pragma once

#include "AnimalEntity.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 鸡实体
 *
 * 会下蛋的被动动物，用种子繁殖。
 *
 * 参考 MC 1.16.5 ChickenEntity
 */
class ChickenEntity : public AnimalEntity {
public:
    ChickenEntity(LegacyEntityType type, EntityId id);
    ~ChickenEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 下蛋 ==========

    /**
     * @brief 获取下蛋计时器
     * @return 到下次下蛋的时间（tick）
     */
    [[nodiscard]] i32 getEggTimer() const { return m_eggTimer; }

    /**
     * @brief 重置下蛋计时器
     */
    void resetEggTimer();

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.4f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.7f; }

private:
    i32 m_eggTimer = 0;        // 下蛋计时器
    f32 m_eggTime = 0.0f;      // 累计时间

    static constexpr i32 EGG_TIME_MIN = 6000;   // 最小下蛋时间（5分钟）
    static constexpr i32 EGG_TIME_MAX = 12000;  // 最大下蛋时间（10分钟）
};

} // namespace mc
