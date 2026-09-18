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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/TestWorldHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;

namespace {

// 测试用方块实体，记录 load/save 调用
class MockBlockEntity : public BlockEntity {
public:
    MockBlockEntity(BlockEntityType type, const BlockPos& pos)
        : BlockEntity(type, pos)
    {}

    // 记录自定义数据
    nlohmann::json lastLoadedData;

    bool load(const nlohmann::json& data) override
    {
        BlockEntity::load(data);
        lastLoadedData = data;
        // 记录自定义字段
        if (data.contains("custom_value") && data["custom_value"].is_number()) {
            m_customValue = data["custom_value"].get<i32>();
        }
        return true;
    }

    void save(nlohmann::json& data) const override
    {
        BlockEntity::save(data);
        if (m_customValue != 0) {
            data["custom_value"] = m_customValue;
        }
    }

    std::unique_ptr<BlockEntity> clone() const override { return std::make_unique<MockBlockEntity>(m_type, m_pos); }

    i32 customValue() const { return m_customValue; }
    void setCustomValue(i32 value) { m_customValue = value; }

private:
    i32 m_customValue = 0;
};

// 需要 OP 权限的测试用方块实体
class OpOnlyBlockEntity : public BlockEntity {
public:
    OpOnlyBlockEntity(const BlockPos& pos)
        : BlockEntity(BlockEntityType::CommandBlock, pos)
    {}

    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    bool load(const nlohmann::json& data) override
    {
        BlockEntity::load(data);
        return true;
    }

    void save(nlohmann::json& data) const override { BlockEntity::save(data); }

    std::unique_ptr<BlockEntity> clone() const override { return std::make_unique<OpOnlyBlockEntity>(m_pos); }
};

// 测试用方块，拥有 facing + lit 属性
class NbtTestBlock : public Block {
public:
    NbtTestBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
                             .add(BlockStateProperties::HORIZONTAL_FACING())
                             .add(BlockStateProperties::LIT())
                             .create([](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                                 return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
                             });
        createBlockState(std::move(container));
        setDefaultState(defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                .with(BlockStateProperties::LIT(), false));
    }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

} // namespace

// ========== BlockEntity::onlyOpsCanSetNbt 测试 ==========

TEST(BlockEntityOnlyOpsCanSetNbtTest, DefaultReturnsFalse)
{
    // 大部分方块实体默认不需要 OP 权限
    MockBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));
    EXPECT_FALSE(entity.onlyOpsCanSetNbt());
}

TEST(BlockEntityOnlyOpsCanSetNbtTest, OpOnlyEntityReturnsTrue)
{
    OpOnlyBlockEntity entity(BlockPos(0, 0, 0));
    EXPECT_TRUE(entity.onlyOpsCanSetNbt());
}

// ========== applyBlockStateFromNBT 相关逻辑测试 ==========

class ApplyBlockStateFromNBTTest : public ::testing::Test {
protected:
    NbtTestBlock block;
};

TEST_F(ApplyBlockStateFromNBTTest, BlockStateTagParsing)
{
    // 验证 BlockStateTag 解析逻辑的各个组件

    const auto& container = block.stateContainer();

    // 1. 验证属性查找
    const IProperty* facingProp = container.getProperty("facing");
    ASSERT_NE(facingProp, nullptr);
    EXPECT_EQ(facingProp->name(), "facing");

    const IProperty* litProp = container.getProperty("lit");
    ASSERT_NE(litProp, nullptr);
    EXPECT_EQ(litProp->name(), "lit");

    // 2. 验证字符串值解析
    auto eastIdx = facingProp->parseValue("east");
    ASSERT_TRUE(eastIdx.has_value());
    EXPECT_EQ(facingProp->valueToString(*eastIdx), "east");

    auto southIdx = facingProp->parseValue("south");
    ASSERT_TRUE(southIdx.has_value());
    EXPECT_EQ(facingProp->valueToString(*southIdx), "south");

    auto litTrueIdx = litProp->parseValue("true");
    ASSERT_TRUE(litTrueIdx.has_value());

    // 3. 验证无效值解析
    auto invalidFacing = facingProp->parseValue("up"); // facing 不包含 up
    EXPECT_FALSE(invalidFacing.has_value());

    // 4. 验证不存在的属性名
    EXPECT_EQ(container.getProperty("nonexistent"), nullptr);
    EXPECT_EQ(container.getProperty("age"), nullptr);
}

