#pragma once

#include "../../core/Entity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// 前向声明
class LivingEntity;
class Player;

namespace entity {

/**
 * @brief 射线追踪结果类型
 */
enum class RayTraceResultType : u8 {
    Miss,   // 未命中
    Block,  // 命中方块
    Entity  // 命中实体
};

/**
 * @brief 射线追踪结果
 */
struct RayTraceResult {
    RayTraceResultType type = RayTraceResultType::Miss;
    Vector3 hitPosition;
    BlockPos blockPos;
    mc::Entity* hitEntity = nullptr;

    static RayTraceResult miss() {
        return RayTraceResult{};
    }

    static RayTraceResult block(const Vector3& pos, const BlockPos& blockPos) {
        RayTraceResult result;
        result.type = RayTraceResultType::Block;
        result.hitPosition = pos;
        result.blockPos = blockPos;
        return result;
    }

    static RayTraceResult entity(const Vector3& pos, mc::Entity* hitEntity) {
        RayTraceResult result;
        result.type = RayTraceResultType::Entity;
        result.hitPosition = pos;
        result.hitEntity = hitEntity;
        return result;
    }
};

/**
 * @brief 投掷物实体基类
 *
 * 所有投掷物（箭矢、雪球、火球等）的基类。
 * 提供发射、飞行、碰撞检测等通用功能。
 *
 * 参考 MC 1.16.5 ProjectileEntity
 */
class ProjectileEntity : public Entity {
public:
    virtual ~ProjectileEntity() = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }
    [[nodiscard]] f32 eyeHeight() const override { return 0.125f; }

    void tick() override;

    // ========== 投掷物属性 ==========

    /**
     * @brief 获取发射者
     * @return 发射此投掷物的实体（可能为nullptr）
     */
    [[nodiscard]] Entity* getShooter() const;

    /**
     * @brief 获取发射者UUID
     */
    [[nodiscard]] const String& shooterUuid() const { return m_shooterUuid; }

    /**
     * @brief 设置发射者
     * @param shooter 发射者实体
     */
    void setShooter(Entity* shooter);

    /**
     * @brief 检查是否已经离开发射者
     *
     * 投掷物在发射后需要离开发射者的碰撞箱才能伤害发射者
     */
    [[nodiscard]] bool hasLeftShooter() const { return m_leftShooter; }

    /**
     * @brief 检查是否不受重力影响
     */
    [[nodiscard]] bool hasNoGravity() const { return m_noGravity; }

    /**
     * @brief 设置是否受重力影响
     */
    void setNoGravity(bool noGravity) { m_noGravity = noGravity; }

    // ========== 发射方法 ==========

    /**
     * @brief 向指定方向发射
     * @param x X方向分量
     * @param y Y方向分量
     * @param z Z方向分量
     * @param velocity 初始速度
     * @param inaccuracy 散布精度（越大越不准）
     */
    void shoot(f32 x, f32 y, f32 z, f32 velocity, f32 inaccuracy);

    /**
     * @brief 从实体位置向指定角度发射
     * @param shooter 发射者
     * @param pitch 俯仰角（度）
     * @param yaw 偏航角（度）
     * @param pitchOffset 俯仰角偏移
     * @param velocity 初始速度
     * @param inaccuracy 散布精度
     */
    void shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset,
                   f32 velocity, f32 inaccuracy);

    // ========== 碰撞检测 ==========

    /**
     * @brief 检查是否可以命中指定实体
     * @param target 目标实体
     * @return 如果可以命中返回true
     */
    [[nodiscard]] virtual bool canHitEntity(const mc::Entity& target) const;

    /**
     * @brief 处理命中实体
     * @param result 命中结果
     */
    virtual void onEntityHit(const RayTraceResult& result);

    /**
     * @brief 处理命中方块
     * @param result 命中结果
     */
    virtual void onBlockHit(const RayTraceResult& result);

    /**
     * @brief 处理碰撞
     * @param result 碰撞结果
     */
    virtual void onImpact(const RayTraceResult& result);

    // ========== 物理 ==========

    /**
     * @brief 获取重力加速度
     * @return 每tick的重力加速度
     */
    [[nodiscard]] virtual f32 getGravity() const { return 0.03f; }

    /**
     * @brief 获取空气阻力
     * @return 空气阻力系数（0-1）
     */
    [[nodiscard]] virtual f32 getAirDrag() const { return 0.99f; }

    /**
     * @brief 获取水中阻力
     * @return 水中阻力系数（0-1）
     */
    [[nodiscard]] virtual f32 getWaterDrag() const { return 0.8f; }

protected:
    /**
     * @brief 构造函数（子类调用）
     * @param type 实体类型
     * @param id 实体ID
     */
    ProjectileEntity(LegacyEntityType type, EntityId id);

    /**
     * @brief 更新旋转（根据速度方向）
     */
    void updateRotation();

    /**
     * @brief 检查是否离开发射者
     */
    bool checkLeftShooter();

    /**
     * @brief 执行射线追踪
     * @return 命中结果
     */
    RayTraceResult performRayTrace();

    /**
     * @brief 执行实体射线追踪
     * @param start 起点
     * @param end 终点
     * @return 命中的实体（如果有）
     */
    virtual RayTraceResult rayTraceEntities(const Vector3& start, const Vector3& end);

    /**
     * @brief 执行方块射线追踪
     * @param start 起点
     * @param end 终点
     * @return 命中的方块（如果有）
     */
    RayTraceResult rayTraceBlocks(const Vector3& start, const Vector3& end);

    // 发射者信息
    String m_shooterUuid;           // 发射者UUID
    EntityId m_shooterEntityId = INVALID_ENTITY_ID;  // 发射者实体ID
    bool m_leftShooter = false;     // 是否已离开发射者
    bool m_noGravity = false;       // 是否不受重力
};

} // namespace entity
} // namespace mc
