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

/**
 * @file AllayEntityTest.cpp
 * @brief 悦灵音效测试
 *
 * 原版 `Allay` 的环境音按"是否手持物品"分流：
 *   - 手持物品 → `entity.allay.ambient_with_item`
 *   - 空手     → `entity.allay.ambient_without_item`
 * 受伤音固定为 `entity.allay.hurt`。
 *
 * 此前 `AllayEntity` 完全没有音效 override，会落到基类拼接的
 * `entity.allay.ambient`，该声音键在 sounds.json 中不存在。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/allay/AllayEntity.hpp"
#include "common/item/Items.hpp"
#include "common/sound/SoundEvents.hpp"

#include <optional>

using namespace mc;

namespace {

/// BaseTestWorld 构造函数为 protected，派生公开以供 fixture 持有。
class AllayTestWorld final : public mc::test::BaseTestWorld {
public:
    AllayTestWorld() = default;
};

class AllayEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
    }

    AllayTestWorld m_world;
};

TEST_F(AllayEntityTest, AmbientSoundIsWithoutItemWhenHandsEmpty)
{
    entity::AllayEntity allay(EntityInstanceId(1), mc::test::testEcsRegistry());
    allay.setWorld(&m_world);

    ASSERT_TRUE(allay.getEquipment(EquipmentSlot::MainHand).isEmpty());
    EXPECT_EQ(allay.getAmbientSound(), std::optional<ResourceLocation>(SoundEvents::ENTITY_ALLAY_AMBIENT_WITHOUT_ITEM));
}

TEST_F(AllayEntityTest, AmbientSoundIsWithItemWhenHoldingItem)
{
    entity::AllayEntity allay(EntityInstanceId(2), mc::test::testEcsRegistry());
    allay.setWorld(&m_world);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    allay.setEquipment(EquipmentSlot::MainHand, ItemStack(*stick, 1));

    EXPECT_EQ(allay.getAmbientSound(), std::optional<ResourceLocation>(SoundEvents::ENTITY_ALLAY_AMBIENT_WITH_ITEM));
}

TEST_F(AllayEntityTest, HurtSoundUsesDedicatedConstant)
{
    entity::AllayEntity allay(EntityInstanceId(3), mc::test::testEcsRegistry());
    allay.setWorld(&m_world);

    auto source = DamageSources::generic();
    EXPECT_EQ(allay.getHurtSound(source), std::optional<ResourceLocation>(SoundEvents::ENTITY_ALLAY_HURT));
}

} // namespace