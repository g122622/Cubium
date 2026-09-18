/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "common/TestWorldHelper.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "server/command/data/DataAccessor.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::blockentity;

// ========== 测试用世界桩 ==========

/// @brief 测试用世界桩，支持方块实体的存储和方块更新通知追踪
class BlockDataAccessorTestWorld final : public IWorld {
public:
    struct BlockUpdateCall {
        BlockPos pos;
    };

    // --- IWorld 接口实现（仅实现 BlockDataAccessor 所需的方法）---
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BlockDataAccessorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BlockDataAccessorTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // --- 重写需要追踪的方法 ---
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        if (m_blockEntity != nullptr && m_blockEntity->getPos() == pos) {
            return m_blockEntity;
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        if (m_blockEntity != nullptr && m_blockEntity->getPos() == pos) {
            return m_blockEntity;
        }
        return nullptr;
    }

    void notifyBlockUpdate(const BlockPos& pos) override { m_blockUpdateCalls.push_back({pos}); }

    // --- 测试辅助方法 ---
    void setBlockEntity(BlockEntity* entity) { m_blockEntity = entity; }

    [[nodiscard]] const std::vector<BlockUpdateCall>& blockUpdateCalls() const { return m_blockUpdateCalls; }
    void clearTrackedCalls() { m_blockUpdateCalls.clear(); }

private:
    BlockEntity* m_blockEntity = nullptr;
    mutable math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
    world::gamerule::GameRules m_gameRules;
    std::vector<BlockUpdateCall> m_blockUpdateCalls;
};

// ========== 测试夹具 ==========

class BlockDataAccessorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建 SignEntity（有完整的 saveToNBT/loadFromNBT 实现）
        signEntity_ = std::make_unique<SignEntity>(kPos);
        signEntity_->setWorld(&world_);
        world_.setBlockEntity(signEntity_.get());
    }

    void TearDown() override
    {
        world_.setBlockEntity(nullptr);
        signEntity_.reset();
    }

    BlockPos kPos{10, 20, 30};
    BlockDataAccessorTestWorld world_;
    std::unique_ptr<SignEntity> signEntity_;
};

// ========== 构造和 isValid 测试 ==========

TEST_F(BlockDataAccessorTest, Constructor_NullWorld_IsNotValid)
{
    BlockDataAccessor accessor(nullptr, kPos);
    EXPECT_FALSE(accessor.isValid());
}

TEST_F(BlockDataAccessorTest, Constructor_WithWorldAndBlockEntity_IsValid)
{
    BlockDataAccessor accessor(&world_, kPos);
    EXPECT_TRUE(accessor.isValid());
}

TEST_F(BlockDataAccessorTest, Constructor_WithWorldButNoBlockEntity_IsNotValid)
{
    // 不同位置没有方块实体
    BlockDataAccessor accessor(&world_, BlockPos(99, 99, 99));
    EXPECT_FALSE(accessor.isValid());
}

TEST_F(BlockDataAccessorTest, GetPosition_ReturnsConstructorPosition)
{
    BlockDataAccessor accessor(&world_, kPos);
    EXPECT_EQ(accessor.getPosition(), kPos);
}

// ========== getData 测试 ==========

TEST_F(BlockDataAccessorTest, GetData_NoBlockEntity_ThrowsException)
{
    BlockDataAccessor accessor(nullptr, kPos);
    EXPECT_THROW(accessor.getData(), CommandException);
}

TEST_F(BlockDataAccessorTest, GetData_WrongPosition_ThrowsException)
{
    // 位置不匹配时 getBlockEntity 返回 nullptr，isValid 为 false
    BlockDataAccessor accessor(&world_, BlockPos(99, 99, 99));
    EXPECT_FALSE(accessor.isValid());
    EXPECT_THROW(accessor.getData(), CommandException);
}

TEST_F(BlockDataAccessorTest, GetData_BaseBlockEntity_ContainsBasicFields)
{
    // 基类 BlockEntity::saveToNBT 写入 id, x, y, z
    BlockDataAccessor accessor(&world_, kPos);
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    // 验证基础字段
    auto idIt = data->value.find("id");
    ASSERT_NE(idIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::string_tag&>(*idIt->second).value, "minecraft:sign");

    auto xIt = data->value.find("x");
    ASSERT_NE(xIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*xIt->second).value, kPos.x);

    auto yIt = data->value.find("y");
    ASSERT_NE(yIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*yIt->second).value, kPos.y);

    auto zIt = data->value.find("z");
    ASSERT_NE(zIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*zIt->second).value, kPos.z);
}

