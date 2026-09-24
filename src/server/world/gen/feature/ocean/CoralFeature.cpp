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

// 珊瑚特征聚合源文件（包含所有实现）
#include "CoralFeature.hpp"
#include "CoralClawFeature.hpp"
#include "CoralMushroomFeature.hpp"
#include "CoralTreeFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/coral/CoralBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <utility>

namespace mc {

// ============================================================================
// 珊瑚辅助函数实现
// ============================================================================

const BlockState* getCoralBlockState(blocks::CoralColor color, bool isDead)
{
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                return VanillaBlocks::DEAD_TUBE_CORAL_BLOCK ? &VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Brain:
                return VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK ? &VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK->defaultState()
                                                             : nullptr;
            case blocks::CoralColor::Bubble:
                return VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK ? &VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK->defaultState()
                                                              : nullptr;
            case blocks::CoralColor::Fire:
                return VanillaBlocks::DEAD_FIRE_CORAL_BLOCK ? &VanillaBlocks::DEAD_FIRE_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Horn:
                return VanillaBlocks::DEAD_HORN_CORAL_BLOCK ? &VanillaBlocks::DEAD_HORN_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            default:
                return nullptr;
        }
    }

    switch (color) {
        case blocks::CoralColor::Tube:
            return VanillaBlocks::TUBE_CORAL_BLOCK ? &VanillaBlocks::TUBE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Brain:
            return VanillaBlocks::BRAIN_CORAL_BLOCK ? &VanillaBlocks::BRAIN_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Bubble:
            return VanillaBlocks::BUBBLE_CORAL_BLOCK ? &VanillaBlocks::BUBBLE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Fire:
            return VanillaBlocks::FIRE_CORAL_BLOCK ? &VanillaBlocks::FIRE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Horn:
            return VanillaBlocks::HORN_CORAL_BLOCK ? &VanillaBlocks::HORN_CORAL_BLOCK->defaultState() : nullptr;
        default:
            return nullptr;
    }
}

const BlockState* getCoralFanState(blocks::CoralColor color, bool isDead)
{
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                return VanillaBlocks::DEAD_TUBE_CORAL_FAN ? &VanillaBlocks::DEAD_TUBE_CORAL_FAN->defaultState()
                                                          : nullptr;
            case blocks::CoralColor::Brain:
                return VanillaBlocks::DEAD_BRAIN_CORAL_FAN ? &VanillaBlocks::DEAD_BRAIN_CORAL_FAN->defaultState()
                                                           : nullptr;
            case blocks::CoralColor::Bubble:
                return VanillaBlocks::DEAD_BUBBLE_CORAL_FAN ? &VanillaBlocks::DEAD_BUBBLE_CORAL_FAN->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Fire:
                return VanillaBlocks::DEAD_FIRE_CORAL_FAN ? &VanillaBlocks::DEAD_FIRE_CORAL_FAN->defaultState()
                                                          : nullptr;
            case blocks::CoralColor::Horn:
                return VanillaBlocks::DEAD_HORN_CORAL_FAN ? &VanillaBlocks::DEAD_HORN_CORAL_FAN->defaultState()
                                                          : nullptr;
            default:
                return nullptr;
        }
    }

    switch (color) {
        case blocks::CoralColor::Tube:
            return VanillaBlocks::TUBE_CORAL_FAN ? &VanillaBlocks::TUBE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Brain:
            return VanillaBlocks::BRAIN_CORAL_FAN ? &VanillaBlocks::BRAIN_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Bubble:
            return VanillaBlocks::BUBBLE_CORAL_FAN ? &VanillaBlocks::BUBBLE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Fire:
            return VanillaBlocks::FIRE_CORAL_FAN ? &VanillaBlocks::FIRE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Horn:
            return VanillaBlocks::HORN_CORAL_FAN ? &VanillaBlocks::HORN_CORAL_FAN->defaultState() : nullptr;
        default:
            return nullptr;
    }
}

const BlockState* getCoralWallFanState(blocks::CoralColor color, Direction facing, bool isDead)
{
    // 墙珊瑚扇的 FACING 只接受水平四向，竖直方向无法贴附
    if (facing == Direction::Up || facing == Direction::Down || facing == Direction::None) {
        return nullptr;
    }

    Block* wallFanBlock = nullptr;
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                wallFanBlock = VanillaBlocks::DEAD_TUBE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Brain:
                wallFanBlock = VanillaBlocks::DEAD_BRAIN_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Bubble:
                wallFanBlock = VanillaBlocks::DEAD_BUBBLE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Fire:
                wallFanBlock = VanillaBlocks::DEAD_FIRE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Horn:
                wallFanBlock = VanillaBlocks::DEAD_HORN_CORAL_WALL_FAN;
                break;
            default:
                wallFanBlock = nullptr;
                break;
        }
    } else {
        switch (color) {
            case blocks::CoralColor::Tube:
                wallFanBlock = VanillaBlocks::TUBE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Brain:
                wallFanBlock = VanillaBlocks::BRAIN_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Bubble:
                wallFanBlock = VanillaBlocks::BUBBLE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Fire:
                wallFanBlock = VanillaBlocks::FIRE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Horn:
                wallFanBlock = VanillaBlocks::HORN_CORAL_WALL_FAN;
                break;
            default:
                wallFanBlock = nullptr;
                break;
        }
    }

    if (wallFanBlock == nullptr) {
        return nullptr;
    }

    // 墙珊瑚扇朝向由状态属性 HORIZONTAL_FACING 表达（面向背离附着面的一侧）
    return &wallFanBlock->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

