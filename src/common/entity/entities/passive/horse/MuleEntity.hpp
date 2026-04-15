#pragma once

#include "AbstractChestedHorseEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 骡实体
 *
 * 对齐 1.16.5 `MuleEntity`。骡和驴共用箱子马类中间层，
 * 但保留自身“不育”的后代语义。
 */
class MuleEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 构造骡实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    MuleEntity(LegacyEntityType type, EntityId id);
    ~MuleEntity() override = default;

    MuleEntity(const MuleEntity&) = delete;
    MuleEntity& operator=(const MuleEntity&) = delete;
    MuleEntity(MuleEntity&&) = default;
    MuleEntity& operator=(MuleEntity&&) = default;

    /**
     * @brief 创建骡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 骡不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 骡不能生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.5f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
