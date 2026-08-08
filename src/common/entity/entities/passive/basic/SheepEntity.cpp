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

#include "SheepEntity.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp" // 包含 LookRandomlyGoal
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/EatGrassGoal.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace mc {

std::unique_ptr<Entity> SheepEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<SheepEntity>(0, registry);
}

SheepEntity::SheepEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
}

std::optional<ResourceLocation> SheepEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> SheepEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> SheepEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

bool SheepEntity::isShearable() const
{
    // 只有活着、有羊毛、非幼羊的羊才能被剪
    return isAlive() && !m_sheared && !isChild();
}

std::vector<ItemStack> SheepEntity::shear(Player* /*player*/)
{
    std::vector<ItemStack> drops;

    if (!isShearable()) {
        return drops;
    }

    // 设置剪毛状态
    m_sheared = true;

    // 播放剪毛音效
    playSound(*makeSoundEventId("shear"), 1.0f, 1.0f);

    // 掉落 1-3 个对应颜色的羊毛
    math::Random rng(ticksExisted());
    i32 woolCount = 1 + rng.nextInt(3);

    // 获取对应颜色的羊毛物品
    const Block* woolBlock = getWoolBlockByColor(m_fleeceColor);
    if (woolBlock != nullptr) {
        const BlockItem* woolItem = BlockItemRegistry::instance().getBlockItem(*woolBlock);
        if (woolItem != nullptr) {
            for (i32 i = 0; i < woolCount; ++i) {
                drops.emplace_back(static_cast<const Item*>(woolItem), 1);
            }
        }
    }

    return drops;
}

bool SheepEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 羊用小麦繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT;
}

bool SheepEntity::canMateWith(const AnimalEntity& other) const
{
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SheepEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小羊
    auto baby = std::make_unique<SheepEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 颜色继承逻辑
    SheepEntity* partnerSheep = dynamic_cast<SheepEntity*>(&partner);
    if (partnerSheep != nullptr) {
        // 使用颜色混合逻辑
        math::Random& rng = getRandom();
        DyeColor mixedColor = getDyeColorMixFromParents(getFleeceColor(), partnerSheep->getFleeceColor(), rng);
        baby->setFleeceColor(mixedColor);
    } else {
        baby->setFleeceColor(getFleeceColor());
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

void SheepEntity::eatGrassBonus()
{
    // 吃草奖励
    // 如果被剪过，重新长出羊毛
    if (m_sheared) {
        m_sheared = false;
    }

    // 如果是幼羊，加速成长
    if (isChild()) {
        addGrowingAge(60); // 加速成长 60 ticks (3秒)
    }
}

DyeColor SheepEntity::getRandomSheepColor(math::Random& random)
{
    i32 i = random.nextInt(100);

    if (i < 5) {
        return DyeColor::Black; // 5% 黑色
    } else if (i < 10) {
        return DyeColor::Gray; // 5% 灰色
    } else if (i < 15) {
        return DyeColor::LightGray; // 5% 浅灰色
    } else if (i < 18) {
        return DyeColor::Brown; // 3% 棕色
    } else {
        // 82% 白色，其中 0.2% 粉色
        if (random.nextInt(500) == 0) {
            return DyeColor::Pink; // ~0.2% 粉色
        }
        return DyeColor::White; // ~81.8% 白色
    }
}

const Block* SheepEntity::getWoolBlockByColor(DyeColor color)
{
    // 根据颜色返回对应的羊毛方块
    switch (color) {
        case DyeColor::White:
            return VanillaBlocks::WHITE_WOOL;
        case DyeColor::Orange:
            return VanillaBlocks::ORANGE_WOOL;
        case DyeColor::Magenta:
            return VanillaBlocks::MAGENTA_WOOL;
        case DyeColor::LightBlue:
            return VanillaBlocks::LIGHT_BLUE_WOOL;
        case DyeColor::Yellow:
            return VanillaBlocks::YELLOW_WOOL;
        case DyeColor::Lime:
            return VanillaBlocks::LIME_WOOL;
        case DyeColor::Pink:
            return VanillaBlocks::PINK_WOOL;
        case DyeColor::Gray:
            return VanillaBlocks::GRAY_WOOL;
        case DyeColor::LightGray:
            return VanillaBlocks::LIGHT_GRAY_WOOL;
        case DyeColor::Cyan:
            return VanillaBlocks::CYAN_WOOL;
        case DyeColor::Purple:
            return VanillaBlocks::PURPLE_WOOL;
        case DyeColor::Blue:
            return VanillaBlocks::BLUE_WOOL;
        case DyeColor::Brown:
            return VanillaBlocks::BROWN_WOOL;
        case DyeColor::Green:
            return VanillaBlocks::GREEN_WOOL;
        case DyeColor::Red:
            return VanillaBlocks::RED_WOOL;
        case DyeColor::Black:
            return VanillaBlocks::BLACK_WOOL;
        default:
            return VanillaBlocks::WHITE_WOOL;
    }
}

void SheepEntity::registerGoals()
{
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.25));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 小麦诱惑
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.1,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr && item == Items::WHEAT;
            },
            false)); // scaredByMovement = false

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 5: 吃草 - 在 RandomWalkingGoal 之前
    auto eatGrassGoal = std::make_unique<entity::ai::goal::EatGrassGoal>(
        this, [this]() { this->eatGrassBonus(); }, [this]() { return this->isChild(); });
    m_eatGrassGoal = eatGrassGoal.get();
    m_goalSelector.addGoal(5, std::move(eatGrassGoal));

    // 优先级 6: 随机漫步
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void SheepEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 羊的属性设置
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
}

