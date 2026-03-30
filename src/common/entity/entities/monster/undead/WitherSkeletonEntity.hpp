#pragma once

#include "SkeletonEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 凋灵骷髅实体
 *
 * 生活在下界要塞的骷髅变种。
 *
 * 特性：
 * - 下界生成：只在下界要塞生成
 * - 凋灵效果：攻击会给予凋灵效果
 * - 高大：比普通骷髅更高
 * - 石剑：手持石剑近战攻击
 *
 * 参考 MC 1.16.5 WitherSkeletonEntity
 */
class WitherSkeletonEntity : public SkeletonEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    WitherSkeletonEntity(LegacyEntityType type, EntityId id);

    ~WitherSkeletonEntity() override = default;

    // 禁止拷贝
    WitherSkeletonEntity(const WitherSkeletonEntity&) = delete;
    WitherSkeletonEntity& operator=(const WitherSkeletonEntity&) = delete;

    // 允许移动
    WitherSkeletonEntity(WitherSkeletonEntity&&) = default;
    WitherSkeletonEntity& operator=(WitherSkeletonEntity&&) = default;

    /**
     * @brief 创建凋灵骷髅实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 近战攻击 ==========

    /**
     * @brief 凋灵骷髅不使用弓箭
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override {
        (void)target;
        (void)charge;
        // 凋灵骷髅使用近战攻击
    }

    /**
     * @brief 是否手持石剑
     */
    [[nodiscard]] bool hasStoneSword() const { return m_hasStoneSword; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 凋灵骷髅不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.1f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasStoneSword = true;
};

} // namespace mc
