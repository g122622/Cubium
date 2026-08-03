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

#include "SpawnPointValidator.hpp"
#include "../../util/property/Properties.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/dimension/DimensionType.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <optional>
#include <utility>

namespace mc {

// ========== 公共方法实现 ==========

SpawnPointValidationResult SpawnPointValidator::validate(
    IWorld& world, const GlobalPos& spawnPoint, bool spawnForced, bool consumeCharge)
{
    (void)consumeCharge; // 避免未使用警告，实际消耗能量需要在重生时执行

    // 1. 获取方块状态
    const BlockPos& pos = spawnPoint.getPos();
    const BlockState* state = world.getBlockState(pos);

    if (state == nullptr || state->isAir()) {
        // 方块不存在或为空气
        // 如果是强制重生点，检查是否可以在该位置生成
        if (spawnForced) {
            if (validateForcedSpawn(world, pos)) {
                return SpawnPointValidationResult::Valid;
            }
            return SpawnPointValidationResult::BlockCannotSpawnIn;
        }
        // 无法判断是床还是重生锚，返回通用错误
        return SpawnPointValidationResult::BedMissing;
    }

    // 2. 根据方块类型执行验证
    if (isBed(*state)) {
        // 床验证
        // 检查维度是否正确（床只在主世界有效）
        DimensionType dimType = DimensionType::fromId(world.dimension());
        if (!dimType.bedWorks()) {
            return SpawnPointValidationResult::BedWrongDimension;
        }

        // 检查床是否有效
        if (!validateBedSpawn(world, pos)) {
            return SpawnPointValidationResult::BedObstructed;
        }

        return SpawnPointValidationResult::Valid;
    }

    if (isRespawnAnchor(*state)) {
        // 重生锚验证
        // 检查维度是否正确（重生锚只在下界有效）
        DimensionType dimType = DimensionType::fromId(world.dimension());
        if (!dimType.respawnAnchorWorks()) {
            return SpawnPointValidationResult::RespawnAnchorWrongDimension;
        }

        // 检查是否有能量
        i32 charges = getRespawnAnchorCharges(*state);
        if (charges <= 0) {
            return SpawnPointValidationResult::RespawnAnchorNoCharge;
        }

        // 检查周围是否有安全位置
        if (!validateRespawnAnchorSpawn(world, pos)) {
            return SpawnPointValidationResult::RespawnAnchorNoSafePosition;
        }

        // 如果需要消耗能量，在这里处理
        // 注意：实际消耗能量需要在重生时执行，这里只验证

        return SpawnPointValidationResult::Valid;
    }

    // 3. 其他方块 - 检查是否为强制重生点
    if (spawnForced) {
        if (validateForcedSpawn(world, pos)) {
            return SpawnPointValidationResult::Valid;
        }
        return SpawnPointValidationResult::BlockCannotSpawnIn;
    }

    // 非床/重生锚方块，且非强制重生点
    return SpawnPointValidationResult::BedMissing;
}

std::optional<Vector3> SpawnPointValidator::findSafeSpawnPosition(
    IWorld& world, const GlobalPos& spawnPoint, bool spawnForced, bool consumeCharge)
{

    const BlockPos& pos = spawnPoint.getPos();
    const BlockState* state = world.getBlockState(pos);

    // 1. 检查床
    if (state != nullptr && isBed(*state)) {
        DimensionType dimType = DimensionType::fromId(world.dimension());
        if (dimType.bedWorks() && validateBedSpawn(world, pos)) {
            return findBedSpawnPosition(world, pos);
        }
    }

    // 2. 检查重生锚
    if (state != nullptr && isRespawnAnchor(*state)) {
        DimensionType dimType = DimensionType::fromId(world.dimension());
        if (dimType.respawnAnchorWorks() && getRespawnAnchorCharges(*state) > 0) {
            return findRespawnAnchorSpawnPosition(world, pos, consumeCharge);
        }
    }

    // 3. 强制重生点
    if (spawnForced) {
        // 对于强制重生点，直接在方块上方生成
        if (_hasStandingSpace(world, pos.up())) {
            return Vector3(static_cast<f64>(pos.x) + 0.5, static_cast<f64>(pos.y) + 0.1, static_cast<f64>(pos.z) + 0.5);
        }
        // 尝试方块内部
        if (validateForcedSpawn(world, pos)) {
            return Vector3(static_cast<f64>(pos.x) + 0.5, static_cast<f64>(pos.y) + 0.1, static_cast<f64>(pos.z) + 0.5);
        }
    }

    return std::nullopt;
}

bool SpawnPointValidator::validateBedSpawn(IWorld& world, const BlockPos& bedPos)
{
    // 检查床是否存在
    const BlockState* state = world.getBlockState(bedPos);
    if (state == nullptr || !isBed(*state)) {
        return false;
    }

    // 获取床的朝向
    Direction facing = _getBedFacing(*state);
    if (facing == Direction::None) {
        return false;
    }

    // 床头位置
    BlockPos headPos = bedPos;
    BlockStateProperties::BedPart part = state->get(BlockStateProperties::BED_PART());

    // 如果是床尾，计算床头位置
    if (part == BlockStateProperties::BedPart::Foot) {
        headPos = bedPos.offset(facing);
    }

    // 检查床头是否有站立空间
    if (!_hasStandingSpace(world, headPos.up())) {
        return false;
    }

    // 检查起床位置（床尾前方）
    Direction footDir = Directions::opposite(facing);
    BlockPos footPos = headPos.offset(footDir);

    // 尝试多个方向找安全位置
    constexpr std::array<std::pair<i32, i32>, 8> offsets = {
        {{0, -1}, {-1, 0}, {0, 1}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

    for (const auto& [dx, dz] : offsets) {
        BlockPos checkPos(headPos.x + dx, headPos.y + 1, headPos.z + dz);
        if (_hasStandingSpace(world, checkPos)) {
            return true;
        }
    }

    // 检查床尾周围
    for (const auto& [dx, dz] : offsets) {
        BlockPos checkPos(footPos.x + dx, footPos.y + 1, footPos.z + dz);
        if (_hasStandingSpace(world, checkPos)) {
            return true;
        }
    }

    // 检查床头正上方作为最后的备选
    return _hasStandingSpace(world, headPos.up());
}

std::optional<Vector3> SpawnPointValidator::findBedSpawnPosition(IWorld& world, const BlockPos& bedPos)
{

    const BlockState* state = world.getBlockState(bedPos);
    if (state == nullptr || !isBed(*state)) {
        return std::nullopt;
    }

    // 获取床的朝向和部分
    Direction facing = _getBedFacing(*state);
    if (facing == Direction::None) {
        return std::nullopt;
    }

    BlockStateProperties::BedPart part = state->get(BlockStateProperties::BED_PART());

    // 计算床头位置
    BlockPos headPos = bedPos;
    if (part == BlockStateProperties::BedPart::Foot) {
        headPos = bedPos.offset(facing);
    }

    // 尝试的方向列表
    constexpr std::array<std::pair<i32, i32>, 8> offsets = {
        {{0, -1}, {-1, 0}, {0, 1}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

    // 先检查床头周围
    for (const auto& [dx, dz] : offsets) {
        BlockPos checkPos(headPos.x + dx, headPos.y + 1, headPos.z + dz);
        if (_hasStandingSpace(world, checkPos)) {
            return Vector3(
                static_cast<f64>(checkPos.x) + 0.5, static_cast<f64>(checkPos.y), static_cast<f64>(checkPos.z) + 0.5);
        }
    }

    // 再检查床尾周围
    Direction footDir = Directions::opposite(facing);
    BlockPos footPos = headPos.offset(footDir);
    for (const auto& [dx, dz] : offsets) {
        BlockPos checkPos(footPos.x + dx, footPos.y + 1, footPos.z + dz);
        if (_hasStandingSpace(world, checkPos)) {
            return Vector3(
                static_cast<f64>(checkPos.x) + 0.5, static_cast<f64>(checkPos.y), static_cast<f64>(checkPos.z) + 0.5);
        }
    }

    // 最后检查床头正上方
    BlockPos aboveHead = headPos.up();
    if (_hasStandingSpace(world, aboveHead)) {
        return Vector3(static_cast<f64>(aboveHead.x) + 0.5,
            static_cast<f64>(aboveHead.y) + 0.1,
            static_cast<f64>(aboveHead.z) + 0.5);
    }

    return std::nullopt;
}

bool SpawnPointValidator::validateRespawnAnchorSpawn(IWorld& world, const BlockPos& anchorPos)
{
    // 检查重生锚是否存在且有能量
    const BlockState* state = world.getBlockState(anchorPos);
    if (state == nullptr || !isRespawnAnchor(*state)) {
        return false;
    }

    i32 charges = getRespawnAnchorCharges(*state);
    if (charges <= 0) {
        return false;
    }

    // 检查周围是否有安全位置
    return findRespawnAnchorSpawnPosition(world, anchorPos, false).has_value();
}

std::optional<Vector3> SpawnPointValidator::findRespawnAnchorSpawnPosition(
    IWorld& world, const BlockPos& anchorPos, bool consumeCharge)
{

    const BlockState* state = world.getBlockState(anchorPos);
    if (state == nullptr || !isRespawnAnchor(*state)) {
        return std::nullopt;
    }

    i32 charges = getRespawnAnchorCharges(*state);
    if (charges <= 0) {
        return std::nullopt;
    }

    // 预定义的位置偏移列表
    // 基础偏移：重生锚周围8个位置
    constexpr std::array<std::pair<i32, i32>, 8> baseOffsets = {
        {{0, -1}, {-1, 0}, {0, 1}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

    // 尝试 Y=0（同一层）、Y=-1（下一层）、Y=1（上一层）
    for (i32 dy = 0; dy <= 1; ++dy) {
        for (const auto& [dx, dz] : baseOffsets) {
            BlockPos checkPos(anchorPos.x + dx, anchorPos.y + dy, anchorPos.z + dz);
            if (_hasStandingSpace(world, checkPos)) {
                // 如果需要消耗能量，在这里处理
                // 注意：实际消耗需要在调用方执行
                (void)consumeCharge; // 避免未使用警告

                return Vector3(static_cast<f64>(checkPos.x) + 0.5,
                    static_cast<f64>(checkPos.y),
                    static_cast<f64>(checkPos.z) + 0.5);
            }
        }
    }

    // 尝试下一层
    for (const auto& [dx, dz] : baseOffsets) {
        BlockPos checkPos(anchorPos.x + dx, anchorPos.y - 1, anchorPos.z + dz);
        if (_hasStandingSpace(world, checkPos)) {
            return Vector3(
                static_cast<f64>(checkPos.x) + 0.5, static_cast<f64>(checkPos.y), static_cast<f64>(checkPos.z) + 0.5);
        }
    }

    // 尝试正上方
    BlockPos aboveAnchor = anchorPos.up();
    if (_hasStandingSpace(world, aboveAnchor)) {
        return Vector3(static_cast<f64>(aboveAnchor.x) + 0.5,
            static_cast<f64>(aboveAnchor.y),
            static_cast<f64>(aboveAnchor.z) + 0.5);
    }

    return std::nullopt;
}

bool SpawnPointValidator::validateForcedSpawn(IWorld& world, const BlockPos& pos)
{
    // 对于强制重生点，检查方块和上方方块是否允许在内部生成

    const BlockState* state = world.getBlockState(pos);
    const BlockState* stateAbove = world.getBlockState(pos.up());

    // 检查当前位置是否可以在内部生成
    bool canSpawnInCurrent = (state == nullptr || state->isAir() || (state->isLiquid() == false && !state->isSolid()));

    // 检查上方位置是否可以在内部生成
    bool canSpawnInAbove =
        (stateAbove == nullptr || stateAbove->isAir() || (stateAbove->isLiquid() == false && !stateAbove->isSolid()));

    return canSpawnInCurrent && canSpawnInAbove;
}

bool SpawnPointValidator::isBed(const BlockState& state)
{
    // 检查是否有床的属性来判断是否为床
    return state.hasProperty(BlockStateProperties::BED_PART());
}

bool SpawnPointValidator::isRespawnAnchor(const BlockState& state)
{
    // 检查是否有充能属性来判断是否为重生锚
    return state.hasProperty(BlockStateProperties::CHARGES_0_4());
}

i32 SpawnPointValidator::getRespawnAnchorCharges(const BlockState& state)
{
    if (!isRespawnAnchor(state)) {
        return 0;
    }
    return state.get(BlockStateProperties::CHARGES_0_4());
}

// ========== 私有方法实现 ==========

bool SpawnPointValidator::_hasStandingSpace(const IWorld& world, const BlockPos& pos)
{
    // 检查 pos 和 pos.up() 是否都是非固体方块
    // 玩家需要两格高的空间

    const BlockState* state1 = world.getBlockState(pos);
    const BlockState* state2 = world.getBlockState(pos.up());

    // 两个方块都必须是空气或非固体
    bool canStand1 = (state1 == nullptr || state1->isAir() || !state1->blocksMovement() || state1->isLiquid());

    bool canStand2 = (state2 == nullptr || state2->isAir() || !state2->blocksMovement() || state2->isLiquid());

    return canStand1 && canStand2;
}

Direction SpawnPointValidator::_getBedFacing(const BlockState& state)
{
    if (!isBed(state)) {
        return Direction::None;
    }

    if (state.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        return state.get(BlockStateProperties::HORIZONTAL_FACING());
    }

    return Direction::None;
}

bool SpawnPointValidator::_isSafeSpawnPosition(const IWorld& world, const BlockPos& pos, bool requireSafe)
{

    const BlockState* state = world.getBlockState(pos);

    // 如果要求安全且当前位置会窒息实体，返回 false
    if (requireSafe && state != nullptr && state->blocksMovement() && !state->isLiquid()) {
        return false;
    }

    // 检查是否有站立空间
    return _hasStandingSpace(world, pos);
}

} // namespace mc
