#pragma once

#include "AbstractHorseEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 骷髅马实体
 *
 * 由骷髅骑乘的亡灵马，在雷暴天气中生成。
 *
 * 特性：
 * - 可骑乘：可直接骑乘，无需驯服
 * - 不死生物：免疫溺水、中毒
 * - 阳光燃烧：在阳光下燃烧（如果不是戴着头盔）
 * - 不繁殖：无法繁殖
 * - 捕获：击败骷髅骑手后可以骑乘
 *
 * 参考 MC 1.16.5 SkeletonHorseEntity
 */
class SkeletonHorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SkeletonHorseEntity(LegacyEntityType type, EntityId id);
    ~SkeletonHorseEntity() override = default;

    // 禁止拷贝
    SkeletonHorseEntity(const SkeletonHorseEntity&) = delete;
    SkeletonHorseEntity& operator=(const SkeletonHorseEntity&) = delete;

    // 允许移动
    SkeletonHorseEntity(SkeletonHorseEntity&&) = default;
    SkeletonHorseEntity& operator=(SkeletonHorseEntity&&) = default;

    /**
     * @brief 创建骷髅马实体
     * @param world 世界实例
     * @return 新的骷髅马实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查玩家是否可以骑乘
     * 骷髅马不需要驯服即可骑乘
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const override;

    // ========== 繁殖系统 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 骷髅马不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 生成幼体
     * 骷髅马不能繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        (void)partner;
        return nullptr;
    }

    // ========== 不死生物特性 ==========

    /**
     * @brief 是否免疫溺水
     */
    [[nodiscard]] bool canBreatheUnderwater() const override { return true; }

    /**
     * @brief 是否应该燃烧（阳光）
     * 骷髅马在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return true; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.6f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_trapChecked = false;  // 是否已检查陷阱
};

} // namespace mc
