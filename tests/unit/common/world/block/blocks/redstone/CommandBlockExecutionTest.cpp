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

#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/special/CommandBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/redstone/CommandBlockEntity.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using mc::blockentity::CommandBlockEntity;
using mc::blockentity::CommandBlockMode;

// 命令方块执行链路测试：对齐 MC Java 1.21.11 CommandBlock.tick / executeChain /
// setPoweredAndUpdate / setPlacedBy 与 CommandBlockEntity.markConditionMet / setAutomatic。
namespace {

/// 命令方块测试世界：稀疏方块 + 方块实体注册 + 命令执行记录。
class CommandBlockTestWorld final : public mc::test::BaseTestWorld {
public:
    CommandBlockTestWorld() { m_tickManager = std::make_unique<world::tick::TickManager>(*this); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        // BlockState 不可拷贝（状态对象由方块持有），与其它测试世界一致只保存指针。
        const BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManager; }

    i32 executeCommand(
        const std::string& command, const Vector3d&, i32, const Vector2f&, Player* /*player*/ = nullptr) override
    {
        m_executedCommands.push_back(command);
        return 1;
    }

    /// 清除方块、方块实体与执行记录（世界实例在用例之间复用）。
    void reset()
    {
        m_executedCommands.clear();
        m_blocks.clear();
        m_blockEntities.clear();
        m_tickManager = std::make_unique<world::tick::TickManager>(*this);
    }

    /// 放置方块并在 pos 处创建命令方块实体（方块实体关联本世界）。
    /// 朝向用于构造状态属性（默认 North）；连锁链沿方块自身 FACING 前进。
    CommandBlockEntity& placeCommandBlock(const Block& block,
        const BlockPos& pos,
        CommandBlockMode mode,
        bool conditional,
        Direction facing = Direction::North)
    {
        // 使用方块类注册的正式状态对象（推导结果与注册表同一实例）。
        const BlockState* state = &block.defaultState().with(BlockStateProperties::FACING(), facing);
        if (conditional) {
            state = &state->with(BlockStateProperties::CONDITIONAL(), true);
        }
        setBlockState(pos.x, pos.y, pos.z, state);
        auto entity = std::make_unique<CommandBlockEntity>(pos, mode);
        entity->setWorld(this);
        CommandBlockEntity& ref = *entity;
        m_blockEntities[pos] = std::move(entity);
        return ref;
    }

    [[nodiscard]] const std::vector<std::string>& executedCommands() const { return m_executedCommands; }

    /// 清空命令执行记录（同一次用例内分阶段断言时使用）。
    void clearExecutedCommands() { m_executedCommands.clear(); }

    /// 模拟 BlockItem 放置完成后的方块侧回调（携带普通物品堆，无方块实体数据）。
    void placeBlock(const Block& block, const BlockPos& pos)
    {
        const_cast<Block&>(block).onBlockPlacedBy(
            *this, pos, *getBlockState(pos.x, pos.y, pos.z), nullptr, ItemStack(Items::STONE, 1));
    }

    /// 模拟调度刻执行。vanilla 在计划刻触发时会从世界重新读取方块状态；
    /// 此处传入的是世界中的正式注册状态对象，状态不可变，const_cast 仅用于适配签名。
    void tickBlock(const Block& block, const BlockPos& pos)
    {
        const BlockState* state = getBlockState(pos.x, pos.y, pos.z);
        ASSERT_NE(state, nullptr);
        const_cast<Block&>(block).tick(*this, pos, *const_cast<BlockState*>(state), getRandom());
    }

private:
    std::unique_ptr<world::tick::TickManager> m_tickManager;
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::string> m_executedCommands;
};

class CommandBlockExecutionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override { world.reset(); }

    CommandBlockTestWorld world;
    Block* commandBlock = VanillaBlocks::COMMAND_BLOCK;
    Block* chainBlock = VanillaBlocks::CHAIN_COMMAND_BLOCK;
};

} // namespace

