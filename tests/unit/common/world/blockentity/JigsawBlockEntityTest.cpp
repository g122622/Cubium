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

// 拼图方块实体测试（对齐 vanilla JigsawBlockEntity）。
//
// 覆盖：
//   - 默认值（name/target = minecraft:empty、final_state = minecraft:air、优先级 0）
//   - NBT 往返（saveToNBT → loadFromNBT，含 joint / 优先级）
//   - JSON 字段名（对齐 vanilla：name/target/pool/final_state/joint/placement_priority/selection_priority）
//   - clone 与实体注册
//
// 注：joint 默认值依赖方块前方朝向，无世界上下文时 frontFacing 回退 North（水平 → ALIGNED），
//     故空实体加载无 joint 字段的 NBT 时得到 ALIGNED。
//
// Ref: net.minecraft.world.level.block.entity.JigsawBlockEntity

#include "common/world/blockentity/special/JigsawBlockEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;
using world::gen::jigsaw::JigsawJointType;

class JigsawBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_entity = std::make_unique<JigsawBlockEntity>(BlockPos(1, 2, 3));
    }

    std::unique_ptr<JigsawBlockEntity> m_entity;
};

TEST_F(JigsawBlockEntityTest, Create_DefaultValuesMatchVanilla)
{
    EXPECT_EQ(m_entity->getType(), BlockEntityType::JigsawBlock);
    EXPECT_EQ(m_entity->getName(), "minecraft:empty");
    EXPECT_EQ(m_entity->getTarget(), "minecraft:empty");
    EXPECT_EQ(m_entity->getPool(), "minecraft:empty");
    EXPECT_EQ(m_entity->getFinalState(), "minecraft:air");
    EXPECT_EQ(m_entity->getJoint(), JigsawJointType::Rollable);
    EXPECT_EQ(m_entity->getPlacementPriority(), 0);
    EXPECT_EQ(m_entity->getSelectionPriority(), 0);
}

TEST_F(JigsawBlockEntityTest, NbtRoundTrip_PreservesAllFields)
{
    m_entity->setName("minecraft:village/houses/plains_small_house_1");
    m_entity->setTarget("minecraft:village/plains/streets");
    m_entity->setPool("minecraft:village/plains/houses");
    m_entity->setFinalState("minecraft:oak_planks");
    m_entity->setJoint(JigsawJointType::Aligned);
    m_entity->setPlacementPriority(3);
    m_entity->setSelectionPriority(5);

    const nbt::CompoundTag tag = m_entity->getUpdateTag();
    JigsawBlockEntity loaded(BlockPos(1, 2, 3));
    ASSERT_TRUE(loaded.loadFromNBT(tag));

    EXPECT_EQ(loaded.getName(), "minecraft:village/houses/plains_small_house_1");
    EXPECT_EQ(loaded.getTarget(), "minecraft:village/plains/streets");
    EXPECT_EQ(loaded.getPool(), "minecraft:village/plains/houses");
    EXPECT_EQ(loaded.getFinalState(), "minecraft:oak_planks");
    EXPECT_EQ(loaded.getJoint(), JigsawJointType::Aligned);
    EXPECT_EQ(loaded.getPlacementPriority(), 3);
    EXPECT_EQ(loaded.getSelectionPriority(), 5);
}

TEST_F(JigsawBlockEntityTest, Load_MissingJointFallsBackToDefaultJointType)
{
    nbt::CompoundTag tag;
    ASSERT_TRUE(m_entity->loadFromNBT(tag));
    // 无方块世界上下文：前方朝向回退 North（水平方向），默认 ALIGNED。
    EXPECT_EQ(m_entity->getJoint(), JigsawJointType::Aligned);
}

TEST_F(JigsawBlockEntityTest, Save_WritesVanillaFieldNames)
{
    m_entity->setName("minecraft:a");
    nlohmann::json data;
    m_entity->save(data);

    EXPECT_EQ(data["name"], "minecraft:a");
    EXPECT_EQ(data["target"], "minecraft:empty");
    EXPECT_EQ(data["joint"], "rollable");
    EXPECT_EQ(data["placement_priority"], 0);
    EXPECT_EQ(data["selection_priority"], 0);
    EXPECT_EQ(data["final_state"], "minecraft:air");
}

TEST_F(JigsawBlockEntityTest, Clone_CopiesEditableState)
{
    m_entity->setJoint(JigsawJointType::Aligned);
    m_entity->setSelectionPriority(9);

    auto copy = m_entity->clone();
    ASSERT_NE(copy, nullptr);
    auto* jigsaw = dynamic_cast<JigsawBlockEntity*>(copy.get());
    ASSERT_NE(jigsaw, nullptr);
    EXPECT_EQ(jigsaw->getJoint(), JigsawJointType::Aligned);
    EXPECT_EQ(jigsaw->getSelectionPriority(), 9);
}

TEST_F(JigsawBlockEntityTest, Registry_CreatesJigsawBlockEntity)
{
    auto& registry = BlockEntityRegistry::instance();
    registry.registerBuiltinTypes();
    ASSERT_TRUE(registry.hasType(BlockEntityType::JigsawBlock));

    auto entity = registry.create(BlockEntityType::JigsawBlock, BlockPos(4, 5, 6));
    ASSERT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<JigsawBlockEntity*>(entity.get()), nullptr);
}
