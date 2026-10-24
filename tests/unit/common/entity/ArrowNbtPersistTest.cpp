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

// 箭矢 NBT 持久化对齐 vanilla AbstractArrow.addAdditionalSaveData 测试。
//
// vanilla 1.21.11 AbstractArrow 持久化 life/inBlockState/shake/inGround/pickup/damage/
// crit/PierceLevel/SoundEvent/item/weapon 共 11 字段。此前项目缺 SoundEvent 与 weapon
// 两项（NBT 键常量里明确标注 TODO），导致：
//   - 射出方指定的命中音效（如三叉戟专属音效）在读档后丢失，回落到默认落地音效；
//   - firedFromWeapon 丢失，击退附魔加成与 killed_by_arrow 进度判定失效。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/TridentEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

using namespace mc;
using namespace mc::entity;

namespace {

class ArrowNbtTestWorld final : public mc::test::BaseTestWorld {
public:
    ArrowNbtTestWorld() = default;
};

/// 暴露 protected 的落地音效解析入口供测试调用。
template <typename T>
class ArrowSoundProbe final : public T {
public:
    using T::T;

    [[nodiscard]] ResourceLocation probeHitGroundSound() const
    {
        return this->resolveHitGroundSound();
    }
};

} // namespace

class ArrowNbtPersistTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    ArrowNbtTestWorld m_world;
};

// SoundEvent 与 weapon 经 writeToNBT / readFromNBT 往返保持。
TEST_F(ArrowNbtPersistTest, SoundEventAndWeaponRoundTrip)
{
    ArrowEntity arrow(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow.setWorld(&m_world);
    arrow.setSoundEventId(SoundEvents::ITEM_TRIDENT_HIT_GROUND.toString());
    arrow.setFiredFromWeapon(ItemStack(Items::BOW));

    nbt::tags::compound_tag tag;
    arrow.writeToNBT(tag);

    // 两个键必须真实写出（缺键时读出侧只能回落到默认值）。
    ASSERT_NE(serialization::nbt_helper::tryGetString(tag, serialization::nbt_keys::ARROW_SOUND_EVENT), std::nullopt);
    ASSERT_NE(serialization::nbt_helper::tryGetCompound(tag, serialization::nbt_keys::ARROW_WEAPON), nullptr);

    ArrowEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    restored.setWorld(&m_world);
    ASSERT_TRUE(restored.readFromNBT(tag).success());

    EXPECT_EQ(restored.soundEventId(), SoundEvents::ITEM_TRIDENT_HIT_GROUND.toString());
    EXPECT_EQ(restored.firedFromWeapon().getItem(), Items::BOW);
}

// 未设置音效时回落默认落地音效（普通箭 ENTITY_ARROW_HIT_GROUND，三叉戟 ITEM_TRIDENT_HIT_GROUND）。
TEST_F(ArrowNbtPersistTest, DefaultHitGroundSoundFollowsSubclass)
{
    ArrowSoundProbe<ArrowEntity> arrow(EntityInstanceId(3), mc::test::testEcsRegistry());
    arrow.setWorld(&m_world);
    EXPECT_TRUE(arrow.soundEventId().empty());
    EXPECT_EQ(arrow.probeHitGroundSound(), SoundEvents::ENTITY_ARROW_HIT_GROUND);

    ArrowSoundProbe<TridentEntity> trident(EntityInstanceId(4), mc::test::testEcsRegistry());
    trident.setWorld(&m_world);
    EXPECT_EQ(trident.probeHitGroundSound(), SoundEvents::ITEM_TRIDENT_HIT_GROUND);

    // 显式设置后覆盖默认值。
    trident.setSoundEventId(SoundEvents::ENTITY_ARROW_HIT.toString());
    EXPECT_EQ(trident.probeHitGroundSound(), SoundEvents::ENTITY_ARROW_HIT);
}