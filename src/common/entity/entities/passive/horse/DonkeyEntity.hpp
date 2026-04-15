#pragma once

#include "AbstractChestedHorseEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 驴实体
 *
 * 对齐 1.16.5 `DonkeyEntity`。驴属于可装备箱子的马类，
 * 和骡共享 `AbstractChestedHorseEntity` 层。
 */
class DonkeyEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 构造驴实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    DonkeyEntity(LegacyEntityType type, EntityId id);
    ~DonkeyEntity() override = default;

    DonkeyEntity(const DonkeyEntity&) = delete;
    DonkeyEntity& operator=(const DonkeyEntity&) = delete;
    DonkeyEntity(DonkeyEntity&&) = default;
    DonkeyEntity& operator=(DonkeyEntity&&) = default;

    /**
     * @brief 创建驴实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.45f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
