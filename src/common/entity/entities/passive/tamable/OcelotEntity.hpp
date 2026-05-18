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
* THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#pragma once

#include "../../../../core/Types.hpp"
#include "../basic/AnimalEntity.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

namespace entity::ai::goal {
// Forward declarations for internal classes
class OcelotAvoidPlayerGoal;
class OcelotTemptGoal;
class OcelotAttackGoal;
}

/**
 * @brief 豹猫实体
 *
 * 生活在丛林中的害羞动物。
 *
 * 特性：
 * - 信任机制：不完全驯服，建立信任后不再逃跑
 * - 逃跑：未信任时靠近玩家会逃跑，需要悄悄靠近
 * - 繁殖：使用生鳕鱼和生鲑鱼
 * - 狩猎：会攻击小鸡和小海龟
 * - 免疫摔落伤害
 *
 * 参考 MC 1.16.5 OcelotEntity
 */
class OcelotEntity : public AnimalEntity {
public:
    /**
     * @brief 豹猫类型
     *
     * 注意：在 MC 1.16.5 中，豹猫只有一种野生皮肤
     * 这里的类型是为了兼容性保留的
     */
    enum class OcelotType : u8 {
        Wild = 0,    // 野生豹猫
        Tuxedo = 1,  // 黑白猫（已废弃）
        Tabby = 2,   // 虎斑猫（已废弃）
        Red = 3,     // 红猫（已废弃）
        Siamese = 4, // 暹罗猫（已废弃）
        British = 5, // 英短（已废弃）
        Calico = 6,  // 三花猫（已废弃）
        Persian = 7, // 波斯猫（已废弃）
        Ragdoll = 8, // 布偶猫（已废弃）
        White = 9,   // 白猫（已废弃）
        Jellie = 10  // Jellie猫（已废弃）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    OcelotEntity(EntityId id);
    ~OcelotEntity() override = default;

    // 禁止拷贝
    OcelotEntity(const OcelotEntity&) = delete;
    OcelotEntity& operator=(const OcelotEntity&) = delete;

    // 允许移动
    OcelotEntity(OcelotEntity&&) = default;
    OcelotEntity& operator=(OcelotEntity&&) = default;

    /**
     * @brief 创建豹猫实体
     * @param world 世界实例
     * @return 新的豹猫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 信任系统 ==========

    /**
     * @brief 是否信任玩家
     * @param playerId 玩家ID
     * @return 是否信任
     */
    [[nodiscard]] bool trustsPlayer(u64 playerId) const;

    /**
     * @brief 设置信任玩家
     * @param playerId 玩家ID
     * @param trust 是否信任
     */
    void setPlayerTrust(u64 playerId, bool trust);

    /**
     * @brief 是否已被信任
     * 豹猫的"驯服"实际上是建立信任
     */
    [[nodiscard]] bool isTrusting() const { return m_trusting; }

    /**
     * @brief 设置信任状态
     */
    void setTrusting(bool trusting);

    /**
     * @brief 获取信任玩家ID
     */
    [[nodiscard]] u64 getTrustingPlayerId() const { return m_trustingPlayerId; }

    // ========== 逃跑状态 ==========

    /**
     * @brief 是否正在逃跑
     */
    [[nodiscard]] bool isFleeing() const { return m_fleeing; }

    /**
     * @brief 设置逃跑状态
     */
    void setFleeing(bool fleeing) { m_fleeing = fleeing; }

    // ========== 类型 ==========

    /**
     * @brief 获取豹猫类型
     */
    [[nodiscard]] OcelotType getOcelotType() const { return m_ocelotType; }

