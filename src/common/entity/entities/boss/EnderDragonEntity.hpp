/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../../core/Types.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../core/MobEntity.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;
class LivingEntity;

namespace entity {
class EnderCrystalEntity;
} // namespace entity

namespace entity::effect {
class EffectInstance;
} // namespace entity::effect

namespace entity {

/**
 * @brief Boss实体基类
 *
 * Boss级怪物的基类，具有生命条显示等特殊功能。
 */
class BossEntity : public MobEntity {
public:
    explicit BossEntity(EntityId id);
    ~BossEntity() override = default;

    // ========== Boss 特有功能 ==========

    /**
     * @brief 获取Boss名称（显示在生命条上）
     */
    [[nodiscard]] virtual std::string getBossName() const = 0;

    /**
     * @brief 获取生命条显示范围
     */
    [[nodiscard]] virtual f32 getHealthBarRange() const { return 100.0f; }

    /**
     * @brief 是否显示生命条
     */
    [[nodiscard]] bool shouldDisplayHealthBar() const { return m_displayHealthBar; }

    /**
     * @brief 设置是否显示生命条
     */
    void setDisplayHealthBar(bool display) { m_displayHealthBar = display; }

    /**
     * @brief 获取生命条颜色
     */
    [[nodiscard]] virtual u32 getHealthBarColor() const { return 0xFF0000; } // 红色

    /**
     * @brief 是否为Boss战
     */
    [[nodiscard]] bool inBossFight() const { return m_inBossFight; }

    /**
     * @brief 设置Boss战状态
     */
    void setBossFight(bool fighting) { m_inBossFight = fighting; }

protected:
    bool m_displayHealthBar = true;
    bool m_inBossFight = false;
};

/**
 * @brief 末影龙部件实体
 *
 * 末影龙的碰撞部件，用于精确碰撞检测。
 *
 * 参考 MC 1.16.5 EnderDragonPartEntity
 */
class EnderDragonPartEntity : public Entity {
public:
    /**
     * @brief 部件类型
     */
    enum class Part : u8 {
        Head,     // 头部
        Neck,     // 颈部
        Body,     // 身体
        Tail1,    // 尾部1
        Tail2,    // 尾部2
        Tail3,    // 尾部3
        WingLeft, // 左翼
        WingRight // 右翼
    };

    /**
     * @brief 构造函数
     */
    explicit EnderDragonPartEntity(EntityId id);

    ~EnderDragonPartEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 部件方法 ==========

    /**
     * @brief 获取父龙
     */
    [[nodiscard]] Entity* parentDragon() const { return m_parent; }

    /**
     * @brief 设置父龙
     */
    void setParentDragon(Entity* parent) { m_parent = parent; }

    /**
     * @brief 获取部件类型
     */
    [[nodiscard]] Part part() const { return m_part; }

    /**
     * @brief 设置部件类型
     */
    void setPart(Part part) { m_part = part; }

    /**
     * @brief 更新部件位置
     */
    void updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 width, f32 height);

private:
    Entity* m_parent = nullptr;
    Part m_part = Part::Body;
};

/**
 * @brief 末影龙实体
 *
 * 末地Boss，具有多种攻击模式和阶段。
 *
 * 特性：
 * - 多阶段AI：HoldingPattern, StrafePlayer, Landing, Charging, Dying等
 * - 多部件碰撞：头、颈、身、尾、翼
 * - 末影水晶回血
 * - 飞行移动
 * - 方块破坏
 *
 * 参考 MC 1.16.5 EnderDragonEntity
 */
class EnderDragonEntity : public BossEntity {
public:
    /**
     * @brief 龙的阶段
     * MC 1.16.5: PhaseType enum
     */
    enum class Phase : u8 {
        HoldingPattern,   // 盘旋
        StrafePlayer,     // 突袭玩家
        LandingApproach,  // 准备降落
        Landing,          // 降落
        Takeoff,          // 起飞
        SittingFlaming,   // 坐在传送门上喷火
        SittingScanning,  // 坐在传送门上扫描
        SittingAttacking, // 坐在传送门上攻击
        ChargingPlayer,   // 冲向玩家
        Dying,            // 死亡
        Hover             // 悬停
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit EnderDragonEntity(EntityId id);

    ~EnderDragonEntity() override = default;

    // 禁止拷贝
    EnderDragonEntity(const EnderDragonEntity&) = delete;
    EnderDragonEntity& operator=(const EnderDragonEntity&) = delete;

