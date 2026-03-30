#pragma once

#include "AnimalEntity.hpp"
#include <memory>

namespace mc {

// 前向声明
class IWorld;
class ItemStack;

/**
 * @brief 牛实体
 *
 * 可被挤奶的被动动物，用小麦繁殖。
 *
 * 参考 MC 1.16.5 CowEntity
 */
class CowEntity : public AnimalEntity {
public:
    CowEntity(LegacyEntityType type, EntityId id);
    ~CowEntity() override = default;

    /**
     * @brief 实体工厂方法
     *
     * 用于 EntityRegistry 注册
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 繁殖 ==========

    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.4f; }

    // TODO: 挤奶逻辑
};

} // namespace mc
