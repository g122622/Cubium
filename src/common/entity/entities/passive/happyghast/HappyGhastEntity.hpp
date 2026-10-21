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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/chunk/data/IChunk.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc {

// 前向声明
class IWorld;
class ItemStack;
class DamageSource;

namespace entity::ai {
class Goal;
} // namespace entity::ai

/**
 * @brief 快乐恶魂实体
 *
 * 1.21 新增的被动飞行生物，由干燥恶魂方块在水中孵化（幼年个体）或通过幼体成长获得。
 * 成体会自动远离地面悬浮巡游，可载多名乘客。
 *
 * 参考：net.minecraft.world.entity.animal.happyghast.HappyGhast
 *
 * @note 本项目无 Brain 系统，vanilla HappyGhastAi 的 IDLE 活动改以 GoalSelector 的
 *       GhastHappyFloatGoal（幼体漂浮）+ GhastRandomFlyGoal（成年随机飞行）实现，
 *       与恶魂共用同一套飞行移动控制器与飞行物理（travelFlying）。
 * @note 未实现（TODO）：
 *       - 骑乘/多人载客（MAX_PASSANGERS）与鞍具（HARNESS）装备槽语义；
 *       - 静止超时（serverStillTimeout / STAYS_STILL 同步字段）与头盔装饰音效；
 *       - 成年/幼年切换时的移动控制与 goal 集合切换（ageBoundaryReached）。
 */
class HappyGhastEntity : public AnimalEntity {
public:
    /**
     * @brief 幼体缩放（对齐 vanilla HappyGhast.BABY_SCALE）
     *
     * vanilla 幼体尺寸为成体的 0.2375 倍（远小于常规动物的 0.5 倍）。
     */
    static constexpr f32 HAPPY_GHAST_BABY_SCALE = 0.2375f;

    /**
     * @brief 构造快乐恶魂
     * @param id 实体 ID
     * @param registry ECS 注册表
     */
    HappyGhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~HappyGhastEntity() override = default;

    // 禁止拷贝
    HappyGhastEntity(const HappyGhastEntity&) = delete;
    HappyGhastEntity& operator=(const HappyGhastEntity&) = delete;

    // 允许移动
    HappyGhastEntity(HappyGhastEntity&&) = delete;
    HappyGhastEntity& operator=(HappyGhastEntity&&) = delete;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 新创建的实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    // ========== 年龄 / 尺寸 ==========

    /// 幼体缩放比例（对齐 vanilla HappyGhast.BABY_SCALE = 0.2375）
    [[nodiscard]] f32 babyScale() const override { return HAPPY_GHAST_BABY_SCALE; }

    /// 幼体基础宽度（缩放由 babyScale 承担，故此处返回成体宽度）
    [[nodiscard]] f32 getBaseWidth() const override;
    /// 幼体基础高度（缩放由 babyScale 承担，故此处返回成体高度）
    [[nodiscard]] f32 getBaseHeight() const override;

    // ========== 繁殖 ==========

    /// 繁殖/食物判定：HAPPY_GHAST_FOOD 标签（雪球），对齐 vanilla HappyGhast.isFood
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /// 快乐恶魂不能繁殖（对齐 vanilla HappyGhast.canFallInLove = false 与 getBreedOffspring 的产卵语义）
    [[nodiscard]] bool canBreed() const override;

    /**
     * @brief 生成幼体
     *
     * @note vanilla 繁殖产出的仍是快乐恶魂幼体，但实际孵化路径为干燥恶魂方块；
     *       此处返回幼体以满足 BreedGoal 契约。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 飞行物理 ==========

    /**
     * @brief 飞行移动（对齐 vanilla LivingEntity.travelFlying，加速 0.02、阻力 0.91/0.8/0.5）
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

    // ========== 受伤 ==========

    /// 对齐 vanilla HappyGhast.checkFallDamage：空实现，飞行生物免疫摔落伤害。
    void handleFallDamage(f32 distance, f32 damageMultiplier) override;

    // ========== 声音 ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /// 音量：幼体 1.0，成体 4.0（对齐 vanilla HappyGhast.getSoundVolume）
    [[nodiscard]] f32 getSoundVolume() const override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;
};

} // namespace mc