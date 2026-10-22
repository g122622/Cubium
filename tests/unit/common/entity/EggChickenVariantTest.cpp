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

// 投掷蛋按物品 chicken/variant 组件决定孵化变种（对齐 vanilla ThrownEgg.onHit:78-80）。
//
// vanilla 从 this.getItem().get(DataComponents.CHICKEN_VARIANT)（可空 Holder）读取并
// 覆盖小鸡变种；组件缺省时保留小鸡默认 TEMPERATE。项目此前物品侧未承载该组件值，
// 只能按蛋的种类映射，并在代码中留 TODO。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::entity;

namespace {

class EggVariantTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        registerEntityForLookup(m_spawned.back().get());
        return m_spawned.back()->id();
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawned; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
};

} // namespace

class EggChickenVariantTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EggVariantTestWorld m_world;
};

// 组件值 {"id":"minecraft:warm"}：孵化出温暖变种小鸡。
TEST_F(EggChickenVariantTest, HatchesVariantFromItemComponent)
{
    EggEntity egg(EntityInstanceId(1), mc::test::testEcsRegistry());
    egg.setWorld(&m_world);
    egg.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stack(Items::EGG);
    stack.setComponent(item::component::DataComponentType::ChickenVariant,
        item::component::DataComponentPayload{std::in_place_index<item::component::ComponentPayloadIndex::Json>,
            nlohmann::json{{"id", "minecraft:warm"}}});
    egg.setItemStack(stack);

    egg.hatchChickenForTest();

    ASSERT_EQ(m_world.spawnedEntities().size(), 1U);
    const auto* chicken = dynamic_cast<const ChickenEntity*>(m_world.spawnedEntities().front().get());
    ASSERT_NE(chicken, nullptr);
    EXPECT_EQ(chicken->getVariant(), ChickenVariant::Warm);
    EXPECT_TRUE(chicken->isChild());
}

// 组件缺省时保持默认 TEMPERATE（不按蛋种类猜测）。
TEST_F(EggChickenVariantTest, DefaultsToTemperateWithoutComponent)
{
    EggEntity egg(EntityInstanceId(2), mc::test::testEcsRegistry());
    egg.setWorld(&m_world);
    egg.setItemStack(ItemStack(Items::BROWN_EGG));

    egg.hatchChickenForTest();

    ASSERT_EQ(m_world.spawnedEntities().size(), 1U);
    const auto* chicken = dynamic_cast<const ChickenEntity*>(m_world.spawnedEntities().front().get());
    ASSERT_NE(chicken, nullptr);
    EXPECT_EQ(chicken->getVariant(), ChickenVariant::Temperate);
}
