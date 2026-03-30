#pragma once

#include "ProjectileEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 箭矢拾取状态
 */
enum class PickupStatus : u8 {
    Disallowed,    // 不允许拾取
    Allowed,       // 允许拾取
    CreativeOnly   // 仅创造模式拾取
};

/**
 * @brief 抽象箭矢实体基类
 *
 * 所有箭矢类型（普通箭、光灵箭、三叉戟等）的基类。
 * 提供穿透、暴击、伤害计算等通用功能。
 *
 * 参考 MC 1.16.5 AbstractArrowEntity
 */
class AbstractArrowEntity : public ProjectileEntity {
public:
    virtual ~AbstractArrowEntity() = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    void tick() override;

    // ========== 箭矢属性 ==========

    /**
     * @brief 获取伤害值
     */
    [[nodiscard]] f32 damage() const { return m_damage; }

    /**
     * @brief 设置伤害值
     */
    void setDamage(f32 damage) { m_damage = damage; }

    /**
     * @brief 获取击退强度
     */
    [[nodiscard]] i32 knockbackStrength() const { return m_knockbackStrength; }

    /**
     * @brief 设置击退强度
     */
    void setKnockbackStrength(i32 strength) { m_knockbackStrength = strength; }

    /**
     * @brief 是否暴击
     */
    [[nodiscard]] bool isCritical() const { return m_critical; }

    /**
     * @brief 设置暴击状态
     */
    void setCritical(bool critical) { m_critical = critical; }

    /**
     * @brief 获取穿透等级
     */
    [[nodiscard]] u8 pierceLevel() const { return m_pierceLevel; }

    /**
     * @brief 设置穿透等级
     */
    void setPierceLevel(u8 level) { m_pierceLevel = level; }

    /**
     * @brief 是否插在方块中
     */
    [[nodiscard]] bool isInGround() const { return m_inGround; }

    /**
     * @brief 获取拾取状态
     */
    [[nodiscard]] PickupStatus pickupStatus() const { return m_pickupStatus; }

    /**
     * @brief 设置拾取状态
     */
    void setPickupStatus(PickupStatus status) { m_pickupStatus = status; }

    /**
     * @brief 是否从弩射出
     */
    [[nodiscard]] bool shotFromCrossbow() const { return m_shotFromCrossbow; }

    /**
     * @brief 设置是否从弩射出
     */
    void setShotFromCrossbow(bool fromCrossbow) { m_shotFromCrossbow = fromCrossbow; }

    // ========== 物理 ==========

    [[nodiscard]] f32 getGravity() const override { return 0.05f; }
    [[nodiscard]] f32 getAirDrag() const override { return 0.99f; }
    [[nodiscard]] f32 getWaterDrag() const override { return 0.6f; }

    // ========== 箭矢特有方法 ==========

    /**
     * @brief 根据发射者设置附魔效果
     * @param shooter 发射者
     * @param baseVelocity 基础速度
     */
    void setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity);

    /**
     * @brief 玩家拾取箭矢
     * @param player 玩家
     * @return 是否成功拾取
     */
    bool onPlayerPickup(Player& player);

    /**
     * @brief 获取箭矢物品堆（用于拾取）
     * @return 箭矢物品堆
     */
    // virtual ItemStack getArrowStack() const = 0;

protected:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractArrowEntity(LegacyEntityType type, EntityId id);

    /**
     * @brief 箭矢命中实体时的处理
     */
    void onEntityHit(const RayTraceResult& result) override;

    /**
     * @brief 箭矢命中方块时的处理
     */
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 箭矢插在方块中的tick处理
     */
    void tickInGround();

    /**
     * @brief 检查是否应该从方块中脱落
     * @return 如果应该脱落返回true
     */
    bool shouldDespawn();

    // 属性
    f32 m_damage = 2.0f;            // 基础伤害
    i32 m_knockbackStrength = 0;    // 击退强度
    bool m_critical = false;        // 是否暴击
    u8 m_pierceLevel = 0;           // 穿透等级
    bool m_inGround = false;        // 是否插在方块中
    i32 m_ticksInGround = 0;        // 插在方块中的时间
    i32 m_arrowShake = 0;           // 箭矢抖动时间
    PickupStatus m_pickupStatus = PickupStatus::Disallowed;
    bool m_shotFromCrossbow = false;

    // 穿透追踪
    std::vector<EntityId> m_piercedEntities;  // 已穿透的实体ID
};

/**
 * @brief 普通箭矢实体
 *
 * 弓和弩射出的标准箭矢。
 *
 * 参考 MC 1.16.5 ArrowEntity
 */
class ArrowEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    ArrowEntity(LegacyEntityType type, EntityId id);

    /**
     * @brief 从发射者创建
     * @param shooter 发射者
     * @param world 世界
     */
    static std::unique_ptr<ArrowEntity> createFromShooter(
        LivingEntity& shooter, IWorld* world);

    // ========== Entity 接口重写 ==========

    void tick() override;

    // ========== 箭矢特有方法 ==========

    /**
     * @brief 设置箭矢颜色
     * @param color RGB颜色值
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 获取箭矢颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 设置是否为光灵箭
     */
    void setGlowing(bool glowing) { m_glowing = glowing; }

    /**
     * @brief 是否为光灵箭
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }

private:
    u32 m_color = 0xFFFFFFFF;  // 箭矢颜色（药水箭）
    bool m_glowing = false;    // 是否发光（光灵箭）
};

/**
 * @brief 光灵箭实体
 *
 * 光灵箭会让被命中的实体发光。
 *
 * 参考 MC 1.16.5 SpectralArrowEntity
 */
class SpectralArrowEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    SpectralArrowEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    void tick() override;

private:
    i32 m_glowDuration = 200;  // 发光持续时间（ticks）
};

} // namespace entity
} // namespace mc