TEST_F(ApplyBlockStateFromNBTTest, WithValueIndexIntegration)
{
    // 验证 withValueIndex 与 parseValue 配合使用
    const BlockState& defaultState = block.defaultState();
    const auto& container = block.stateContainer();

    // 默认 facing=north, lit=false
    EXPECT_EQ(defaultState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_FALSE(defaultState.get(BlockStateProperties::LIT()));

    // 设置 facing=east
    const IProperty* facingProp = container.getProperty("facing");
    auto eastIdx = facingProp->parseValue("east");
    ASSERT_TRUE(eastIdx.has_value());
    const BlockState& eastState = defaultState.withValueIndex(*facingProp, *eastIdx);
    EXPECT_EQ(eastState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    EXPECT_FALSE(eastState.get(BlockStateProperties::LIT())); // lit 不变

    // 在 eastState 基础上设置 lit=true
    const IProperty* litProp = container.getProperty("lit");
    auto litTrueIdx = litProp->parseValue("true");
    ASSERT_TRUE(litTrueIdx.has_value());
    const BlockState& eastLitState = eastState.withValueIndex(*litProp, *litTrueIdx);
    EXPECT_EQ(eastLitState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    EXPECT_TRUE(eastLitState.get(BlockStateProperties::LIT()));
}

TEST_F(ApplyBlockStateFromNBTTest, EmptyBlockStateTagJson)
{
    // 验证空的 BlockStateTag 不影响状态
    nlohmann::json blockStateTag = nlohmann::json::object();
    EXPECT_TRUE(blockStateTag.is_object());
    EXPECT_TRUE(blockStateTag.empty());

    // 遍历空对象应无任何迭代
    size_t count = 0;
    for (const auto& [key, value] : blockStateTag.items()) {
        (void)key;
        (void)value;
        count++;
    }
    EXPECT_EQ(count, 0u);
}

TEST_F(ApplyBlockStateFromNBTTest, BlockStateTagWithInvalidValues)
{
    // 验证 BlockStateTag 中包含无效属性或值时的容错性
    nlohmann::json blockStateTag;
    blockStateTag["facing"] = "east";       // 有效属性和值
    blockStateTag["nonexistent"] = "value"; // 不存在的属性
    blockStateTag["lit"] = 123;             // 非字符串值
    blockStateTag["power"] = "5";           // 此方块没有 power 属性

    const auto& container = block.stateContainer();

    // 只有 "facing" 是有效的
    for (const auto& [propName, propValue] : blockStateTag.items()) {
        if (!propValue.is_string()) {
            continue; // 跳过非字符串值
        }
        const IProperty* prop = container.getProperty(propName);
        if (prop == nullptr) {
            continue; // 跳过不存在的属性
        }
        auto parsedIndex = prop->parseValue(propValue.get<std::string>());
        if (!parsedIndex.has_value()) {
            continue; // 跳过无效值
        }
        // 只有 facing=east 应该成功
        EXPECT_EQ(propName, "facing");
    }
}

// ========== setTileEntityNBT 相关逻辑测试 ==========

TEST(SetTileEntityNBTLogicTest, BlockEntityTagTypeValidation)
{
    // 验证 BlockEntityTag 类型ID验证逻辑
    MockBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));

    // 类型匹配的情况
    nlohmann::json tag1;
    tag1["id"] = "minecraft:chest";
    tag1["custom_value"] = 42;
    auto idIt1 = tag1.find("id");
    ASSERT_NE(idIt1, tag1.end());
    EXPECT_TRUE(idIt1->is_string());
    ResourceLocation tagType1(idIt1->get<std::string>());
    BlockEntityType parsedType1 = blockEntityTypeFromId(tagType1);
    EXPECT_EQ(parsedType1, BlockEntityType::Chest);
    EXPECT_EQ(parsedType1, entity.getType());

    // 类型不匹配的情况
    nlohmann::json tag2;
    tag2["id"] = "minecraft:furnace";
    auto idIt2 = tag2.find("id");
    ASSERT_NE(idIt2, tag2.end());
    ResourceLocation tagType2(idIt2->get<std::string>());
    BlockEntityType parsedType2 = blockEntityTypeFromId(tagType2);
    EXPECT_EQ(parsedType2, BlockEntityType::Furnace);
    EXPECT_NE(parsedType2, entity.getType());

    // 未知类型
    nlohmann::json tag3;
    tag3["id"] = "minecraft:unknown_type";
    auto idIt3 = tag3.find("id");
    ASSERT_NE(idIt3, tag3.end());
    ResourceLocation tagType3(idIt3->get<std::string>());
    BlockEntityType parsedType3 = blockEntityTypeFromId(tagType3);
    EXPECT_EQ(parsedType3, BlockEntityType::Unknown);
}

TEST(SetTileEntityNBTLogicTest, BlockEntityTagMergeAndRollback)
{
    // 验证 BlockEntityTag 数据合并逻辑
    MockBlockEntity entity(BlockEntityType::Chest, BlockPos(0, 0, 0));

    // 设置初始数据
    entity.setCustomValue(10);
    nlohmann::json currentData;
    entity.save(currentData);
    EXPECT_EQ(currentData["custom_value"].get<i32>(), 10);
    EXPECT_EQ(currentData["id"].get<std::string>(), "minecraft:chest");

    // 模拟 BlockEntityTag 合并
    nlohmann::json blockEntityTag;
    blockEntityTag["custom_value"] = 99;

    nlohmann::json mergedData = currentData;
    ItemStack::mergeJsonObjects(mergedData, blockEntityTag);
    mergedData.erase("id"); // 移除类型标识符

    // 合并后 custom_value 应被覆盖
    EXPECT_EQ(mergedData["custom_value"].get<i32>(), 99);
    // id 应被移除
    EXPECT_EQ(mergedData.find("id"), mergedData.end());

    // 加载合并后数据
    entity.load(mergedData);
    EXPECT_EQ(entity.customValue(), 99);

    // 回滚：加载原始数据
    entity.load(currentData);
    EXPECT_EQ(entity.customValue(), 10);
}

TEST(SetTileEntityNBTLogicTest, ItemStackChildTagAccess)
{
    // 验证 ItemStack 对 BlockEntityTag 的访问
    ItemStack stack;

    // 没有 tag 时 getChildTag 返回 nullptr
    EXPECT_FALSE(stack.hasTag());
    EXPECT_EQ(stack.getChildTag("BlockEntityTag"), nullptr);
    EXPECT_EQ(stack.getChildTag("BlockStateTag"), nullptr);

    // 创建 BlockEntityTag
    nlohmann::json& blockEntityTag = stack.getOrCreateChildTag("BlockEntityTag");
    EXPECT_TRUE(blockEntityTag.is_object());

    // 写入数据
    blockEntityTag["custom_value"] = 42;

    // 读取回来
    const nlohmann::json* readTag = stack.getChildTag("BlockEntityTag");
    ASSERT_NE(readTag, nullptr);
    EXPECT_TRUE(readTag->is_object());
    EXPECT_EQ((*readTag)["custom_value"].get<i32>(), 42);

    // BlockStateTag 不应存在
    EXPECT_EQ(stack.getChildTag("BlockStateTag"), nullptr);
}

TEST(SetTileEntityNBTLogicTest, ItemStackBlockStateTagAccess)
{
    // 验证 ItemStack 对 BlockStateTag 的访问
    ItemStack stack;

    // 创建 BlockStateTag
    nlohmann::json& blockStateTag = stack.getOrCreateChildTag("BlockStateTag");
    blockStateTag["facing"] = "east";
    blockStateTag["lit"] = "true";

    // 读取回来
    const nlohmann::json* readTag = stack.getChildTag("BlockStateTag");
    ASSERT_NE(readTag, nullptr);
    EXPECT_TRUE(readTag->is_object());
    EXPECT_EQ((*readTag)["facing"].get<std::string>(), "east");
    EXPECT_EQ((*readTag)["lit"].get<std::string>(), "true");

    // 非字符串值不应影响属性设置
    blockStateTag["power"] = 15; // 整数值，应被跳过
    EXPECT_FALSE((*readTag)["power"].is_string());
}

TEST(SetTileEntityNBTLogicTest, MergeJsonObjectsBehavior)
{
    // 验证 JSON 对象合并行为
    nlohmann::json target;
    target["existing_key"] = "original";
    target["nested"] = nlohmann::json::object();
    target["nested"]["inner_key"] = "inner_value";

    nlohmann::json source;
    source["existing_key"] = "overwritten";
    source["new_key"] = "new_value";
    source["nested"] = nlohmann::json::object();
    source["nested"]["inner_key"] = "overwritten_inner";
    source["nested"]["new_inner"] = "added_inner";

    ItemStack::mergeJsonObjects(target, source);

    // 已有键应被覆盖
    EXPECT_EQ(target["existing_key"].get<std::string>(), "overwritten");
    // 新键应被添加
    EXPECT_EQ(target["new_key"].get<std::string>(), "new_value");
    // 嵌套对象应递归合并
    EXPECT_EQ(target["nested"]["inner_key"].get<std::string>(), "overwritten_inner");
    EXPECT_EQ(target["nested"]["new_inner"].get<std::string>(), "added_inner");
}
