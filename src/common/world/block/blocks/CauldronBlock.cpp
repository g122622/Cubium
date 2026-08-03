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
 * LIABILITY, CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CauldronBlock.hpp"
#include "LavaCauldronBlock.hpp"
#include "LayeredCauldronBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CauldronBlock::CauldronBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 空炼药锅没有水位属性，不需要额外的状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 炼药锅外部形状（与 LayeredCauldronBlock 和 LavaCauldronBlock 共享相同几何）
    // 底部: (0, 0, 0) -> (16, 3, 16)
    // 壁: 2像素厚，内部12x12空间
    CollisionShape base = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 3.0f / 16.0f, 1.0f);
    CollisionShape northWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 0.0f, 1.0f, 1.0f, 2.0f / 16.0f);
    CollisionShape southWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 14.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape westWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 2.0f / 16.0f, 2.0f / 16.0f, 1.0f, 14.0f / 16.0f);
    CollisionShape eastWall = VoxelShapes::cube(14.0f / 16.0f, 3.0f / 16.0f, 2.0f / 16.0f, 1.0f, 1.0f, 14.0f / 16.0f);

    m_outerShape = CollisionShape::combine(base, northWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, southWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, westWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, eastWall, CollisionShape::CombineOp::OR);
}

// ========== 放置和更新 ==========

void CauldronBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 炼药锅不需要响应邻居更新
}

void CauldronBlock::handlePrecipitation(
    IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation)
{
    // 空炼药锅在雨天/雪天接收降水后替换为对应的炼药锅变体
    // 确定降水触发概率：雨天 5%，雪天 10%
    f32 chance = 0.0f;
    if (precipitation == world::biome::BiomeClimate::Precipitation::Rain) {
        chance = 0.05f;
    } else if (precipitation == world::biome::BiomeClimate::Precipitation::Snow) {
        chance = 0.1f;
    }

    if (chance <= 0.0f) {
        return;
    }

    // 随机概率触发
    if (world.getRandom().nextFloat() >= chance) {
        return;
    }

    // 根据降水类型替换为对应的分层炼药锅（水位1）
    // 参考: MC CauldronBlock.handlePrecipitation
    if (precipitation == world::biome::BiomeClimate::Precipitation::Rain) {
        // 雨天 → 水炼药锅
        const BlockState* waterCauldronState = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
        world.setBlockState(pos, waterCauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, waterCauldronState);
    } else if (precipitation == world::biome::BiomeClimate::Precipitation::Snow) {
        // 雪天 → 细雪炼药锅
        const BlockState* powderSnowCauldronState =
            &block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON->defaultState();
        world.setBlockState(pos, powderSnowCauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, powderSnowCauldronState);
    }
}

// ========== 交互 ==========

BlockActionResult CauldronBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    ItemStack& heldItem = player.getHeldItem(hand);
    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    // 水桶交互（空炼药锅装水或装岩浆）
    ActionResultType result = _handleBucketInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 水瓶交互（空炼药锅倒入水瓶）
    result = _handleBottleInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& CauldronBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getEntityInsideCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 空炼药锅返回完整方块形状
    // 返回 Shapes.block()（完整方块）
    return VoxelShapes::fullCube();
}

// ========== 滴石填充 ==========

void CauldronBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 当滴石调度了炼药锅的 tick 时，重新验证上方是否存在可滴水的钟乳石尖端
    std::optional<BlockPos> tipPos = PointedDripstoneBlock::findStalactiteTipAboveCauldron(world, pos);
    if (tipPos.has_value()) {
        const fluid::Fluid* fluid = PointedDripstoneBlock::getCauldronFillFluidType(world, tipPos.value());
        if (fluid != nullptr && fluid != fluid::Fluids::EMPTY() && canReceiveStalactiteDrip(*fluid)) {
            receiveStalactiteDrip(world, pos, state, *fluid);
        }
    }
}

bool CauldronBlock::canReceiveStalactiteDrip(const fluid::Fluid& fluid)
{
    // 空炼药锅可以接收任何流体（水和岩浆）的滴石滴水
    MC_UNUSED(fluid);
    return true;
}

void CauldronBlock::receiveStalactiteDrip(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid)
{
    if (fluid.isIn(fluid::FluidTags::WATER())) {
        // 水滴：空炼药锅 → 替换为水位1的水炼药锅
        const BlockState* waterCauldronState = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
        world.setBlockState(pos, waterCauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, waterCauldronState);
        world.playEvent(world::WorldEvents::DRIP_WATER_INTO_CAULDRON_SOUND, pos, 0);
    } else if (fluid.isIn(fluid::FluidTags::LAVA())) {
        // 岩浆滴：空炼药锅 → 替换为岩浆炼药锅
        const BlockState* lavaCauldronState = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
        world.setBlockState(pos, lavaCauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, lavaCauldronState);
        world.playEvent(world::WorldEvents::DRIP_LAVA_INTO_CAULDRON_SOUND, pos, 0);
    }
}

// ========== 私有方法 ==========

ActionResultType CauldronBlock::_handleBucketInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    MC_UNUSED(state);

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 水桶：空炼药锅 → 水炼药锅（水位3）
    if (item == Items::WATER_BUCKET) {
        if (!world.isClientSide()) {
            const BlockState* waterCauldronState = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState().with(
                BlockStateProperties::LEVEL_1_3(), 3);
            world.setBlockState(pos, waterCauldronState, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 触发 FLUID_PLACE 游戏事件
            world.gameEvent(gameevent::GameEvents::FLUID_PLACE, pos, waterCauldronState);

            // 非创造模式：替换为空桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 岩浆桶：空炼药锅 → 岩浆炼药锅
    if (item == Items::LAVA_BUCKET) {
        if (!world.isClientSide()) {
            const BlockState* lavaCauldronState = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
            world.setBlockState(pos, lavaCauldronState, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY_LAVA,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 触发 FLUID_PLACE 游戏事件
            world.gameEvent(gameevent::GameEvents::FLUID_PLACE, pos, lavaCauldronState);

            // 非创造模式：替换为空桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 细雪桶：空炼药锅 → 细雪炼药锅（水位3）
    // 参考: MC CauldronInteraction.fillPowderSnowInteraction
    if (item == Items::POWDER_SNOW_BUCKET) {
        if (!world.isClientSide()) {
            const BlockState* powderSnowCauldronState =
                &block_registry::BuildingBlocks::POWDER_SNOW_CAULDRON->defaultState().with(
                    BlockStateProperties::LEVEL_1_3(), 3);
            world.setBlockState(pos, powderSnowCauldronState, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY_POWDER_SNOW,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, powderSnowCauldronState);

            // 非创造模式：替换为空桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleBottleInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    MC_UNUSED(state);

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 水瓶（水瓶药水）：空炼药锅 → 水炼药锅（水位1）
    if (item == Items::POTION && potion::PotionUtils::isWaterBottle(heldItem)) {
        if (!world.isClientSide()) {
            const BlockState* waterCauldronState = &block_registry::BuildingBlocks::WATER_CAULDRON->defaultState();
            world.setBlockState(pos, waterCauldronState, 3);
            world.playSound(SoundEvents::ITEM_BOTTLE_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为玻璃瓶
            if (!player.abilities().creativeMode) {
                ItemStack glassBottle(Items::GLASS_BOTTLE, 1);
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = glassBottle;
                    player.inventory().setChanged();
                } else {
                    player.inventory().add(glassBottle);
                    if (!glassBottle.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, glassBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
