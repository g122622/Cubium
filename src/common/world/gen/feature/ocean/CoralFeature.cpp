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
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../WorldConstants.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "CoralClawFeature.hpp"
#include "CoralMushroomFeature.hpp"
#include "CoralTreeFeature.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include <array>
#include <cmath>

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

const BlockState* getCoralWallFanState(blocks::CoralColor color, Direction supportDirection, bool isDead)
{
    if (supportDirection == Direction::Up || supportDirection == Direction::Down ||
        supportDirection == Direction::None) {
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

    return &wallFanBlock->defaultState().with(BlockStateProperties::FACING(), supportDirection);
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
    const BlockState* coralState = getCoralBlockState(color, isDead);
    if (coralState == nullptr || !isWaterAt(world, pos)) {
        return false;
    }

    world.setBlockState(pos, coralState);
    return true;
}

void placeCoralDecorations(WorldGenRegion& world,
    math::Random& random,
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
    math::Random& random,
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
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
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

void CoralFeature::_placeCoralBlock(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color) const
{
    [[maybe_unused]] const bool placed = placeCoralBase(world, pos, color, false);
}

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
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
