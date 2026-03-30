#pragma once

#include "ProjectileEntity.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 抽象火球实体基类
 *
 * 恶魂火球、烈焰人火球等的基类。
 * 使用加速度而非速度，持续追踪目标。
 *
 * 参考 MC 1.16.5 AbstractFireballEntity
 */
class AbstractFireballEntity : public ProjectileEntity {
public:
    virtual ~AbstractFireballEntity() = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

    void tick() override;

    // ========== 火球属性 ==========

    /**
     * @brief 获取加速度X
     */
    [[nodiscard]] f32 accelerationX() const { return m_accelerationX; }

    /**
     * @brief 获取加速度Y
     */
    [[nodiscard]] f32 accelerationY() const { return m_accelerationY; }

    /**
     * @brief 获取加速度Z
     */
    [[nodiscard]] f32 accelerationZ() const { return m_accelerationZ; }

    /**
     * @brief 设置加速度
     */
    void setAcceleration(f32 x, f32 y, f32 z) {
        m_accelerationX = x;
        m_accelerationY = y;
        m_accelerationZ = z;
    }

    // ========== 物理 ==========

    [[nodiscard]] f32 getGravity() const override { return 0.0f; }  // 火球不受重力
    [[nodiscard]] f32 getAirDrag() const override { return 0.95f; }

protected:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractFireballEntity(LegacyEntityType type, EntityId id);

    // 加速度（每tick增加的速度）
    f32 m_accelerationX = 0.0f;
    f32 m_accelerationY = 0.0f;
    f32 m_accelerationZ = 0.0f;
};

/**
 * @brief 火球实体（恶魂火球）
 *
 * 恶魂发射的大型火球，命中后爆炸。
 *
 * 参考 MC 1.16.5 FireballEntity
 */
class FireballEntity : public AbstractFireballEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    FireballEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

    // ========== 火球属性 ==========

    /**
     * @brief 获取爆炸威力
     */
    [[nodiscard]] i32 explosionPower() const { return m_explosionPower; }

    /**
     * @brief 设置爆炸威力
     */
    void setExplosionPower(i32 power) { m_explosionPower = power; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    i32 m_explosionPower = 1;  // 爆炸威力
};

/**
 * @brief 小火球实体（烈焰人火球）
 *
 * 烈焰人发射的小型火球，不会爆炸，只会造成伤害和点燃。
 *
 * 参考 MC 1.16.5 SmallFireballEntity
 */
class SmallFireballEntity : public AbstractFireballEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    SmallFireballEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
};

/**
 * @brief 龙火球实体
 *
 * 末影龙发射的紫色火球。
 *
 * 参考 MC 1.16.5 DragonFireballEntity
 */
class DragonFireballEntity : public AbstractFireballEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    DragonFireballEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
};

/**
 * @brief 凋灵之首实体
 *
 * 凋灵发射的爆炸头颅。
 *
 * 参考 MC 1.16.5 WitherSkullEntity
 */
class WitherSkullEntity : public AbstractFireballEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    WitherSkullEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    /**
     * @brief 是否为蓝色凋灵之首
     */
    [[nodiscard]] bool isBlue() const { return m_blue; }

    /**
     * @brief 设置是否为蓝色凋灵之首
     */
    void setBlue(bool blue) { m_blue = blue; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    bool m_blue = false;  // 蓝色凋灵之首（更强力的爆炸）
};

} // namespace entity
} // namespace mc