    /**
     * @brief 设置豹猫类型
     */
    void setOcelotType(OcelotType type) { m_ocelotType = type; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 豹猫使用生鱼繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.3f : 0.6f; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 更新 AI 任务（姿态和奔跑状态）
     * MC 1.16.5: 根据移动速度设置潜行/奔跑姿态
     */
    void updateAITasks() override;

    /**
     * @brief 检查是否可以消失
     * MC 1.16.5: 未信任的豹猫存在超过 2400 tick (2分钟) 后可以消失
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override;

    /**
     * @brief 摔落伤害处理
     * MC 1.16.5: 豹猫免疫摔落伤害
     */
    [[nodiscard]] bool canTakeFallDamage() const { return false; }

    /**
     * @brief 作为生物攻击目标
     * MC 1.16.5: 豹猫攻击伤害
     */
    [[nodiscard]] bool attackEntityAsMob(LivingEntity& target) override;

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家交互
     * MC 1.16.5: OcelotEntity.func_230254_b_()
     * 喂食生鱼建立信任
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    void registerData() override;

private:
    // 信任状态
    bool m_trusting = false;
    u64 m_trustingPlayerId = 0;

    // 逃跑状态
    bool m_fleeing = false;

    // 类型
    OcelotType m_ocelotType = OcelotType::Wild;

    // AI 目标（需要动态管理）
    entity::ai::goal::OcelotAvoidPlayerGoal* m_avoidPlayerGoal = nullptr;
    entity::ai::goal::OcelotTemptGoal* m_temptGoal = nullptr;

    // 常量 - 参考 MC 1.16.5 OcelotEntity
    static constexpr f64 TEMPT_SPEED = 0.6;        // 诱惑速度
    static constexpr f64 AVOID_FAR_SPEED = 0.8;    // 远距离逃避速度
    static constexpr f64 AVOID_NEAR_SPEED = 1.33;  // 近距离逃避速度
    static constexpr f32 AVOID_DISTANCE = 16.0f;   // 逃避检测距离
    static constexpr f32 ATTACK_DAMAGE = 3.0f;     // 攻击伤害
    static constexpr i32 DESPAWN_TICKS = 2400;     // 消失所需tick数（2分钟）

    /**
     * @brief 根据信任状态动态调整 AI 目标
     * MC 1.16.5: func_213529_dV()
     */
    void setupTrustingAI();

    /**
     * @brief 生成信任粒子效果
     * @param success 是否成功建立信任
     */
    void spawnTrustingParticles(bool success);

    // 内部类声明
    friend class entity::ai::goal::OcelotAvoidPlayerGoal;
    friend class entity::ai::goal::OcelotTemptGoal;
    friend class entity::ai::goal::OcelotAttackGoal;
};

namespace entity::ai::goal {

/**
 * @brief 豹猫躲避玩家目标
 *
 * 未信任时躲避玩家，信任后停止躲避。
 *
 * 参考 MC 1.16.5 OcelotEntity.AvoidEntityGoal
 */
class OcelotAvoidPlayerGoal : public AvoidEntityGoal {
public:
    /**
     * @brief 构造函数
     * @param ocelot 豹猫实体
     * @param avoidDistance 检测距离
     * @param farSpeed 远距离逃跑速度
     * @param nearSpeed 近距离逃跑速度
     */
    OcelotAvoidPlayerGoal(OcelotEntity* ocelot, f32 avoidDistance, f64 farSpeed, f64 nearSpeed);

    ~OcelotAvoidPlayerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;

private:
    OcelotEntity* m_ocelot;
};

/**
 * @brief 豹猫诱惑目标
 *
 * 被生鱼诱惑，但会对玩家移动敏感（未信任时）。
 *
 * 参考 MC 1.16.5 OcelotEntity.TemptGoal
 */
class OcelotTemptGoal : public TemptGoal {
public:
    /**
     * @brief 构造函数
     * @param ocelot 豹猫实体
     * @param speed 移动速度
     * @param itemPredicate 物品检查函数
     * @param scaredByMovement 是否被玩家移动吓跑
     */
    OcelotTemptGoal(OcelotEntity* ocelot, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement);

    ~OcelotTemptGoal() override = default;

protected:
    /**
     * @brief 检查是否被玩家移动吓跑
     * MC 1.16.5: 只有未信任时才会被移动吓跑
     */
    [[nodiscard]] bool isScaredByPlayerMovement() const override;

private:
    OcelotEntity* m_ocelot;
};

/**
 * @brief 豹猫攻击目标
 *
 * 使豹猫攻击目标实体（小鸡、小海龟）。
 * 与 MeleeAttackGoal 类似，但有特殊的攻击范围和距离检查。
 *
 * 参考 MC 1.16.5 OcelotAttackGoal
 */
class OcelotAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ocelot 豹猫实体
     */
    explicit OcelotAttackGoal(OcelotEntity* ocelot);

    ~OcelotAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

private:
    OcelotEntity* m_ocelot;
    LivingEntity* m_target = nullptr;
    i32 m_attackCooldown = 0;

    static constexpr f32 STOP_ATTACK_DISTANCE_SQ = 225.0f; // 15*15 停止追踪距离
    static constexpr i32 ATTACK_COOLDOWN_TICKS = 20;       // 攻击冷却（ticks）
};

} // namespace entity::ai::goal
} // namespace mc