// 循环模式：自动执行且未供电时每刻执行命令并自我续期。
TEST_F(CommandBlockExecutionTest, AutoMode_AutomaticExecutesAndReschedules)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Auto, false);
    entity.setCommand("say hi");
    entity.setPowered(false);
    entity.setAuto(true);

    world.tickBlock(*commandBlock, pos);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(world.executedCommands()[0], "say hi");
    EXPECT_TRUE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// 循环模式被供电（非自动）：同样执行并自我续期（对齐 vanilla：isPowered || isAutomatic 时继续调度）。
TEST_F(CommandBlockExecutionTest, AutoMode_PoweredExecutesAndReschedules)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Auto, false);
    entity.setCommand("say hi");
    entity.setAuto(false);
    entity.setPowered(true);

    world.tickBlock(*commandBlock, pos);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(world.executedCommands()[0], "say hi");
    EXPECT_TRUE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// 循环模式既未供电又非自动：不执行、不续期。
TEST_F(CommandBlockExecutionTest, AutoMode_NotPoweredAndNotAutomatic_DoesNotExecute)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Auto, false);
    entity.setCommand("say hi");
    entity.setPowered(false);
    entity.setAuto(false);

    world.tickBlock(*commandBlock, pos);
    EXPECT_TRUE(world.executedCommands().empty());
    EXPECT_FALSE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// 脉冲模式：被供电时方块刻执行一次命令，且不自我续期。
TEST_F(CommandBlockExecutionTest, RedstoneMode_ExecutesOnceOnScheduledTick)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Redstone, false);
    entity.setCommand("say pulse");
    entity.setPowered(true);

    world.tickBlock(*commandBlock, pos);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(world.executedCommands()[0], "say pulse");
    EXPECT_EQ(entity.getSuccessCount(), 1);
    EXPECT_FALSE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// 条件脉冲方块：背后（FACING 反方向）命令方块成功计数为 0 时不执行，并清零自身成功计数。
TEST_F(CommandBlockExecutionTest, Conditional_BlockedWhenBackBlockHasNoSuccess)
{
    const BlockPos behind(0, 64, 1); // FACING=North（-Z）→ 背后为 z+1
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& back = world.placeCommandBlock(*commandBlock, behind, CommandBlockMode::Redstone, false);
    back.setSuccessCount(0);
    ASSERT_EQ(back.getSuccessCount(), 0);

    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Redstone, true);
    entity.setCommand("say conditional");
    entity.setPowered(true);
    entity.setSuccessCount(3);

    world.tickBlock(*commandBlock, pos);
    EXPECT_TRUE(world.executedCommands().empty());
    EXPECT_EQ(entity.getSuccessCount(), 0);
    EXPECT_FALSE(entity.isConditionMet());
}

// 条件脉冲方块：背后命令方块成功计数 > 0 时执行。
TEST_F(CommandBlockExecutionTest, Conditional_ExecutesWhenBackBlockSucceeded)
{
    const BlockPos behind(0, 64, 1);
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& back = world.placeCommandBlock(*commandBlock, behind, CommandBlockMode::Redstone, false);
    back.setSuccessCount(5);

    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Redstone, true);
    entity.setCommand("say conditional");
    entity.setPowered(true);

    world.tickBlock(*commandBlock, pos);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(entity.getSuccessCount(), 1);
    EXPECT_TRUE(entity.isConditionMet());
}

// 连锁：沿 FACING 触发满足条件、已供电的连锁方块。
TEST_F(CommandBlockExecutionTest, ExecuteChain_RunsPoweredChainBlocks)
{
    const BlockPos first(0, 64, 0);
    const BlockPos second(0, 64, -1); // 头部 FACING=North，连锁方块位于其正前方（z-1）
    CommandBlockEntity& head =
        world.placeCommandBlock(*commandBlock, first, CommandBlockMode::Redstone, false, Direction::North);
    head.setPowered(true);
    CommandBlockEntity& chain = world.placeCommandBlock(*chainBlock, second, CommandBlockMode::Sequence, false);
    chain.setPowered(true);
    chain.setCommand("say chain");
    chain.setAuto(true);

    world.tickBlock(*commandBlock, first);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(world.executedCommands()[0], "say chain");
}

// 连锁：未供电且非自动的连锁方块不执行。
TEST_F(CommandBlockExecutionTest, ExecuteChain_SkipsUnpoweredChainBlock)
{
    const BlockPos first(0, 64, 0);
    const BlockPos second(0, 64, -1);
    CommandBlockEntity& head =
        world.placeCommandBlock(*commandBlock, first, CommandBlockMode::Redstone, false, Direction::North);
    head.setPowered(true);
    CommandBlockEntity& chain = world.placeCommandBlock(*chainBlock, second, CommandBlockMode::Sequence, false);
    chain.setPowered(false);
    chain.setAuto(false);
    chain.setCommand("say chain");

    world.tickBlock(*commandBlock, first);
    EXPECT_TRUE(world.executedCommands().empty());
    EXPECT_EQ(chain.getSuccessCount(), 0);
}

// 连锁：条件满足的连锁方块执行后，链继续沿其自身朝向推进到下一个连锁方块。
TEST_F(CommandBlockExecutionTest, ExecuteChain_ConditionalChainBlockRuns)
{
    // 头部位于 z=1 且面向 North（-Z），前方 z=0 为条件连锁方块。
    const BlockPos first(0, 64, 1);
    const BlockPos conditionalPos(0, 64, 0);
    CommandBlockEntity& head =
        world.placeCommandBlock(*commandBlock, first, CommandBlockMode::Redstone, false, Direction::North);
    head.setPowered(true);
    // 头部空命令：执行时把成功计数置 0（对齐 vanilla execute 的失败分支），因此这里让头部
    // 实际执行一条命令，使其成功计数 > 0，供条件方块判定。
    head.setCommand("say head");

    // 条件连锁方块朝向 North（-Z），其背后（z=1）是头部方块 → 条件满足并执行。
    CommandBlockEntity& conditional =
        world.placeCommandBlock(*chainBlock, conditionalPos, CommandBlockMode::Sequence, true);
    conditional.setPowered(true);
    conditional.setCommand("say conditional chain");

    world.tickBlock(*commandBlock, first);
    ASSERT_EQ(world.executedCommands().size(), 2u);
    EXPECT_EQ(world.executedCommands()[0], "say head");
    EXPECT_EQ(world.executedCommands()[1], "say conditional chain");
    EXPECT_TRUE(conditional.isConditionMet());
    EXPECT_GT(conditional.getSuccessCount(), 0);
}

// 连锁：条件不满足的连锁方块被跳过（清零自身计数），后续连锁方块继续执行。
TEST_F(CommandBlockExecutionTest, ExecuteChain_ConditionalFailedChainBlockSkippedButChainContinues)
{
    // 头部位于 z=1 且面向 North，前方 z=0 为条件连锁方块，其后 z=-1 为最后一个连锁方块。
    const BlockPos first(0, 64, 1);
    const BlockPos conditionalPos(0, 64, 0);
    const BlockPos last(0, 64, -1);
    CommandBlockEntity& head =
        world.placeCommandBlock(*commandBlock, first, CommandBlockMode::Redstone, false, Direction::North);
    head.setPowered(true);
    head.setCommand(""); // 头部空命令：成功计数保持 0

    // 条件连锁方块背后（z=1）是头部方块，成功计数为 0 → 条件不满足，仅清零自身计数后继续。
    CommandBlockEntity& conditional =
        world.placeCommandBlock(*chainBlock, conditionalPos, CommandBlockMode::Sequence, true);
    conditional.setPowered(true);
    conditional.setSuccessCount(9);
    conditional.setCommand("say conditional chain");

    CommandBlockEntity& lastEntity = world.placeCommandBlock(*chainBlock, last, CommandBlockMode::Sequence, false);
    lastEntity.setPowered(true);
    lastEntity.setCommand("say last");

    world.tickBlock(*commandBlock, first);
    ASSERT_EQ(world.executedCommands().size(), 1u);
    EXPECT_EQ(world.executedCommands()[0], "say last");
    EXPECT_EQ(conditional.getSuccessCount(), 0);
}

// 空命令视为失败并继续驱动连锁（不中断），成功计数清零。
TEST_F(CommandBlockExecutionTest, Execute_EmptyCommandResetsSuccessCount)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Redstone, false);
    entity.setSuccessCount(7);
    entity.setPowered(true);

    world.tickBlock(*commandBlock, pos);
    EXPECT_TRUE(world.executedCommands().empty());
    EXPECT_EQ(entity.getSuccessCount(), 0);
}

// setAuto 开启循环执行（此前未开启、未供电、非连锁）时应调度一次方块刻。
TEST_F(CommandBlockExecutionTest, SetAuto_SchedulesTickWhenEnabling)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Auto, false);
    entity.setPowered(false);
    entity.setAuto(false);

    entity.setAuto(true);
    EXPECT_TRUE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// setAuto 在已供电时不额外调度（供电路径已负责驱动）。
TEST_F(CommandBlockExecutionTest, SetAuto_DoesNotScheduleWhenPowered)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Auto, false);
    entity.setPowered(true);
    entity.setAuto(false);

    entity.setAuto(true);
    EXPECT_FALSE(world.tickManager().isBlockTickScheduled(pos, *commandBlock));
}

// 放置：脉冲命令方块默认关闭自动执行并按 sendCommandFeedback 设置追踪输出。
TEST_F(CommandBlockExecutionTest, PlacedBy_PulseBlockDefaultsToNotAutomatic)
{
    const BlockPos pos(0, 64, 0);
    CommandBlockEntity& entity = world.placeCommandBlock(*commandBlock, pos, CommandBlockMode::Redstone, false);
    entity.setAuto(true);

    world.placeBlock(*commandBlock, pos);
    EXPECT_FALSE(entity.isAuto());
    EXPECT_TRUE(entity.shouldTrackOutput());
}

// 放置：循环命令方块默认开启自动执行（对齐 vanilla new CommandBlock(true, ...)）。
TEST_F(CommandBlockExecutionTest, PlacedBy_RepeatingBlockDefaultsToAutomatic)
{
    const BlockPos pos(0, 64, 0);
    Block* repeatingBlock = VanillaBlocks::REPEATING_COMMAND_BLOCK;
    CommandBlockEntity& entity = world.placeCommandBlock(*repeatingBlock, pos, CommandBlockMode::Auto, false);
    entity.setAuto(false);

    world.placeBlock(*repeatingBlock, pos);
    EXPECT_TRUE(entity.isAuto());
}

// 创建实体：脉冲方块创建的实体不自动执行，循环方块创建的可自动执行。
TEST_F(CommandBlockExecutionTest, CreateBlockEntity_AppliesBlockAutomaticFlag)
{
    const BlockPos pulsePos(0, 64, 0);
    const BlockPos repeatPos(0, 64, 1);

    const std::unique_ptr<BlockEntity> pulseEntity = commandBlock->createBlockEntity(pulsePos);
    ASSERT_NE(pulseEntity, nullptr);
    EXPECT_EQ(pulseEntity->getType(), BlockEntityType::CommandBlock);
    EXPECT_FALSE(static_cast<CommandBlockEntity&>(*pulseEntity).isAuto());

    const std::unique_ptr<BlockEntity> repeatEntity =
        VanillaBlocks::REPEATING_COMMAND_BLOCK->createBlockEntity(repeatPos);
    ASSERT_NE(repeatEntity, nullptr);
    EXPECT_TRUE(static_cast<CommandBlockEntity&>(*repeatEntity).isAuto());
}