TEST_F(BlockDataAccessorTest, GetData_SignEntity_ContainsSpecificFields)
{
    // SignEntity 额外保存 lines, editable, color, glowing, is_waxed
    BlockDataAccessor accessor(&world_, kPos);
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    // 验证 SignEntity 特有字段
    auto linesIt = data->value.find("lines");
    ASSERT_NE(linesIt, data->value.end());
    ASSERT_EQ(linesIt->second->id(), nbt::TagId::List);

    auto editableIt = data->value.find("editable");
    ASSERT_NE(editableIt, data->value.end());
    EXPECT_EQ(editableIt->second->id(), nbt::TagId::Byte);

    auto colorIt = data->value.find("color");
    ASSERT_NE(colorIt, data->value.end());
    EXPECT_EQ(colorIt->second->id(), nbt::TagId::Int);

    auto glowingIt = data->value.find("glowing");
    ASSERT_NE(glowingIt, data->value.end());
    EXPECT_EQ(glowingIt->second->id(), nbt::TagId::Byte);

    auto waxedIt = data->value.find("is_waxed");
    ASSERT_NE(waxedIt, data->value.end());
    EXPECT_EQ(waxedIt->second->id(), nbt::TagId::Byte);
}

// ========== mergeData 测试 ==========

TEST_F(BlockDataAccessorTest, MergeData_NoBlockEntity_ThrowsException)
{
    BlockDataAccessor accessor(nullptr, kPos);
    nbt::tags::compound_tag data;
    EXPECT_THROW(accessor.mergeData(data), CommandException);
}

TEST_F(BlockDataAccessorTest, MergeData_AddsNewField)
{
    // 合并一个 SignEntity 能识别的字段
    // 注意：BlockDataAccessor 使用 saveToNBT/loadFromNBT 路径，
    // loadFromNBT 只会读回子类识别的字段，未知字段会被忽略。
    // 这是预期行为：与 MC Java 的 BlockEntity.loadWithComponents 一致。
    BlockDataAccessor accessor(&world_, kPos);

    // 修改 is_waxed 字段（SignEntity 识别的字段）
    nbt::tags::compound_tag data;
    data.put("is_waxed", static_cast<i8>(1));

    accessor.mergeData(data);

    // 验证字段已合并且被 SignEntity 正确加载
    auto mergedData = accessor.getData();
    ASSERT_NE(mergedData, nullptr);
    auto waxedIt = mergedData->value.find("is_waxed");
    ASSERT_NE(waxedIt, mergedData->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*waxedIt->second).value, 1);

    // 原有字段仍存在
    auto idIt = mergedData->value.find("id");
    ASSERT_NE(idIt, mergedData->value.end());
}

TEST_F(BlockDataAccessorTest, MergeData_OverwritesExistingField)
{
    // 覆盖已有的字段
    BlockDataAccessor accessor(&world_, kPos);

    // 先检查 is_waxed 默认为 false
    auto originalData = accessor.getData();
    ASSERT_NE(originalData, nullptr);
    auto waxedIt = originalData->value.find("is_waxed");
    ASSERT_NE(waxedIt, originalData->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*waxedIt->second).value, 0);

    // 合并修改 is_waxed 为 true
    nbt::tags::compound_tag data;
    data.put("is_waxed", static_cast<i8>(1));
    accessor.mergeData(data);

    // 验证字段已被覆盖
    auto mergedData = accessor.getData();
    ASSERT_NE(mergedData, nullptr);
    waxedIt = mergedData->value.find("is_waxed");
    ASSERT_NE(waxedIt, mergedData->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*waxedIt->second).value, 1);
}

