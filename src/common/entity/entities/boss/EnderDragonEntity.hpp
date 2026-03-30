#pragma once

#include "../../core/MobEntity.hpp"
#include "../../attribute/Attributes.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace entity {

/**
 * @brief Boss实体基类
 *
 * Boss级怪物的基类，具有生命条显示等特殊功能。
 */
class BossEntity : public MobEntity {
public:
    BossEntity(LegacyEntityType type, EntityId id);
    ~BossEntity() override = default;

    // ========== Boss 特有功能 ==========

    /**
     * @brief 获取Boss名称（显示在生命条上）
     */
    [[nodiscard]] virtual String getBossName() const = 0;

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
    [[nodiscard]] virtual u32 getHealthBarColor() const { return 0xFF0000; }  // 红色

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
 * @brief 末影龙实体
 *
 * 末地Boss，具有多种攻击模式和阶段。
 *
 * 参考 MC 1.16.5 EnderDragonEntity
 */
class EnderDragonEntity : public BossEntity {
public:
    /**
     * @brief 龙的阶段
     */
    enum class Phase : u8 {
        HoldingPattern,      // 盘旋
        StrafePlayer,        // 突袭玩家
        LandingApproach,     // 准备降落
        Landing,             // 降落
        Takeoff,             // 起飞
        Sitting,             // 坐在传送门上
        ChargingPlayer,      // 冲向玩家
        Dying,               // 死亡
        Hover               // 悬停
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    EnderDragonEntity(LegacyEntityType type, EntityId id);

    ~EnderDragonEntity() override = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 16.0f; }
    [[nodiscard]] f32 height() const override { return 8.0f; }
    [[nodiscard]] f32 eyeHeight() const override { return 6.0f; }

    void tick() override;

    // ========== BossEntity 接口 ==========

    [[nodiscard]] String getBossName() const override { return "Ender Dragon"; }
    [[nodiscard]] f32 getHealthBarRange() const override { return 256.0f; }
    [[nodiscard]] u32 getHealthBarColor() const override { return 0x800080; }  // 紫色

    // ========== 末影龙特有 ==========

    /**
     * @brief 获取当前阶段
     */
    [[nodiscard]] Phase phase() const { return m_phase; }

    /**
     * @brief 设置阶段
     */
    void setPhase(Phase phase) { m_phase = phase; }

    /**
     * @brief 获取龙息来源位置
     */
    [[nodiscard]] Vector3 breathOrigin() const;

    /**
     * @brief 是否在栖息点
     */
    [[nodiscard]] bool isSitting() const { return m_phase == Phase::Sitting; }

    /**
     * @brief 是否正在死亡
     */
    [[nodiscard]] bool isDying() const { return m_phase == Phase::Dying; }

    /**
     * @brief 处理龙息攻击
     */
    void breathAttack();

    /**
     * @brief 处理冲撞攻击
     */
    void chargeAttack();

    /**
     * @brief 处理龙火球攻击
     */
    void dragonFireballAttack();

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    /**
     * @brief 重生末影龙
     */
    static void respawnDragon(IWorld* world, BlockCoord portalPos);

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 更新AI阶段
     */
    void updatePhase();

    /**
     * @brief 更新盘旋行为
     */
    void updateHoldingPattern();

    /**
     * @brief 更新死亡阶段
     */
    void updateDying();

private:
    Phase m_phase = Phase::HoldingPattern;
    LivingEntity* m_attackTarget = nullptr;

    // 阶段计时
    i32 m_phaseTime = 0;
    i32 m_chargeTime = 0;

    // 路径点
    std::vector<Vector3> m_pathPoints;
    i32 m_currentPathPoint = 0;

    // 死亡动画
    i32 m_deathTime = 0;
    f32 m_deathY = 0.0f;

    // 龙息冷却
    i32 m_breathCooldown = 0;
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
        Head,       // 头部
        Neck,       // 颈部
        Body,       // 身体
        Tail,       // 尾部
        WingLeft,   // 左翼
        WingRight   // 右翼
    };

    /**
     * @brief 构造函数
     */
    EnderDragonPartEntity(LegacyEntityType type, EntityId id);

    ~EnderDragonPartEntity() override = default;

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 部件方法 ==========

    /**
     * @brief 获取父龙
     */
    [[nodiscard]] EnderDragonEntity* parentDragon() const { return m_parent; }

    /**
     * @brief 设置父龙
     */
    void setParentDragon(EnderDragonEntity* parent) { m_parent = parent; }

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
     * @param offsetX 相对父龙的X偏移
     * @param offsetY 相对父龙的Y偏移
     * @param offsetZ 相对父龙的Z偏移
     * @param size 部件大小
     */
    void updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 size);

private:
    EnderDragonEntity* m_parent = nullptr;
    Part m_part = Part::Body;
};

} // namespace entity
} // namespace mc
