#pragma once

#include "../../core/MobEntity.hpp"
#include "../../interfaces/IRangedAttackMob.hpp"
#include "../../core/DataParameter.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include <memory>
#include <vector>
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class DamageSource;
class LivingEntity;

namespace entity {

/**
 * @brief 凋灵Boss实体
 *
 * 地狱Boss，具有三个头和多种攻击模式。
 *
 * 特性：
 * - 三头：主头和两个侧头独立追踪目标
 * - 无敌阶段：生成后220 ticks无敌
 * - 充能阶段：生命值低于一半时充能发射蓝色凋灵之首
 * - 方块破坏：能够破坏周围方块
 * - Boss条：显示Boss生命值
 *
 * 参考 MC 1.16.5 WitherEntity
 */
class WitherEntity : public MobEntity, public IRangedAttackMob {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    WitherEntity(LegacyEntityType type, EntityId id);

    ~WitherEntity() override = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.9f; }
    [[nodiscard]] f32 height() const override { return 3.5f; }
    [[nodiscard]] f32 eyeHeight() const override { return 2.0f; }

    void tick() override;

    // ========== LivingEntity 接口重写 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: entity.wither.ambient
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: entity.wither.hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * MC 1.16.5: entity.wither.death
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 凋灵免疫火焰、溺水、凋零伤害
     * MC 1.16.5: attackEntityFrom()
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 是否为亡灵生物
     * MC 1.16.5: getCreatureAttribute() -> UNDEAD
     */
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }

    /**
     * @brief 是否为Boss
     * MC 1.16.5: isNonBoss() -> false
     */
    [[nodiscard]] bool isNonBoss() const override { return false; }

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 40; }
    [[nodiscard]] bool canRangedAttack() const override;

    // ========== Boss 功能 ==========

    /**
     * @brief 获取Boss名称
     */
    [[nodiscard]] std::string getBossName() const;

    /**
     * @brief 是否显示生命条
     */
    [[nodiscard]] bool shouldDisplayHealthBar() const { return true; }

    // ========== 凋灵特有 ==========

    /**
     * @brief 获取无敌时间
     * MC 1.16.5: getInvulTime()
     */
    [[nodiscard]] i32 getInvulTime() const { return m_invulTime; }

    /**
     * @brief 设置无敌时间
     * MC 1.16.5: setInvulTime()
     */
    void setInvulTime(i32 time) { m_invulTime = time; }

    /**
     * @brief 是否处于无敌阶段
     */
    [[nodiscard]] bool isInvulnerablePhase() const { return m_invulTime > 0; }

    /**
     * @brief 是否充能（生命值低于一半）
     * MC 1.16.5: isCharged()
     */
    [[nodiscard]] bool isCharged() const { return health() <= maxHealth() / 2.0f; }

    /**
     * @brief 获取指定头的目标实体ID
     * MC 1.16.5: getWatchedTargetId()
     * @param head 头索引 (0=主头, 1=左头, 2=右头)
     */
    [[nodiscard]] i32 getWatchedTargetId(i32 head) const;

    /**
     * @brief 更新指定头的目标
     * MC 1.16.5: updateWatchedTargetId()
     * @param head 头索引
     * @param targetId 目标实体ID
     */
    void updateWatchedTargetId(i32 head, i32 targetId);

    /**
     * @brief 发射凋灵之首
     * @param head 头索引 (0=主头, 1=左头, 2=右头)
     * @param target 目标实体
     */
    void launchWitherSkullToEntity(i32 head, LivingEntity* target);

    /**
     * @brief 开始生成序列
     * MC 1.16.5: ignite()
     */
    void ignite();

protected:
    void registerData() override;
    void registerGoals() override;
    void registerAttributes() override;

private:
    // ========== 数据参数 ==========
    // 头部追踪目标实体ID（MC 1.16.5: FIRST_HEAD_TARGET, SECOND_HEAD_TARGET, THIRD_HEAD_TARGET）
    static entity::DataParameter<i32> HEAD_TARGET_1;  // 主头目标
    static entity::DataParameter<i32> HEAD_TARGET_2;  // 左头目标
    static entity::DataParameter<i32> HEAD_TARGET_3;  // 右头目标

    // 无敌时间（MC 1.16.5: invulTime）
    i32 m_invulTime = 0;

    // 头部旋转角度（用于渲染）
    f32 m_headXRot[2] = {0.0f, 0.0f};      // 侧头俯仰角
    f32 m_headYRot[2] = {0.0f, 0.0f};      // 侧头偏航角
    f32 m_prevHeadXRot[2] = {0.0f, 0.0f};  // 上一帧俯仰角
    f32 m_prevHeadYRot[2] = {0.0f, 0.0f};  // 上一帧偏航角

    // 头部攻击相关
    i32 m_nextHeadUpdate[2] = {0, 0};      // 下次攻击更新tick
    i32 m_idleHeadUpdates[2] = {0, 0};     // 空闲头部更新计数
    i32 m_blockBreakCounter = 0;           // 方块破坏计数器

    // MC 1.16.5 常量
    static constexpr i32 INVULNERABILITY_TIME = 220;  // 生成无敌时间 (11秒)
    static constexpr i32 BLOCK_BREAK_COOLDOWN = 20;   // 方块破坏冷却
    static constexpr f32 HEAD_TRACK_RANGE = 20.0f;    // 头部追踪范围
    static constexpr i32 ATTACK_COOLDOWN = 40;        // 攻击冷却 (2秒)

    /**
     * @brief 更新AI任务
     * MC 1.16.5: updateAITasks()
     */
    void updateAITasks();

    /**
     * @brief 更新头部目标追踪
     */
    void updateHeadTargets();

    /**
     * @brief 获取头的X坐标
     * MC 1.16.5: getHeadX()
     */
    [[nodiscard]] f32 getHeadX(i32 head) const;

    /**
     * @brief 获取头的Y坐标
     * MC 1.16.5: getHeadY()
     */
    [[nodiscard]] f32 getHeadY(i32 head) const;

    /**
     * @brief 获取头的Z坐标
     * MC 1.16.5: getHeadZ()
     */
    [[nodiscard]] f32 getHeadZ(i32 head) const;

    /**
     * @brief 破坏周围方块
     */
    void breakNearbyBlocks();

    /**
     * @brief 在生成时创建爆炸
     */
    void explodeOnSpawn();
};

} // namespace entity
} // namespace mc
