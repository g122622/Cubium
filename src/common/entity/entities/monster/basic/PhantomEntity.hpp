#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../core/FlyingEntity.hpp"
#include <optional>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;

/**
 * @brief 幻翼实体
 *
 * 在夜间生成的飞行敌对生物，会俯冲攻击玩家。
 *
 * 特性：
 * - 飞行：可以自由飞行
 * - 俯冲攻击：环绕目标后俯冲攻击
 * - 阳光燃烧：在阳光下燃烧
 * - 变体：有不同大小
 *
 * 参考 MC 1.16.5 PhantomEntity
 */
class PhantomEntity : public FlyingEntity {
public:
    /**
     * @brief 幻翼攻击模式
     * MC 1.16.5: AttackPhase enum
     */
    enum class AttackPhase : u8 {
        CIRCLE, // 环绕
        SWOOP   // 俯冲
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    PhantomEntity(LegacyEntityType type, EntityId id);
    ~PhantomEntity() override = default;

    // 禁止拷贝
    PhantomEntity(const PhantomEntity&) = delete;
    PhantomEntity& operator=(const PhantomEntity&) = delete;

    // 允许移动
    PhantomEntity(PhantomEntity&&) = default;
    PhantomEntity& operator=(PhantomEntity&&) = default;

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: entity.phantom.ambient
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: entity.phantom.hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * MC 1.16.5: entity.phantom.death
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 尺寸系统 ==========

    /**
     * @brief 获取幻翼大小
     * MC 1.16.5: getPhantomSize()
     */
    [[nodiscard]] i32 getPhantomSize() const { return m_phantomSize; }

    /**
     * @brief 设置幻翼大小
     * MC 1.16.5: setPhantomSize()
     */
    void setPhantomSize(i32 size);

    /**
     * @brief 获取实体尺寸（考虑幻翼大小）
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    // ========== 攻击 ==========

    /**
     * @brief 获取当前攻击阶段
     */
    [[nodiscard]] AttackPhase getAttackPhase() const { return m_attackPhase; }

    /**
     * @brief 设置攻击阶段
     */
    void setAttackPhase(AttackPhase phase) { m_attackPhase = phase; }

    // ========== 属性 ==========

    /**
     * @brief 获取生物类型
     * MC 1.16.5: UNDEAD
     */
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }

    /**
     * @brief 获取眼睛高度
     * MC 1.16.5: height * 0.35F
     */
    [[nodiscard]] f32 eyeHeight() const override { return height() * 0.35f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 更新AI任务
     */
    void updateAITasks() override;

private:
    // 幻翼大小（0-64）
    i32 m_phantomSize = 0;

    // 攻击阶段
    AttackPhase m_attackPhase = AttackPhase::CIRCLE;

    // 环绕位置
    BlockPos m_orbitPosition;

    // 环绕偏移
    Vector3 m_orbitOffset;

    // MC 1.16.5 常量
    static constexpr f32 BASE_ATTACK_DAMAGE = 6.0f;
    static constexpr f32 SIZE_ATTACK_BONUS = 1.0f;
    static constexpr i32 MAX_PHANTOM_SIZE = 64;
};

} // namespace mc
