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

#include "AbstractSkeletonEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"

#include <memory>

namespace mc {

// 前向声明
namespace entity {
class ArrowEntity;
}

/**
 * @brief 干枯骷髅实体（Parched）
 *
 * 骷髅的沙漠变种（对应原版 MC 1.21.11 Parched），主要特征：
 * - 使用弓箭远程攻击，射出的箭矢附带 30 秒虚弱（WEAKNESS 600 ticks）
 * - 作为亡灵骷髅变种会在阳光下燃烧（继承基类 shouldBurnInDaylight 默认 true）
 * - 生命值 16（普通骷髅为 20），攻击间隔比普通骷髅慢：困难 50 / 非困难 70
 * - 自身免疫虚弱效果（canBeAffected 对 WEAKNESS 恒返回 false）
 * - 作为 #minecraft:skeletons / #minecraft:undead 标签成员，具备亡灵全部特性
 *
 * 参考 MC Java: net.minecraft.world.entity.monster.skeleton.Parched
 */
class ParchedEntity : public AbstractSkeletonEntity {
public:
    ParchedEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~ParchedEntity() override = default;

    ParchedEntity(const ParchedEntity&) = delete;
    ParchedEntity& operator=(const ParchedEntity&) = delete;
    ParchedEntity(ParchedEntity&&) = delete;
    ParchedEntity& operator=(ParchedEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 本类继承链标识（parent = AbstractSkeletonEntity::classInfo()）。见 Entity::classInfo()。
     */
    static const entity::EntityClassInfo& classInfo();

    /// 箭矢附带的虚弱持续时间（ticks），600 ticks = 30 秒，对应原版 MobEffects.WEAKNESS, 600
    static constexpr i32 WEAKNESS_DURATION_TICKS = 600;
    /// 箭矢附带效果的等级：amplifier=0 即等级 I
    static constexpr i32 WEAKNESS_AMPLIFIER = 0;
    /// 干枯骷髅生命值（普通骷髅为 20），对应原版 createAttributes().add(MAX_HEALTH, 16.0)
    static constexpr f32 PARCHED_MAX_HEALTH = 16.0f;

    /**
     * @brief 为射出的箭矢附加虚弱效果
     *
     * 重写基类 customizeArrow 钩子，attackEntityWithRangedAttack 发射箭矢前调用。
     * 对应原版 Parched.getArrow()：arrow.addEffect(MobEffectInstance(WEAKNESS, 600))，
     * 箭矢命中生物且 hurt 成功后由 ArrowEntity::doPostHurtEffects 施加 30 秒虚弱 I。
     */
    void customizeArrow(entity::ArrowEntity& arrow) override;

    /**
     * @brief 困难难度最小攻击间隔（50 ticks，比普通骷髅 20 慢）
     *
     * 对应原版 Parched.getHardAttackInterval() = 50。
     */
    [[nodiscard]] i32 getHardAttackInterval() const override { return INCREASED_HARD_ATTACK_INTERVAL; }

    /**
     * @brief 非困难难度最小攻击间隔（70 ticks，比普通骷髅 40 慢）
     *
     * 对应原版 Parched.getAttackInterval() = 70。
     */
    [[nodiscard]] i32 getAttackInterval() const override { return INCREASED_NORMAL_ATTACK_INTERVAL; }

    // ========== 药水效果免疫 ==========

    /**
     * @brief 检查是否可被施加指定药水效果
     *
     * 对齐原版 Parched.canBeAffected：
     *   return p_480498_.getEffect() == MobEffects.WEAKNESS ? false : super.canBeAffected(p_480498_);
     * 对应 Cubium 的 LivingEntity::isPotionApplicable（EffectManager::addEffect 调用）。
     *
     * @param effect 待施加的效果实例
     * @return 若为虚弱效果返回 false，否则委托基类判定
     */
    [[nodiscard]] bool isPotionApplicable(const entity::effect::EffectInstance& effect) const override;

protected:
    void registerAttributes() override;
};

} // namespace mc