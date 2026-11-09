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

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"

#include <memory>
#include <string>

using namespace mc;

namespace {

/// @brief 头颅方块实体的坐标，仅用于构造，不涉及世界交互。
const BlockPos kSkullPos(4, 2, 4);

} // namespace

// ============================================================================
// SkullBlockEntity 的 note_block_sound
// ============================================================================

// 设置后应能读回同一个声音事件。
TEST(SkullBlockEntityNoteBlockSoundTest, SetThenGetReturnsSameValue)
{
    blockentity::SkullBlockEntity skull(kSkullPos);
    EXPECT_FALSE(skull.getNoteBlockSound().has_value()) << "default must be unset";

    skull.setNoteBlockSound("minecraft:block.note_block.bell");

    ASSERT_TRUE(skull.getNoteBlockSound().has_value());
    EXPECT_EQ(skull.getNoteBlockSound().value(), "minecraft:block.note_block.bell");
}

// 传空串等价于清除：消费方只需判 optional，不用再判空串。
TEST(SkullBlockEntityNoteBlockSoundTest, EmptyStringClearsValue)
{
    blockentity::SkullBlockEntity skull(kSkullPos);
    skull.setNoteBlockSound("minecraft:block.note_block.bell");
    ASSERT_TRUE(skull.getNoteBlockSound().has_value());

    skull.setNoteBlockSound(std::string{});

    EXPECT_FALSE(skull.getNoteBlockSound().has_value())
        << "empty string must reset to nullopt, not store an empty string";
}

// clone 必须带上 note_block_sound，否则结构复制会丢掉音效。
TEST(SkullBlockEntityNoteBlockSoundTest, CloneCopiesNoteBlockSound)
{
    blockentity::SkullBlockEntity skull(kSkullPos);
    skull.setNoteBlockSound("minecraft:block.note_block.banjo");

    const std::unique_ptr<BlockEntity> copy = skull.clone();
    ASSERT_NE(copy, nullptr);

    auto* copiedSkull = dynamic_cast<blockentity::SkullBlockEntity*>(copy.get());
    ASSERT_NE(copiedSkull, nullptr);
    ASSERT_TRUE(copiedSkull->getNoteBlockSound().has_value());
    EXPECT_EQ(copiedSkull->getNoteBlockSound().value(), "minecraft:block.note_block.banjo");
}

// NBT 往返：存档/结构模板读取后音效不能丢。
TEST(SkullBlockEntityNoteBlockSoundTest, NbtRoundTripPreservesSound)
{
    blockentity::SkullBlockEntity original(kSkullPos);
    original.setNoteBlockSound("minecraft:block.note_block.chime");

    nbt::CompoundTag tag;
    original.saveToNBT(tag);

    blockentity::SkullBlockEntity restored(kSkullPos);
    ASSERT_TRUE(restored.loadFromNBT(tag));

    ASSERT_TRUE(restored.getNoteBlockSound().has_value()) << "NBT round trip lost note_block_sound";
    EXPECT_EQ(restored.getNoteBlockSound().value(), "minecraft:block.note_block.chime");
}

// 没有设置音效时，NBT 里不应出现该键（避免存档凭空多一个空字段）。
TEST(SkullBlockEntityNoteBlockSoundTest, NbtOmitsKeyWhenUnset)
{
    blockentity::SkullBlockEntity skull(kSkullPos);

    nbt::CompoundTag tag;
    skull.saveToNBT(tag);

    EXPECT_EQ(tag.value.find("note_block_sound"), tag.value.end());
}

// ============================================================================
// ItemStack 的 note_block_sound 组件
// ============================================================================

// 组件读写往返。
TEST(ItemStackNoteBlockSoundTest, SetThenGetReturnsSameValue)
{
    Items::ensureInitialized();
    ASSERT_NE(Items::PLAYER_HEAD, nullptr);

    ItemStack stack(Items::PLAYER_HEAD, 1);
    EXPECT_EQ(stack.getNoteBlockSound(), nullptr) << "component must be unset by default";
    EXPECT_FALSE(stack.hasComponent(item::component::DataComponentType::NoteBlockSound));

    stack.setNoteBlockSound("minecraft:block.note_block.bell");

    ASSERT_NE(stack.getNoteBlockSound(), nullptr);
    EXPECT_EQ(*stack.getNoteBlockSound(), "minecraft:block.note_block.bell");
    EXPECT_TRUE(stack.hasComponent(item::component::DataComponentType::NoteBlockSound));
}

// 空串清除组件，hasComponent 也随之变假。
TEST(ItemStackNoteBlockSoundTest, EmptyStringClearsComponent)
{
    Items::ensureInitialized();
    ASSERT_NE(Items::PLAYER_HEAD, nullptr);

    ItemStack stack(Items::PLAYER_HEAD, 1);
    stack.setNoteBlockSound("minecraft:block.note_block.bell");
    ASSERT_TRUE(stack.hasComponent(item::component::DataComponentType::NoteBlockSound));

    stack.setNoteBlockSound(std::string{});

    EXPECT_EQ(stack.getNoteBlockSound(), nullptr);
    EXPECT_FALSE(stack.hasComponent(item::component::DataComponentType::NoteBlockSound));
}

// 复制 ItemStack 时组件必须一并复制（ItemExtras 的深拷贝路径）。
TEST(ItemStackNoteBlockSoundTest, CopyPreservesComponent)
{
    Items::ensureInitialized();
    ASSERT_NE(Items::PLAYER_HEAD, nullptr);

    ItemStack original(Items::PLAYER_HEAD, 1);
    original.setNoteBlockSound("minecraft:block.note_block.pling");

    const ItemStack copy = original;
    ASSERT_NE(copy.getNoteBlockSound(), nullptr);
    EXPECT_EQ(*copy.getNoteBlockSound(), "minecraft:block.note_block.pling");
}