TEST_F(BlockDataAccessorTest, MergeData_EmptyCompound_NoChange)
{
    // 合并空数据不应改变方块实体
    BlockDataAccessor accessor(&world_, kPos);
    auto originalData = accessor.getData();
    ASSERT_NE(originalData, nullptr);

    // 记录原始字段数量
    std::size_t originalSize = originalData->value.size();

    nbt::tags::compound_tag emptyData;
    accessor.mergeData(emptyData);

    auto mergedData = accessor.getData();
    ASSERT_NE(mergedData, nullptr);
    // 字段数量不应减少
    EXPECT_GE(mergedData->value.size(), originalSize);
}

TEST_F(BlockDataAccessorTest, MergeData_NotifiesBlockUpdate)
{
    // mergeData 应调用 notifyBlockUpdate
    BlockDataAccessor accessor(&world_, kPos);
    world_.clearTrackedCalls();

    nbt::tags::compound_tag data;
    data.put("is_waxed", static_cast<i8>(1));
    accessor.mergeData(data);

    // 验证 notifyBlockUpdate 被调用且位置正确
    ASSERT_EQ(world_.blockUpdateCalls().size(), 1u);
    EXPECT_EQ(world_.blockUpdateCalls()[0].pos, kPos);
}

TEST_F(BlockDataAccessorTest, MergeData_SetsChangedFlag)
{
    // mergeData 应设置 BlockEntity 的 changed 标志
    signEntity_->clearChanged();
    EXPECT_FALSE(signEntity_->isChanged());

    BlockDataAccessor accessor(&world_, kPos);
    nbt::tags::compound_tag data;
    data.put("is_waxed", static_cast<i8>(1));
    accessor.mergeData(data);

    EXPECT_TRUE(signEntity_->isChanged());
}

TEST_F(BlockDataAccessorTest, GetData_DoesNotNotifyBlockUpdate)
{
    // getData 不应触发方块更新
    BlockDataAccessor accessor(&world_, kPos);
    world_.clearTrackedCalls();

    accessor.getData();

    EXPECT_EQ(world_.blockUpdateCalls().size(), 0u);
}

// ========== 序列化/反序列化往返测试 ==========

TEST_F(BlockDataAccessorTest, RoundTrip_SerializeDeserialize_PreservesSignData)
{
    // 设置 SignEntity 特有数据
    signEntity_->setWaxed(true);
    signEntity_->setGlowing(true);

    BlockDataAccessor accessor(&world_, kPos);
    auto data = accessor.getData();
    ASSERT_NE(data, nullptr);

    // 验证设置的字段在序列化数据中
    auto waxedIt = data->value.find("is_waxed");
    ASSERT_NE(waxedIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*waxedIt->second).value, 1);

    auto glowingIt = data->value.find("glowing");
    ASSERT_NE(glowingIt, data->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::byte_tag&>(*glowingIt->second).value, 1);
}

TEST_F(BlockDataAccessorTest, RoundTrip_MergePreservesUnchangedFields)
{
    // 合并不影响未修改的字段
    BlockDataAccessor accessor(&world_, kPos);
    auto originalData = accessor.getData();
    ASSERT_NE(originalData, nullptr);

    // 记录原始 color 值
    auto colorIt = originalData->value.find("color");
    ASSERT_NE(colorIt, originalData->value.end());
    i32 originalColor = dynamic_cast<const nbt::tags::int_tag&>(*colorIt->second).value;

    // 只修改 is_waxed
    nbt::tags::compound_tag data;
    data.put("is_waxed", static_cast<i8>(1));
    accessor.mergeData(data);

    // color 不应被改变
    auto mergedData = accessor.getData();
    ASSERT_NE(mergedData, nullptr);
    colorIt = mergedData->value.find("color");
    ASSERT_NE(colorIt, mergedData->value.end());
    EXPECT_EQ(dynamic_cast<const nbt::tags::int_tag&>(*colorIt->second).value, originalColor);
}

// ========== 显示消息测试 ==========

TEST_F(BlockDataAccessorTest, GetDisplayName_ReturnsFormattedPosition)
{
    BlockDataAccessor accessor(&world_, kPos);
    EXPECT_EQ(accessor.getDisplayName(), "block at 10, 20, 30");
}

TEST_F(BlockDataAccessorTest, GetModifiedMessage_ReturnsFormattedPosition)
{
    BlockDataAccessor accessor(&world_, kPos);
    EXPECT_EQ(accessor.getModifiedMessage(), "Modified block data at 10, 20, 30");
}