bool isWaterAt(WorldGenRegion& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER);
}

i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        const BlockState* state = world.getBlockState(x, y, z);
        if (state == nullptr || state->isAir()) {
            continue;
        }

        if (VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER)) {
            continue;
        }

        return y;
    }

    return -1;
}

bool placeCoralBase(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, bool isDead)
{
    // TODO: 放置前置条件与原版不一致：原版要求「当前位置为水或已是珊瑚（CORALS 方块标签）」，
    // 且「正上方必须为水」；此处仅校验当前位置为水，故珊瑚可能贴着非水格的方块向上生长。
    const BlockState* coralState = getCoralBlockState(color, isDead);
    if (coralState == nullptr || !isWaterAt(world, pos)) {
        return false;
    }

    world.setBlockState(pos, coralState);
    return true;
}

void placeCoralDecorations(WorldGenRegion& world,
    math::IRandom& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations)
{
    if (!includeDecorations) {
        return;
    }

    const BlockPos topPos(pos.x, pos.y + 1, pos.z);
    if (isWaterAt(world, topPos)) {
        // TODO: 顶部装饰的掷骰结构与原版不一致：原版为「先掷 0.25 放置随机珊瑚植物/扇，
        // 否则再掷 0.05 放置海泡菜」两分支互斥；此处把海泡菜掷骰嵌在 0.25 分支内部，
        // 导致海泡菜生成概率显著偏低。同时原版从 CORALS 方块标签随机取装饰方块，
        // 此处固定使用配置颜色对应的珊瑚扇，待方块标签基建可用后再收敛。
        if (random.nextFloat() < 0.25f) {
            if (VanillaBlocks::SEA_PICKLE != nullptr && random.nextFloat() < 0.05f) {
                const i32 pickleCount = random.nextInt(4) + 1;
                const BlockState* pickleState =
                    &VanillaBlocks::SEA_PICKLE->defaultState().with(BlockStateProperties::PICKLES_1_4(), pickleCount);
                world.setBlockState(topPos, pickleState);
            } else if (const BlockState* fanState = getCoralFanState(color, isDead); fanState != nullptr) {
                world.setBlockState(topPos, fanState);
            }
        }
    }

    const auto horizontalDirections = Directions::horizontal();
    for (Direction horizontal : horizontalDirections) {
        if (random.nextFloat() >= 0.20f) {
            continue;
        }

        const BlockPos sidePos = pos.offset(horizontal);
        if (!isWaterAt(world, sidePos)) {
            continue;
        }

        if (const BlockState* wallFanState = getCoralWallFanState(color, horizontal, isDead); wallFanState != nullptr) {
            world.setBlockState(sidePos, wallFanState);
        }
    }
}

bool placeCoralWithDecorations(WorldGenRegion& world,
    math::IRandom& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations)
{
    if (!placeCoralBase(world, pos, color, isDead)) {
        return false;
    }

    placeCoralDecorations(world, random, pos, color, isDead, includeDecorations);
    return true;
}

// ============================================================================
// CoralFeature 实现
// ============================================================================

bool CoralFeature::place(
    WorldGenRegion& world, math::IRandom& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    const i32 placeX = pos.x + random.nextInt(world::CHUNK_WIDTH);
    const i32 placeZ = pos.z + random.nextInt(world::CHUNK_WIDTH);
    const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
    if (oceanFloorY <= 0) {
        return false;
    }

    const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);

    if (!_canPlaceAt(world, placePos)) {
        return false;
    }

    bool placed = false;
    switch (random.nextInt(3)) {
        case 0: {
            CoralTreeFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
        case 1: {
            CoralMushroomFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
        default: {
            CoralClawFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
    }

    if (!placed) {
        placed = placeCoralWithDecorations(world, random, placePos, config.color, config.isDead, config.includeWallFan);
    }

    return placed;
}

bool CoralFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    if (!_isWater(world, pos)) {
        return false;
    }

    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (!belowState) {
        return false;
    }

    return belowState->owner().isSolid(*belowState);
}

bool CoralFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

// TODO: 当前无调用方（形状放置统一走 placeCoralWithDecorations 的公共辅助函数），
// 保留待珊瑚特征细分形状逻辑接入后再启用或删除。
void CoralFeature::_placeCoralBlock(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color) const
{
    [[maybe_unused]] const bool placed = placeCoralBase(world, pos, color, false);
}

// TODO: 当前无调用方，理由同 _placeCoralBlock。
void CoralFeature::_placeCoralFan(
    WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, Direction direction) const
{
    if (!isWaterAt(world, pos)) {
        return;
    }

    if (direction == Direction::Up) {
        if (const BlockState* fanState = getCoralFanState(color, false); fanState != nullptr) {
            world.setBlockState(pos, fanState);
        }
        return;
    }

    if (const BlockState* wallFanState = getCoralWallFanState(color, direction, false); wallFanState != nullptr) {
        world.setBlockState(pos, wallFanState);
    }
}

// ============================================================================
// ConfiguredCoralFeature 实现
// ============================================================================

ConfiguredCoralFeature::ConfiguredCoralFeature(std::unique_ptr<CoralFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredCoralFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::IRandom& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