void SheepEntity::tick()
{
    // 同步吃草动画计时器（服务端从 EatGrassGoal 读取）
    if (m_eatGrassGoal != nullptr) {
        m_eatAnimationTimer = m_eatGrassGoal->getEatingGrassTimer();
    }

    AnimalEntity::tick();
}

// ============================================================================
// 颜色混合逻辑
// ============================================================================

namespace {

/**
 * @brief 染料颜色混合表
 *
 * 颜色混合基于合成配方：
 * - 白色 + 红色 = 粉红色
 * - 红色 + 黄色 = 橙色
 * - 白色 + 黑色 = 灰色
 * - 灰色 + 白色 = 淡灰色
 * - 白色 + 蓝色 = 淡蓝色
 * - 蓝色 + 红色 = 紫色
 * - 蓝色 + 绿色 = 青色
 * - 白色 + 绿色 = 黄绿色
 * 等等
 */
const std::vector<std::tuple<DyeColor, DyeColor, DyeColor>> COLOR_MIXING_TABLE = {
    // 白色混合
    {DyeColor::White, DyeColor::Red, DyeColor::Pink},
    {DyeColor::White, DyeColor::Blue, DyeColor::LightBlue},
    {DyeColor::White, DyeColor::Green, DyeColor::Lime},
    {DyeColor::White, DyeColor::Black, DyeColor::Gray},
    {DyeColor::White, DyeColor::Gray, DyeColor::LightGray},

    // 红色混合
    {DyeColor::Red, DyeColor::Yellow, DyeColor::Orange},
    {DyeColor::Red, DyeColor::Blue, DyeColor::Purple},

    // 蓝色混合
    {DyeColor::Blue, DyeColor::Green, DyeColor::Cyan},
    {DyeColor::Blue, DyeColor::White, DyeColor::LightBlue},
    {DyeColor::Blue, DyeColor::Red, DyeColor::Purple},

    // 黄色混合
    {DyeColor::Yellow, DyeColor::Red, DyeColor::Orange},

    // 绿色混合
    {DyeColor::Green, DyeColor::Blue, DyeColor::Cyan},
    {DyeColor::Green, DyeColor::White, DyeColor::Lime},

    // 黑色混合
    {DyeColor::Black, DyeColor::White, DyeColor::Gray},

    // 灰色混合
    {DyeColor::Gray, DyeColor::White, DyeColor::LightGray},
};

/**
 * @brief 查找颜色混合结果
 * @param c1 颜色1
 * @param c2 颜色2
 * @return 混合后的颜色，如果没有配方返回无效值
 */
std::optional<DyeColor> findMixingResult(DyeColor c1, DyeColor c2)
{
    for (const auto& [a, b, result] : COLOR_MIXING_TABLE) {
        if ((a == c1 && b == c2) || (a == c2 && b == c1)) {
            return result;
        }
    }
    return std::nullopt;
}

} // anonymous namespace

DyeColor SheepEntity::getDyeColorMixFromParents(DyeColor parent1Color, DyeColor parent2Color, math::Random& random)
{
    // 如果两个颜色相同，直接返回该颜色
    if (parent1Color == parent2Color) {
        return parent1Color;
    }

    // 查找混合结果
    auto mixedColor = findMixingResult(parent1Color, parent2Color);
    if (mixedColor.has_value()) {
        return mixedColor.value();
    }

    // 如果没有配方，随机选择父母颜色
    return random.nextBoolean() ? parent1Color : parent2Color;
}

} // namespace mc