    // 允许移动
    EnderDragonEntity(EnderDragonEntity&&) = delete;
    EnderDragonEntity& operator=(EnderDragonEntity&&) = delete;

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: entity.ender_dragon.ambient
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: entity.ender_dragon.hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取声音音量
     * MC 1.16.5: 5.0F
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 5.0f; }

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 16.0f; }
    [[nodiscard]] f32 height() const override { return 8.0f; }
    [[nodiscard]] f32 eyeHeight() const override { return 6.0f; }

    void tick() override;

    /**
     * @brief 末影龙不能被骑乘
     */
    [[nodiscard]] bool canBeRidden(const Entity& /*vehicle*/) const override { return false; }

    /**
     * @brief 末影龙是Boss
     */
    [[nodiscard]] bool isNonBoss() const override { return false; }

    /**
     * @brief 末影龙免疫药水效果
     */
    [[nodiscard]] bool isPotionApplicable(const entity::effect::EffectInstance& /*effect*/) const override
    {
        return false;
    }

    // ========== BossEntity 接口 ==========

    /**
     * @brief 获取Boss名称（显示在生命条上）
     * MC 1.16.5: 返回自定义名称或默认名称
     */
    [[nodiscard]] std::string getBossName() const override
    {
        if (hasCustomName()) {
            return customNameText();
        }
        return "Ender Dragon";
    }
    [[nodiscard]] f32 getHealthBarRange() const override { return 256.0f; }
    [[nodiscard]] u32 getHealthBarColor() const override { return 0x800080; } // 紫色

    // ========== 末影龙特有 ==========

    /**
     * @brief 获取当前阶段
     */
    [[nodiscard]] Phase phase() const { return m_phase; }

    /**
     * @brief 设置阶段
     */
    void setPhase(Phase phase);

    /**
     * @brief 获取龙部件数组
     */
    [[nodiscard]] const std::vector<EnderDragonPartEntity*>& getDragonParts() const { return m_dragonParts; }

    /**
     * @brief 获取最近的末影水晶
     */
    [[nodiscard]] entity::EnderCrystalEntity* closestEnderCrystal() const { return m_closestEnderCrystal; }

    /**
     * @brief 设置最近的末影水晶
     */
    void setClosestEnderCrystal(entity::EnderCrystalEntity* crystal) { m_closestEnderCrystal = crystal; }

    /**
     * @brief 是否在栖息点
     */
    [[nodiscard]] bool isSitting() const
    {
        return m_phase == Phase::SittingFlaming || m_phase == Phase::SittingScanning ||
            m_phase == Phase::SittingAttacking;
    }

    /**
     * @brief 是否正在死亡
     */
    [[nodiscard]] bool isDying() const { return m_phase == Phase::Dying; }

    /**
     * @brief 获取死亡计时器
     */
    [[nodiscard]] i32 deathTicks() const { return m_deathTicks; }

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    /**
     * @brief 龙部件受到伤害
     * MC 1.16.5: attackEntityPartFrom()
     */
    bool attackEntityPartFrom(EnderDragonPartEntity* part, DamageSource& source, f32 damage);

    /**
     * @brief 末影水晶被破坏
     * MC 1.16.5: onCrystalDestroyed()
     */
    void onCrystalDestroyed(EnderCrystalEntity* crystal, const BlockPos& pos, DamageSource& source);

    /**
     * @brief 初始化路径点
     * MC 1.16.5: initPathPoints()
     */
    void initPathPoints();

    /**
     * @brief 获取最近路径点索引
     */
    [[nodiscard]] i32 getNearestPathPointIndex(f64 x, f64 y, f64 z) const;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /**
     * @brief 更新龙部件位置
     */
    void updateDragonParts();

    /**
     * @brief 更新末影水晶
     */
    void updateDragonEnderCrystal();

    /**
     * @brief 碰撞处理
     */
    void collideWithEntities();

    /**
     * @brief 攻击实体列表
     */
    void attackEntitiesInList();

    /**
     * @brief 破坏方块
     */
    bool destroyBlocksInAABB(const AxisAlignedBB& area);

    /**
     * @brief 死亡更新
     */
    void onDeathUpdate();

    /**
     * @brief 掉落经验
     * 重写父类方法，末影龙使用自定义经验值
     */
    void dropExperience() override;

    /**
     * @brief 掉落指定数量的经验
     * @param amount 经验数量
     */
    void dropExperienceAmount(i32 amount);

    // 阶段
    Phase m_phase = Phase::HoldingPattern;

    // 龙部件
    std::vector<EnderDragonPartEntity*> m_dragonParts;
    EnderDragonPartEntity* m_dragonPartHead = nullptr;
    EnderDragonPartEntity* m_dragonPartNeck = nullptr;
    EnderDragonPartEntity* m_dragonPartBody = nullptr;
    EnderDragonPartEntity* m_dragonPartTail1 = nullptr;
    EnderDragonPartEntity* m_dragonPartTail2 = nullptr;
    EnderDragonPartEntity* m_dragonPartTail3 = nullptr;
    EnderDragonPartEntity* m_dragonPartRightWing = nullptr;
    EnderDragonPartEntity* m_dragonPartLeftWing = nullptr;

    // 攻击目标
    LivingEntity* m_attackTarget = nullptr;

    // 末影水晶
    entity::EnderCrystalEntity* m_closestEnderCrystal = nullptr;

    // 动画
    f32 m_prevAnimTime = 0.0f;
    f32 m_animTime = 0.0f;
    bool m_slowed = false;

    // 死亡动画
    i32 m_deathTicks = 0;

    // 栖息伤害
    i32 m_sittingDamageReceived = 0;

    // 咆哮计时器
    i32 m_growlTime = 100;

    // 路径点
    std::vector<BlockPos> m_pathPoints;
    i32 m_currentPathPoint = 0;

    // 位置历史缓冲区（用于颈部和尾部动画）
    static constexpr i32 RING_BUFFER_SIZE = 64;
    f64 m_ringBuffer[RING_BUFFER_SIZE][3] = {};
    i32 m_ringBufferIndex = -1;

    // 转向速度
    f32 m_turnSpeed = 0.0f;

    // MC 1.16.5 常量
    static constexpr i32 DEATH_DURATION = 200;  // 死亡动画持续时间（ticks）
    static constexpr i32 XP_FIRST_KILL = 12000; // 首次击杀经验
    static constexpr i32 XP_SUBSEQUENT = 500;   // 后续击杀经验
};

} // namespace entity
} // namespace mc
