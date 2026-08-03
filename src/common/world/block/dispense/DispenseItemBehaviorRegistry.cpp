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
 * IMPLIED, INCLUDING ANY WARRANTY OF ANY KIND, EXPRESS OR
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DispenseItemBehaviorRegistry.hpp"

#include "IDispenseItemBehavior.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace blocks {

DispenseItemBehaviorRegistry::DispenseItemBehaviorRegistry()
    : m_defaultBehavior(std::make_unique<DefaultDispenseItemBehavior>())
{}

DispenseItemBehaviorRegistry& DispenseItemBehaviorRegistry::instance()
{
    static DispenseItemBehaviorRegistry instance;
    return instance;
}

void DispenseItemBehaviorRegistry::registerBehavior(
    const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior)
{
    m_behaviors[itemId] = std::move(behavior);
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return nullptr;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return getBehavior(item->itemLocation().toString());
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const std::string& itemId) const
{
    auto it = m_behaviors.find(itemId);
    if (it != m_behaviors.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DispenseItemBehaviorRegistry::hasBehavior(const std::string& itemId) const
{
    return m_behaviors.find(itemId) != m_behaviors.end();
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getDefaultBehavior()
{
    return m_defaultBehavior.get();
}

void DispenseItemBehaviorRegistry::initDefaultBehaviors()
{
    // ========================================================================
    // 投掷物发射行为
    //
    // 通过 ProjectileItem 接口统一注册，每种实现了 ProjectileItem 接口的
    // 物品自动获取正确的发射行为，无需手动编写 lambda 工厂函数。
    // 参考 MC Java 的 DispenserBlock.registerProjectileBehavior()
    // ========================================================================

    // --- 箭矢 ---
    registerProjectileBehavior(*Items::ARROW);
    registerProjectileBehavior(*Items::SPECTRAL_ARROW);
    registerProjectileBehavior(*Items::TIPPED_ARROW);

    // --- 投掷物品 ---
    registerProjectileBehavior(*Items::SNOWBALL);
    registerProjectileBehavior(*Items::EGG);
    registerProjectileBehavior(*Items::BLUE_EGG);
    registerProjectileBehavior(*Items::BROWN_EGG);
    registerProjectileBehavior(*Items::ENDER_PEARL);
    registerProjectileBehavior(*Items::EXPERIENCE_BOTTLE);
    registerProjectileBehavior(*Items::SPLASH_POTION);
    registerProjectileBehavior(*Items::LINGERING_POTION);

    // --- 火焰弹 ---
    registerProjectileBehavior(*Items::FIRE_CHARGE);

    // --- 风弹 ---
    registerProjectileBehavior(*Items::WIND_CHARGE);

    // --- 烟花火箭 ---
    registerProjectileBehavior(*Items::FIREWORK_ROCKET);

    // ========================================================================
    // TNT 发射行为
    // 生成点燃的 TNT 实体；如果 tntExplodes 游戏规则为 false 则不发射
    // ========================================================================
    registerBehavior("minecraft:tnt", std::make_unique<TNTDispenseBehavior>());

    // ========================================================================
    // 船发射行为
    // 需要检测目标位置是否有水
    // ========================================================================

    // 橡木船
    registerBehavior("minecraft:oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::OAK));
    // 云杉木船
    registerBehavior("minecraft:spruce_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::SPRUCE));
    // 白桦木船
    registerBehavior("minecraft:birch_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::BIRCH));
    // 丛林木船
    registerBehavior("minecraft:jungle_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::JUNGLE));
    // 金合欢木船
    registerBehavior("minecraft:acacia_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::ACACIA));
    // 深色橡木船
    registerBehavior(
        "minecraft:dark_oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::DARK_OAK));
    // 红树木船
    registerBehavior(
        "minecraft:mangrove_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::MANGROVE));
    // 樱花木船
    registerBehavior("minecraft:cherry_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::CHERRY));
    // 苍白橡木船
    registerBehavior(
        "minecraft:pale_oak_boat", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::PALE_OAK));
    // 竹筏
    registerBehavior("minecraft:bamboo_raft", std::make_unique<BoatDispenseBehavior>(entity::BoatEntity::Type::BAMBOO));

    // ========================================================================
    // 水桶/岩浆桶发射行为
    // ========================================================================
    // 获取流体实例
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);

    if (waterFluid != nullptr) {
        registerBehavior("minecraft:water_bucket", std::make_unique<BucketDispenseBehavior>(*waterFluid));
    }
    if (lavaFluid != nullptr) {
        registerBehavior("minecraft:lava_bucket", std::make_unique<BucketDispenseBehavior>(*lavaFluid));
    }

    // ========================================================================
    // 空桶发射行为（收集流体或非流体内容物）
    // ========================================================================
    registerBehavior("minecraft:bucket", std::make_unique<EmptyBucketDispenseBehavior>());

    // ========================================================================
    // 细雪桶发射行为（放置细雪方块）
    // ========================================================================
    registerBehavior("minecraft:powder_snow_bucket", std::make_unique<PowderSnowBucketDispenseBehavior>());

    // ========================================================================
    // 打火石发射行为
    // ========================================================================
    registerBehavior("minecraft:flint_and_steel", std::make_unique<FlintAndSteelDispenseBehavior>());

    // ========================================================================
    // 骨粉发射行为
    // ========================================================================
    registerBehavior("minecraft:bone_meal", std::make_unique<BonemealDispenseBehavior>());
}

void DispenseItemBehaviorRegistry::registerProjectileBehavior(const Item& item)
{
    const auto* projectileItem = dynamic_cast<const item::ProjectileItem*>(&item);
    if (projectileItem == nullptr) {
        return;
    }

    auto behavior = std::make_unique<ProjectileDispenseBehavior>(*projectileItem);
    registerBehavior(item.itemLocation().toString(), std::move(behavior));
}

} // namespace blocks
} // namespace mc
