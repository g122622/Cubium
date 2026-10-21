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

#include "HappyGhastEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/GhastMovementController.hpp"
#include "common/entity/ai/goal/goals/special/GhastGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

#include <memory>
#include <optional>

namespace mc {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AnimalEntity::classInfo()）。
const entity::EntityClassInfo& HappyGhastEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"HappyGhastEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

std::unique_ptr<Entity> HappyGhastEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<HappyGhastEntity>(EntityInstanceId(0), registry);
}

HappyGhastEntity::HappyGhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 飞行生物：与恶魂共用带碰撞检测的飞行移动控制器。
    m_moveController = std::make_unique<entity::ai::controller::GhastMovementController>(this);

    // 飞行生物不受重力（飞行物理由 travel() 的 travelFlying 分支承担）。
    setNoGravity(true);

    // 补调 registerGoals / registerAttributes：AnimalEntity 构造期间 vtable 指向基类，
    // 派生 override 永不执行，须在派生类构造显式调用。
    registerGoals();
    registerAttributes();
}

// ========== 年龄 / 尺寸 ==========

f32 HappyGhastEntity::getBaseWidth() const
{
    // 成体宽度 4.0（对齐 vanilla HappyGhast.getDefaultDimensions 继承 Ghast 的 4×4）；
    // 幼体尺寸由 AgeableEntity::width() 经 babyScale()（0.2375）缩放得到。
    return 4.0f;
}

f32 HappyGhastEntity::getBaseHeight() const
{
    return 4.0f;
}

// ========== 繁殖 ==========

bool HappyGhastEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla HappyGhast：动物繁殖由物品标签（HAPPY_GHAST_FOOD）驱动，vanilla 未重写 isFood；
    // 本项目保留繁殖契约但未接入该物品标签（项目无物品标签体系），故不认任何繁殖物品。
    // TODO: ItemTags::HAPPY_GHAST_FOOD 接入后改为查询标签。
    MC_UNUSED(itemStack);
    return false;
}

bool HappyGhastEntity::canMateWith(const AnimalEntity& other) const
{
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> HappyGhastEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<HappyGhastEntity>(EntityInstanceId(0), *registry);
    baby->setTypeId(entity::EntityTypeKeys::HAPPY_GHAST); // 工厂绕过补救：直接构造缺 typeId
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

// ========== 飞行物理 ==========

void HappyGhastEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 对齐 vanilla LivingEntity.travelFlying（与 GhastEntity::travel 同一公式）：
    //   水中：moveRelative(0.02) → move → scale(0.8)
    //   岩浆：moveRelative(0.02) → move → scale(0.5)
    //   其余：moveRelative(0.02) → move → scale(0.91)
    constexpr f32 kFlyAcceleration = 0.02f;
    if (isInWater()) {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.8f);
    } else if (isInLava()) {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.5f);
    } else {
        moveRelative(kFlyAcceleration, strafing, vertical, forward);
        move(entity::MoverType::Self, velocity());
        scaleVelocity(0.91f);
    }
    updateTravelAnimation(false);
}

// ========== 摔落 ==========

void HappyGhastEntity::handleFallDamage(f32 /*distance*/, f32 /*damageMultiplier*/)
{
    // 对齐 vanilla HappyGhast.checkFallDamage：空实现——飞行生物免疫摔落伤害。
    // 不调用 LivingEntity::handleFallDamage，故既不结算摔伤也不播放摔落音效。
}

// ========== 声音 ==========

std::optional<ResourceLocation> HappyGhastEntity::getAmbientSound() const
{
    // vanilla 快乐恶魂使用独立的 HAPPY_GHAST_AMBIENT 音效；
    // TODO: 待 SoundEvents::HAPPY_GHAST_AMBIENT 常量补齐后改为返回该常量。
    return std::nullopt;
}

std::optional<ResourceLocation> HappyGhastEntity::getHurtSound(DamageSource& /*source*/) const
{
    // TODO: 待 SoundEvents::HAPPY_GHAST_HURT 常量补齐后改为返回该常量。
    return std::nullopt;
}

std::optional<ResourceLocation> HappyGhastEntity::getDeathSound() const
{
    // TODO: 待 SoundEvents::HAPPY_GHAST_DEATH 常量补齐后改为返回该常量。
    return std::nullopt;
}

// ========== AI 目标注册 ==========

void HappyGhastEntity::registerGoals()
{
    AnimalEntity::registerGoals();

    // 对齐 vanilla HappyGhast.registerGoals（registerGoals 内的目标优先级）：
    //   3: FloatGoal（幼体持续上浮）——vanilla 的 HappyGhastFloatGoal + FlyingMoveControl(180, true)
    //   5: Ghast.RandomFloatAroundGoal（16 格半径随机巡游）——此处复用成年随机飞行目标
    // 本项目无 Brain，vanilla HappyGhastAi 的其余行为（跟随拴绳持有者、载客等）尚未实现（见类注释 TODO）。
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::GhastHappyFloatGoal>(this));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::GhastRandomFlyGoal>(this));
}

// ========== 属性注册 ==========

void HappyGhastEntity::registerAttributes()
{
    AnimalEntity::registerAttributes();

    // 对齐 vanilla HappyGhast.createAttributes：
    //   Animal.createAnimalAttributes() + MAX_HEALTH=20 + TEMPT_RANGE=16 + FLYING_SPEED=0.05
    //   + MOVEMENT_SPEED=0.05 + FOLLOW_RANGE=16 + CAMERA_DISTANCE=8
    // （TEMPT_RANGE / CAMERA_DISTANCE 属性本项目未建模，故只设置已存在的四项。）
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    attributes().setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.05);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.05);
}

} // namespace mc