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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/special/CamelEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 骆驼尸壳实体（CamelHusk）
 *
 * 沙漠中的敌对骆驼，由尸壳（Husk）在自然生成时 10% 概率召唤并骑乘
 * （对齐 vanilla Husk.finalizeSpawn → CamelHusk + Parched 双乘客）。
 *
 * 相对 Camel 的差异（对齐 vanilla 1.21.11 CamelHusk）：
 * - 敌对生物（MobCategory.MONSTER），远离玩家时消失（removeWhenFarAway=true）
 * - 不可繁殖（canMate=false / canFallInLove=false / getBreedOffspring=null）
 * - 被生物（Mob）骑乘时不可被拴绳牵引（isMobControlled → canBeLeashed=false）
 * - 任意玩家交互即标记为持久化（不退场）
 * - 食物为骆驼尸壳食物标签，声音全部使用 entity.camel_husk.*
 *
 * TODO: 冲锋速度修正 chargeSpeedModifier()=4.0（vanilla CamelHusk:124）依赖长矛
 *       （SpearApproach/SpearAttack/SpearRetreat/SpearUseGoal）与长矛物品体系，
 *       项目尚未实现长矛，待该子系统就绪后接入。
 * TODO: 骆驼尸壳食物标签 #minecraft:camel_husk_food（ItemTags）在项目物品标签中尚未定义，
 *       当前 isFoodItem 暂返回 false，待标签体系补齐后接入。
 */
class CamelHuskEntity : public CamelEntity {
public:
    CamelHuskEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~CamelHuskEntity() override = default;

    CamelHuskEntity(const CamelHuskEntity&) = delete;
    CamelHuskEntity& operator=(const CamelHuskEntity&) = delete;
    CamelHuskEntity(CamelHuskEntity&&) = delete;
    CamelHuskEntity& operator=(CamelHuskEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = CamelEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    /// 步骤音效音量（vanilla CamelHusk.playStepSound 用 0.4F，与 AbstractSkeleton 同）
    static constexpr f32 STEP_VOLUME = 0.4f;

    /**
     * @brief 远离玩家时正常消失（对齐 vanilla CamelHusk.removeWhenFarAway = true）
     *
     * Cubium 以 MobEntity::preventDespawn() 反向表达同一语义：默认实现
     * return isRiding()（被骑乘才免消失），此处直接返回 false 即"任何情况下都不免消失"。
     */
    [[nodiscard]] bool preventDespawn() const override { return false; }

    /// 是否被生物控制（首个乘客是 Mob；对齐 vanilla CamelHusk.isMobControlled）
    [[nodiscard]] bool isMobControlled() const;

    /// 被生物骑乘时不可被拴绳牵引（对齐 vanilla CamelHusk.canBeLeashed）
    [[nodiscard]] bool canBeLeashed() const override { return !isMobControlled(); }

    /// 不可繁殖（对齐 vanilla CamelHusk.canMate = false）
    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override
    {
        (void)other;
        return false;
    }

    /// 不可进入求偶状态（对齐 vanilla CamelHusk.canFallInLove = false）
    [[nodiscard]] bool canBreed() const override { return false; }

    /// 任意玩家交互都标记持久化（对齐 vanilla CamelHusk.interact）
    ActionResultType interactMob(Player& player, Hand hand) override;

    /// 食物：#minecraft:camel_husk_food（TODO：物品标签体系补齐后接入）
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const override;

    // ========== 声音（全部使用 entity.camel_husk.*） ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    std::optional<ResourceLocation> getEatSound() const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc