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

#include "CopperChestBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/ChestBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// CopperChestBlock 实现
// ============================================================================

CopperChestBlock::CopperChestBlock(const BlockProperties& properties,
    BlockStateProperties::OxidationLevel oxidationLevel,
    const ResourceLocation& openSound,
    const ResourceLocation& closeSound)
    : ChestBlock(properties)
    , m_oxidationLevel(oxidationLevel)
    , m_openSound(openSound)
    , m_closeSound(closeSound)
{
    // 父类 ChestBlock 已经创建 HORIZONTAL_FACING + CHEST_TYPE + WATERLOGGED 状态容器
    // 铜箱子不额外添加 OXIDATION 属性到方块状态：
    //   - MC Java 1.21.11 中铜箱子的氧化等级通过方块 ID 区分（copper_chest / exposed_copper_chest ...）
    //     而非 OXIDATION 方块状态属性，每个氧化等级是独立的方块类型
    //   - 这与铜傀儡雕像不同（雕像使用 OXIDATION 属性）
    //   - m_oxidationLevel 成员变量仅用于双箱合并时比较氧化等级
    // 开合音效按氧化等级映射（涂蜡变体复用对应氧化等级的声音）：
    //   - Unaffected/Exposed -> block.copper_chest.open/close
    //   - Weathered -> block.copper_chest_weathered.open/close
    //   - Oxidized -> block.copper_chest_oxidized.open/close
}

BlockState CopperChestBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 调用父类放置逻辑，获得基础状态（HORIZONTAL_FACING + CHEST_TYPE + WATERLOGGED）
    BlockState state = ChestBlock::getStateForPlacement(context);

    // 双箱合并时取较低氧化等级（且优先未涂蜡）的方块类型
    // 对应 MC Java: CopperChestBlock.getLeastOxidizedChestOfConnectedBlocks
    const BlockPos& clickedPos = context.placementPos();
    IWorld& world = context.getWorld();

    // 获取连接方向（LEFT/RIGHT 时为邻居方向，SINGLE 时为 None）
    Direction connectedDir = getConnectedDirection(state);
    if (connectedDir == Direction::None) {
        return state;
    }

    BlockPos neighborPos = clickedPos.offset(connectedDir);
    const BlockState* neighborState = world.getBlockState(neighborPos);
    if (neighborState == nullptr) {
        return state;
    }

    // 检查邻居是否也是铜箱子
    const Block& neighborBlock = neighborState->getBlock();
    auto* neighborCopperChest = dynamic_cast<const CopperChestBlock*>(&neighborBlock);
    if (neighborCopperChest == nullptr) {
        return state;
    }

    // 比较氧化等级：取较低等级的方块作为合并后方块类型
    // 对应 MC Java: copperchestblock.weatherState.ordinal() <= copperchestblock1.weatherState.ordinal()
    //               ? blockstate2.getBlock() : blockstate1.getBlock()
    const Block* targetBlock;
    if (static_cast<i32>(m_oxidationLevel) <= static_cast<i32>(neighborCopperChest->m_oxidationLevel)) {
        targetBlock = &state.getBlock();
    } else {
        targetBlock = &neighborBlock;
    }

    // 使用 targetBlock 的默认状态，复制当前 state 的共有属性
    // 对应 MC Java: block.withPropertiesOf(blockstate2)
    return targetBlock->defaultState().withPropertiesOf(state);
}

BlockState CopperChestBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 调用父类逻辑处理水logged + 双箱连接/断开
    BlockState result = ChestBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);

    // 若与相邻铜箱子建立连接，将当前方块类型同步为相邻方块的类型
    // 对应 MC Java: CopperChestBlock.updateShape 中的 chestCanConnectTo 分支
    if (chestCanConnectTo(facingState)) {
        BlockStateProperties::ChestType chestType = result.get(BlockStateProperties::CHEST_TYPE());
        if (chestType != BlockStateProperties::ChestType::Single && getConnectedDirection(result) == facing) {
            // 将相邻方块类型应用当前方块的属性（FACING/TYPE/WATERLOGGED）
            return facingState.getBlock().defaultState().withPropertiesOf(result);
        }
    }

    return result;
}

bool CopperChestBlock::chestCanConnectTo(const BlockState& neighborState) const
{
    // 铜箱子允许跨氧化等级与涂蜡状态连接：邻居在 COPPER_CHESTS 标签中且拥有 CHEST_TYPE 属性
    // 对应 MC Java: CopperChestBlock.chestCanConnectTo
    return BlockTags::COPPER_CHESTS().contains(neighborState) &&
        neighborState.hasProperty(BlockStateProperties::CHEST_TYPE());
}

void CopperChestBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 铜箱子被移除时：若新方块仍然是铜箱子（氧化/涂蜡/除蜡/刮削导致的方块类型变化），
    // 则不掉落物品（方块实体将由 ServerWorld::setBlockState 通过 shouldChangedStateKeepBlockEntity 保留）。
    // 否则按普通箱子逻辑掉落内容物。
    // 对应 MC Java: ChestBlock.onRemove + CopperChestBlock.shouldChangedStateKeepBlockEntity
    const BlockState* newState = world.getBlockState(pos);
    if (newState != nullptr && BlockTags::COPPER_CHESTS().contains(*newState)) {
        // 新方块仍然是铜箱子：方块实体会被保留，不掉落物品
        return;
    }

    // 新方块不是铜箱子（被破坏或替换为其他方块）：调用父类逻辑掉落物品
    ChestBlock::onBlockRemoved(world, pos, state);
}

std::unique_ptr<BlockEntity> CopperChestBlock::createBlockEntity(const BlockPos& pos)
{
    // 铜箱子复用 ChestEntity（BlockEntityType::Chest）
    // 方块实体的迁移（保留物品内容物）由 ServerWorld::setBlockState 通过
    // shouldChangedStateKeepBlockEntity 机制处理，此处创建的是空箱子实体
    // （仅在方块首次放置时调用；氧化/涂蜡/除蜡/刮削时复用旧实体）
    return std::make_unique<blockentity::ChestEntity>(pos);
}

BlockState CopperChestBlock::getFromCopperBlock(
    const Block* copperBlock, Direction facing, IWorld& world, const BlockPos& pos)
{
    // 对应 MC 1.21.11: CopperChestBlock.getFromCopperBlock
    //   CopperChestBlock copperchestblock = (CopperChestBlock)COPPER_TO_COPPER_CHEST_MAPPING
    //       .getOrDefault(p_434003_, Blocks.COPPER_CHEST::asBlock).get();
    //   ChestType chesttype = copperchestblock.getChestType(p_442654_, p_443072_, p_442692_);
    //   BlockState blockstate = copperchestblock.defaultBlockState()
    //       .setValue(FACING, p_442692_).setValue(TYPE, chesttype);
    //   return getLeastOxidizedChestOfConnectedBlocks(blockstate, p_442654_, p_443072_);

    // 选择目标铜箱子方块：根据源铜块的氧化等级与涂蜡状态映射
    // 涂蜡铜块映射到对应氧化等级的"未涂蜡"铜箱子（与 MC 一致：涂蜡状态不传递到箱子）
    Block* targetChestBlock = VanillaBlocks::COPPER_CHEST;
    if (copperBlock != nullptr) {
        if (copperBlock == VanillaBlocks::COPPER_BLOCK || copperBlock == VanillaBlocks::WAXED_COPPER_BLOCK) {
            targetChestBlock = VanillaBlocks::COPPER_CHEST;
        } else if (copperBlock == VanillaBlocks::EXPOSED_COPPER || copperBlock == VanillaBlocks::WAXED_EXPOSED_COPPER) {
            targetChestBlock = VanillaBlocks::EXPOSED_COPPER_CHEST;
        } else if (copperBlock == VanillaBlocks::WEATHERED_COPPER ||
            copperBlock == VanillaBlocks::WAXED_WEATHERED_COPPER) {
            targetChestBlock = VanillaBlocks::WEATHERED_COPPER_CHEST;
        } else if (copperBlock == VanillaBlocks::OXIDIZED_COPPER ||
            copperBlock == VanillaBlocks::WAXED_OXIDIZED_COPPER) {
            targetChestBlock = VanillaBlocks::OXIDIZED_COPPER_CHEST;
        }
        // 其他铜块（如切制铜、铜门等）回退到 copper_chest（与 MC 默认行为一致）
    }

    if (targetChestBlock == nullptr) {
        // VanillaBlocks::COPPER_CHEST 未注册（理论上不应发生，VanillaBlocks::initialize 早于任何 golem 生成）
        // 兜底返回原铜块状态，避免破坏世界
        MC_ASSERT(false && "CopperChestBlock::getFromCopperBlock: VanillaBlocks::COPPER_CHEST not registered");
        if (copperBlock != nullptr) {
            return copperBlock->defaultState();
        }
        // 实在没有方块可用，返回 air（VanillaBlocks::AIR 一定已注册）
        return VanillaBlocks::AIR->defaultState();
    }

    // 自动判定 ChestType（Single/Left/Right）：扫描四个水平方向寻找可连接的铜箱子
    // 对应 MC Java: CopperChestBlock.getChestType(Level, BlockPos, Direction)
    BlockStateProperties::ChestType chestType = BlockStateProperties::ChestType::Single;
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr) {
            continue;
        }

        // 检查邻居是否为可连接的箱子（COPPER_CHESTS 标签 + 拥有 CHEST_TYPE 属性）
        const CopperChestBlock* neighborChest = dynamic_cast<const CopperChestBlock*>(&neighborState->getBlock());
        if (neighborChest == nullptr) {
            // 也可能为普通箱子（ChestBlock 子类），通过 chestCanConnectTo 间接判断
            const ChestBlock* neighborAnyChest = dynamic_cast<const ChestBlock*>(&neighborState->getBlock());
            if (neighborAnyChest == nullptr || !neighborAnyChest->chestCanConnectTo(*neighborState)) {
                continue;
            }
        } else if (!BlockTags::COPPER_CHESTS().contains(*neighborState)) {
            continue;
        }

        if (!neighborState->hasProperty(BlockStateProperties::CHEST_TYPE())) {
            continue;
        }

        Direction neighborFacing = neighborState->get(BlockStateProperties::HORIZONTAL_FACING());
        BlockStateProperties::ChestType neighborType = neighborState->get(BlockStateProperties::CHEST_TYPE());

        // 只有朝向相同且是单箱的才能合并
        if (neighborFacing == facing && neighborType == BlockStateProperties::ChestType::Single) {
            if (dir == Directions::rotateYCCW(facing)) {
                chestType = BlockStateProperties::ChestType::Right;
            } else if (dir == Directions::rotateY(facing)) {
                chestType = BlockStateProperties::ChestType::Left;
            }
            break;
        }
    }

    // 构造铜箱子状态：默认状态 + FACING + CHEST_TYPE
    BlockState result = targetChestBlock->defaultState()
                            .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                            .with(BlockStateProperties::CHEST_TYPE(), chestType);

    // 与相邻铜箱子合并时取较低氧化等级的方块类型
    // 对应 MC Java: getLeastOxidizedChestOfConnectedBlocks
    if (chestType != BlockStateProperties::ChestType::Single) {
        Direction connectedDir = ChestBlock::getConnectedDirection(result);
        BlockPos neighborPos = pos.offset(connectedDir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState != nullptr) {
            const CopperChestBlock* neighborChest = dynamic_cast<const CopperChestBlock*>(&neighborState->getBlock());
            const CopperChestBlock* selfChest = dynamic_cast<const CopperChestBlock*>(targetChestBlock);
            if (neighborChest != nullptr && selfChest != nullptr) {
                // 比较氧化等级：取较低等级的方块作为合并后方块类型
                const Block* targetBlock = (static_cast<i32>(selfChest->getOxidationLevel()) <=
                                               static_cast<i32>(neighborChest->getOxidationLevel()))
                    ? targetChestBlock
                    : &neighborState->getBlock();
                result = targetBlock->defaultState().withPropertiesOf(result);
            }
        }
    }

    return result;
}

// ============================================================================
// WeatheringCopperChestBlock 实现
// ============================================================================

WeatheringCopperChestBlock::WeatheringCopperChestBlock(const BlockProperties& properties,
    BlockStateProperties::OxidationLevel oxidationLevel,
    const ResourceLocation& openSound,
    const ResourceLocation& closeSound)
    : CopperChestBlock(properties, oxidationLevel, openSound, closeSound)
{
    // 可氧化铜箱子同样不添加 OXIDATION 方块状态属性（氧化等级通过方块 ID 区分）
    // ticksRandomly 由 ticksRandomly() 虚方法动态返回，不修改 m_ticksRandomly 标志
    // 开合音效由父类 CopperChestBlock 存储，按氧化等级映射
}

void WeatheringCopperChestBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对应 MC Java: WeatheringCopperChestBlock.randomTick
    // 满足以下条件之一时不氧化：
    // 1. 当前箱子是双箱的 RIGHT 部分（避免双箱两侧同时氧化导致不同步）
    // 2. 箱子正在被玩家打开（ChestEntity::getOpenCount() > 0）
    if (state.get(BlockStateProperties::CHEST_TYPE()) == BlockStateProperties::ChestType::Right) {
        return;
    }

    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr) {
        auto* chestEntity = dynamic_cast<blockentity::ChestEntity*>(be);
        if (chestEntity != nullptr && chestEntity->getOpenCount() > 0) {
            return;
        }
    }

    // 调用 IOxidizableBlock::tryOxidize 尝试氧化到下一等级
    // tryOxidize 内部使用 withPropertiesOf() 保留 HORIZONTAL_FACING/CHEST_TYPE/WATERLOGGED 属性
    (void)tryOxidize(world, pos, state, random);
}

} // namespace blocks
} // namespace mc
