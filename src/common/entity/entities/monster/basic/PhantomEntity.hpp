/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include <memory>

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/FlyingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <optional>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;

/**
 * @brief 幻翼实体
 *
 * 在夜间生成的飞行敌对生物，会俯冲攻击玩家。
 * 使用专用的 PhantomMovementController 和 PhantomLookController 控制飞行，
 * 客户端侧有翅膀拍打音效和菌丝粒子效果。
 *
 * 特性：
 * - 飞行：使用 PhantomMovementController 直接操控速度向量
 * - 俯冲攻击：环绕目标后俯冲攻击（CIRCLE/SWOOP 阶段切换）
 * - 阳光燃烧：在阳光下燃烧（亡灵生物属性）
 * - 变体：有不同大小（0-64），影响碰撞箱和攻击力
 * - 猫驱赶：附近有猫时会停止攻击
 */
class PhantomEntity : public FlyingEntity {
public:
    /**
     * @brief 幻翼攻击模式
     */
    enum class AttackPhase : u8 {
        CIRCLE, // 环绕
        SWOOP   // 俯冲
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    PhantomEntity(EntityInstanceId id);
    ~PhantomEntity() override = default;

    // 禁止拷贝
    PhantomEntity(const PhantomEntity&) = delete;
    PhantomEntity& operator=(const PhantomEntity&) = delete;

    // 允许移动
    PhantomEntity(PhantomEntity&&) = delete;
    PhantomEntity& operator=(PhantomEntity&&) = delete;

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 尺寸系统 ==========

    /**
     * @brief 获取幻翼大小
     */
    [[nodiscard]] i32 getPhantomSize() const { return m_phantomSize; }

    /**
     * @brief 设置幻翼大小
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

    // ========== 环绕位置 ==========

    /**
     * @brief 获取环绕位置
     */
    [[nodiscard]] BlockPos orbitPosition() const { return m_orbitPosition; }

    /**
     * @brief 设置环绕位置
     */
    void setOrbitPosition(const BlockPos& pos) { m_orbitPosition = pos; }

    /**
     * @brief 获取环绕偏移（移动目标点）
     *
     * PhantomMovementController 根据此点控制飞行方向。
     */
    [[nodiscard]] Vector3 orbitOffset() const { return m_orbitOffset; }

    /**
     * @brief 设置环绕偏移（移动目标点）
     */
    void setOrbitOffset(const Vector3& offset) { m_orbitOffset = offset; }

    // ========== 属性 ==========

    /**
     * @brief 获取生物类型（亡灵）
     */
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }

    /**
     * @brief 检查是否可以攻击指定类型的实体
     *
     * 覆盖 Mob 基类排除恶魂的限制，因为幻翼本身是飞行生物，
     * 具备攻击空中目标的能力。
     */
    [[nodiscard]] bool canAttackType(const entity::EntityType& type) const override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return height() * 0.35f; }

    // ========== 翅膀拍打 ==========

    /**
     * @brief 检查当前tick是否在拍打翅膀
     *
     * 翅膀拍打周期基于实体ID偏移，每 TICKS_PER_FLAP tick 拍打一次。
     */
    [[nodiscard]] bool isFlapping() const
    {
        return (static_cast<i64>(m_ticksExisted) + m_uniqueFlapOffset) % TICKS_PER_FLAP == 0;
    }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 飞行物理
     *
     * 使用飞行惯性因子 0.2，委托 FlyingEntity::travel() 处理飞行物理。
     */
    void travel(f32 x, f32 y, f32 z) override;

    /**
     * @brief 初始生成设置
     *
     * 设置环绕位置为生成位置上方5格，幻翼大小默认为0。
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    /**
     * @brief 客户端侧更新：身体旋转同步
     *
     * 在客户端侧每tick将身体和头部旋转同步为偏航角，
     * 使幻翼的整体朝向与其飞行方向一致。
     */
    void _clientTickBodyRotation();

    /**
     * @brief 客户端侧更新：翅膀拍打音效和菌丝粒子
     *
     * 在翅膀拍打的过零点播放拍打音效，
     * 每tick在翼尖位置生成菌丝粒子。
     */
    void _clientTickEffects();

    // 幻翼大小（0-64）
    i32 m_phantomSize = 0;

    // 攻击阶段
    AttackPhase m_attackPhase = AttackPhase::CIRCLE;

    // 环绕位置
    BlockPos m_orbitPosition;

    // 环绕偏移（移动目标点，PhantomMovementController 读取此值控制飞行）
    Vector3 m_orbitOffset;

    // 翅膀拍打的唯一偏移量（基于实体ID，使不同幻翼的拍打节奏错开）
    i64 m_uniqueFlapOffset = 0;

    // 常量
    static constexpr f32 BASE_ATTACK_DAMAGE = 6.0f;
    static constexpr f32 SIZE_ATTACK_BONUS = 1.0f;
    static constexpr i32 MAX_PHANTOM_SIZE = 64;

    /**
     * @brief 翅膀拍打角速度（度/tick）
     *
     * 与 MC 原版 Phantom.FLAP_DEGREES_PER_TICK 一致。
     */
    static constexpr f32 FLAP_DEGREES_PER_TICK = 7.448451F;

    /**
     * @brief 每次拍打经历的tick数
     *
     * ceil(24.166098) = 25，与 MC 原版 Phantom.TICKS_PER_FLAP 一致。
     */
    static constexpr i32 TICKS_PER_FLAP = 25;
};

} // namespace mc
