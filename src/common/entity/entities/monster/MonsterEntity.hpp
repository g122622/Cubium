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
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class Player;
class ItemStack;
class DamageSource;

// 使用命名空间中的 SpawnReason
using world::spawn::SpawnReason;

/**
 * @brief 敌对生物基类
 *
 * 所有敌对生物（怪物）的基类，提供敌对行为的基础设施。
 *
 * 特性：
 * - 在黑暗中生成
 * - 在阳光下可能燃烧（亡灵类）
 * - 自动攻击玩家
 * - 敌对目标选择
 */
class MonsterEntity : public CreatureEntity, public entity::IMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    MonsterEntity(EntityInstanceId id);

    ~MonsterEntity() override = default;

    /// 本类继承链标识（parent = CreatureEntity::classInfo()）。见 Entity::classInfo()。
    // TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): 本类是 1.16.5 遗留中间层，
    // vanilla 1.21.11 类树已调整（PathfinderMob/WaterAnimal/AgeableWaterCreature 等），本项目保留此层。
    // 若后期 vanilla 此层有同步字段须补 registerData+ClassRegisterGuard，当前仅占位 classInfo。
    static const entity::EntityClassInfo& classInfo();

    [[nodiscard]] sound::SoundCategory getSoundCategory() const override { return sound::SoundCategory::Hostile; }

    // 禁止拷贝
    MonsterEntity(const MonsterEntity&) = delete;
    MonsterEntity& operator=(const MonsterEntity&) = delete;

    // 禁止移动（基类 CreatureEntity 不可移动）
    MonsterEntity(MonsterEntity&&) = delete;
    MonsterEntity& operator=(MonsterEntity&&) = delete;

    // ========== 声音重写 ==========

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取摔落声音
     * @param fallHeight 摔落高度（格数）
     */
    [[nodiscard]] std::optional<ResourceLocation> getFallSound(i32 fallHeight) const override;

    // ========== 伤害处理 ==========

    /**
     * @brief 受到伤害时的处理
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 生成条件 ==========

    /**
     * @brief 检查光照等级是否有效（用于怪物生成）
     * @param world 世界
     * @param pos 位置
     * @param random 随机数生成器
     * @return 如果光照条件允许生成返回true
     */
    [[nodiscard]] static bool isValidLightLevel(IWorld& world, const BlockPos& pos, math::Random& random);

    /**
     * @brief 检查怪物是否可以在指定位置生成（带光照检查）
     */
    [[nodiscard]] static bool canMonsterSpawnInLight(
        IWorld& world, SpawnReason reason, const BlockPos& pos, math::Random& random);

    /**
     * @brief 检查怪物是否可以在指定位置生成（无光照检查）
     */
    [[nodiscard]] static bool canMonsterSpawn(
        IWorld& world, SpawnReason reason, const BlockPos& pos, math::Random& random);

    // ========== 行为 ==========

    /**
     * @brief 是否在和平模式下消失
     */
    [[nodiscard]] bool isDespawnPeaceful() const override { return true; }

    /**
     * @brief 是否可以掉落战利品
     */
    [[nodiscard]] bool canDropLoot() const { return true; }

    // ========== 光照敏感 ==========

    /**
     * @brief 检查是否应该在阳光下燃烧
     * @return 如果应该在阳光下燃烧返回true
     */
    [[nodiscard]] virtual bool shouldBurnInDaylight() const { return m_burnsInDaylight; }

    /**
     * @brief 设置是否在阳光下燃烧
     * @param burn 是否燃烧
     */
    void setBurnsInDaylight(bool burn) { m_burnsInDaylight = burn; }

    // ========== 路径权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 返回 0.5F - 亮度，怪物偏好黑暗环境
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 路径权重（越高越好）
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

protected:
    bool m_burnsInDaylight = true;

    // ========== 敌对行为 ==========

    /**
     * @brief 检查是否应该攻击目标
     * @param target 目标实体
     * @return 如果应该攻击返回true
     */
    [[nodiscard]] virtual bool shouldAttack(LivingEntity* target) const;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 注册默认属性
     *
     * 在 MobEntity 基础上注册 ATTACK_DAMAGE
     */
    void registerAttributes() override;

    /**
     * @brief 注册 AI 目标
     *
     * 子类应重写此方法来注册敌对生物的基础行为：
     * - SwimGoal (优先级 0)
     * - HurtByTargetGoal (优先级 1)
     * - NearestAttackableTargetGoal (优先级 2)
     */
    void registerGoals() override;

    /**
     * @brief 处理阳光燃烧
     */
    void handleDaylightBurning();

    /**
     * @brief 更新空闲时间（基于亮度）
     */
    void updateIdleTimeBasedOnBrightness();
};

} // namespace mc
