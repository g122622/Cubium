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

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/projectile/ProjectileDeflection.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class LivingEntity;
class Player;
class EntityTypeTag;

namespace entity {

/**
 * @brief 射线追踪结果类型
 */
enum class RayTraceResultType : u8 {
    Miss,  // 未命中
    Block, // 命中方块
    Entity // 命中实体
};

/**
 * @brief 射线追踪结果
 */
struct RayTraceResult {
    RayTraceResultType type = RayTraceResultType::Miss;
    Vector3 hitPosition;
    BlockPos blockPos;
    Direction face = Direction::None;
    mc::Entity* hitEntity = nullptr;

    static RayTraceResult miss() { return RayTraceResult{}; }

    static RayTraceResult block(const Vector3& pos, const BlockPos& blockPos, Direction hitFace = Direction::None)
    {
        RayTraceResult result;
        result.type = RayTraceResultType::Block;
        result.hitPosition = pos;
        result.blockPos = blockPos;
        result.face = hitFace;
        return result;
    }

    static RayTraceResult entity(const Vector3& pos, mc::Entity* hitEntity)
    {
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
 */
class ProjectileEntity : public Entity {
public:
    ~ProjectileEntity() override = default;

    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    // TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): 本类是 1.16.5 遗留中间层，
    // vanilla 1.21.11 类树已调整，本项目保留此层。若后期 vanilla 此层有同步字段须补
    // registerData+ClassRegisterGuard，当前仅占位 classInfo。
    static const EntityClassInfo& classInfo();

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }
    [[nodiscard]] f32 eyeHeight() const override { return 0.125f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    /**
     * @brief 处理投掷物实体受到伤害
     *
     * 投掷物不可被伤害，但当来源非无敌时标记 hurtMarked 以同步速度到客户端。
     * 这使得投掷物在被击中时会产生击退效果（如恶魂火球被反射时的速度同步）。
     * 对应 MC Java 的 Projectile.hurtServer()。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 投掷物属性 ==========

    /**
     * @brief 检查投掷物是否可以在指定位置与方块交互
     *
     * 重写 Entity::mayInteract。投掷物的交互权限取决于发射者：
     * - 发射者是玩家：委托给玩家的 mayInteract
     * - 发射者为空：允许交互
     * - 发射者是非玩家实体：取决于 MOB_GRIEFING 游戏规则
     *
     * 对应 MC Java 的 Projectile.mayInteract(ServerLevel, BlockPos)。
     *
     * @param world 世界引用
     * @param pos 目标方块位置
     * @return 如果允许交互返回 true
     */
    [[nodiscard]] bool mayInteract(IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 检查投射物是否可以破坏方块
     *
     * 当投射物的实体类型属于 #minecraft:impact_projectiles 标签
     * 且 PROJECTILES_CAN_BREAK_BLOCKS 游戏规则为 true 时返回 true。
     * 这是陶罐、紫颂花等方块被投射物命中时的额外检查条件。
     *
     * 对应 MC Java 的 Projectile.mayBreak(ServerLevel)。
     *
     * @param world 世界引用
     * @return 如果投射物可以破坏方块返回 true
     */
    [[nodiscard]] bool mayBreak(IWorld& world) const;

    /**
     * @brief 获取发射者
     * @return 发射此投掷物的实体（可能为nullptr）
     */
    [[nodiscard]] Entity* getShooter() const;

    /**
     * @brief 获取发射者UUID
     */
    [[nodiscard]] const std::string& shooterUuid() const { return m_shooterUuid; }

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
    void shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy);

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
     *
     * 当命中实体时，先检查该实体是否偏转弹射物（Entity::deflection），
     * 如果被偏转则不调用 onEntityHit，而是反转弹射物方向。
     */
    virtual void onImpact(const RayTraceResult& result);

    // ========== 偏转 ==========

    /**
     * @brief 应用偏转到弹射物
     *
     * 根据偏转类型修改弹射物的速度和旋转，并将偏转者设为新的发射者。
     * 同时记录 lastDeflectedBy 以防止同一实体连续偏转。
     *
     * 对应 MC Java 的 Projectile.deflect()。
     *
     * @param deflection 偏转类型
     * @param deflector 偏转者实体
     * @param wasPlayerDeflect 是否为玩家偏转（用于盾牌等场景）
     * @return 偏转是否成功
     */
    bool deflect(ProjectileDeflection deflection, Entity& deflector, bool wasPlayerDeflect = false);

    /**
     * @brief 偏转后的回调
     *
     * 子类可重写以在偏转后执行额外逻辑（如清除嵌入状态等）。
     *
     * @param wasPlayerDeflect 是否为玩家偏转
     */
    virtual void onDeflection(bool wasPlayerDeflect) { (void)wasPlayerDeflect; }

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
     * @param id 实体ID
     */
    explicit ProjectileEntity(EntityInstanceId id);

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
    std::string m_shooterUuid;                              // 发射者UUID
    EntityInstanceId m_shooterEntityId = INVALID_ENTITY_ID; // 发射者实体ID
    bool m_leftShooter = false;                             // 是否已离开发射者
    bool m_noGravity = false;                               // 是否不受重力

    /// 上一个偏转此弹射物的实体ID，防止同一实体连续偏转
    EntityInstanceId m_lastDeflectedById = INVALID_ENTITY_ID;
};

} // namespace entity
} // namespace mc
