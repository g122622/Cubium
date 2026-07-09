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

#include "common/world/fluid/FlowingFluid.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/ILiquidContainer.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace mc {
namespace fluid {

namespace {

constexpr f32 FACE_EPSILON = 0.0001f;

struct FaceRectangle {
    f32 uMin;
    f32 uMax;
    f32 vMin;
    f32 vMax;
};

[[nodiscard]] bool approximatelyEqual(f32 lhs, f32 rhs)
{
    return std::fabs(lhs - rhs) <= FACE_EPSILON;
}

// 取空流体状态。MC 的 LevelAccessor#getFluidState 契约保证永不返回 null（无流体处返回 EMPTY
// 的 defaultFluidState）；项目 IWorld::getFluidState 返回指针，存档/边界/未初始化路径可能为 null。
// 流动计算按值取目标流体状态 *getFluidState(...)，若直接解引用 null 会拷贝出 m_owner==nullptr 的
// 野 FluidState，随后 canDisplace 的虚函数派发在 0x8 处崩溃。此处用 EMPTY 兜底，对齐 MC 非空语义。
[[nodiscard]] const FluidState& emptyFluidState()
{
    // EMPTY 流体（fluidId=0）在 FluidRegistry 构造时最先注册，其唯一状态即默认状态。
    // calculateCorrectFlowingState 亦用同一路径取空状态。
    return FluidRegistry::instance().getFluid(FluidRegistry::EMPTY_ID)->defaultState();
}

[[nodiscard]] bool hasSuffix(const std::string& value, const char* suffix)
{
    const size_t suffixLength = std::strlen(suffix);
    return value.size() >= suffixLength && value.compare(value.size() - suffixLength, suffixLength, suffix) == 0;
}

void addFaceRectangles(std::vector<FaceRectangle>& rectangles, const CollisionShape& shape, Direction direction)
{
    if (shape.isEmpty()) {
        return;
    }

    for (const auto& box : shape.boxes()) {
        switch (direction) {
            case Direction::Up:
                if (box.maxY >= 1.0f - FACE_EPSILON) {
                    rectangles.push_back({box.minX, box.maxX, box.minZ, box.maxZ});
                }
                break;
            case Direction::Down:
                if (box.minY <= FACE_EPSILON) {
                    rectangles.push_back({box.minX, box.maxX, box.minZ, box.maxZ});
                }
                break;
            case Direction::North:
                if (box.minZ <= FACE_EPSILON) {
                    rectangles.push_back({box.minX, box.maxX, box.minY, box.maxY});
                }
                break;
            case Direction::South:
                if (box.maxZ >= 1.0f - FACE_EPSILON) {
                    rectangles.push_back({box.minX, box.maxX, box.minY, box.maxY});
                }
                break;
            case Direction::West:
                if (box.minX <= FACE_EPSILON) {
                    rectangles.push_back({box.minZ, box.maxZ, box.minY, box.maxY});
                }
                break;
            case Direction::East:
                if (box.maxX >= 1.0f - FACE_EPSILON) {
                    rectangles.push_back({box.minZ, box.maxZ, box.minY, box.maxY});
                }
                break;
            case Direction::None:
                break;
        }
    }
}

[[nodiscard]] bool intervalsCoverUnit(std::vector<std::pair<f32, f32>> intervals)
{
    if (intervals.empty()) {
        return false;
    }

    std::sort(intervals.begin(), intervals.end(), [](const auto& lhs, const auto& rhs) {
        if (approximatelyEqual(lhs.first, rhs.first)) {
            return lhs.second < rhs.second;
        }
        return lhs.first < rhs.first;
    });

    f32 coveredMax = 0.0f;
    for (const auto& interval : intervals) {
        if (interval.second <= interval.first + FACE_EPSILON) {
            continue;
        }

        if (interval.first > coveredMax + FACE_EPSILON) {
            return false;
        }

        if (interval.second > coveredMax) {
            coveredMax = interval.second;
            if (coveredMax >= 1.0f - FACE_EPSILON) {
                return true;
            }
        }
    }

    return coveredMax >= 1.0f - FACE_EPSILON;
}

[[nodiscard]] bool facesFillSquare(
    const CollisionShape& firstShape, const CollisionShape& secondShape, Direction direction)
{
    std::vector<FaceRectangle> rectangles;
    rectangles.reserve(firstShape.boxCount() + secondShape.boxCount());
    addFaceRectangles(rectangles, firstShape, direction);
    addFaceRectangles(rectangles, secondShape, Directions::opposite(direction));

    if (rectangles.empty()) {
        return false;
    }

    std::vector<f32> uCoordinates;
    uCoordinates.reserve(rectangles.size() * 2 + 2);
    uCoordinates.push_back(0.0f);
    uCoordinates.push_back(1.0f);

    for (const auto& rectangle : rectangles) {
        uCoordinates.push_back(rectangle.uMin);
        uCoordinates.push_back(rectangle.uMax);
    }

    std::sort(uCoordinates.begin(), uCoordinates.end());
    std::vector<f32> uniqueUCoordinates;
    uniqueUCoordinates.reserve(uCoordinates.size());
    for (f32 value : uCoordinates) {
        if (uniqueUCoordinates.empty() || !approximatelyEqual(uniqueUCoordinates.back(), value)) {
            uniqueUCoordinates.push_back(value);
        }
    }

    if (uniqueUCoordinates.size() < 2) {
        return false;
    }

    for (size_t index = 0; index + 1 < uniqueUCoordinates.size(); ++index) {
        const f32 left = uniqueUCoordinates[index];
        const f32 right = uniqueUCoordinates[index + 1];
        if (right <= left + FACE_EPSILON) {
            continue;
        }

        const f32 mid = (left + right) * 0.5f;
        std::vector<std::pair<f32, f32>> intervals;
        for (const auto& rectangle : rectangles) {
            if (mid >= rectangle.uMin - FACE_EPSILON && mid <= rectangle.uMax + FACE_EPSILON) {
                intervals.emplace_back(rectangle.vMin, rectangle.vMax);
            }
        }

        if (!intervalsCoverUnit(std::move(intervals))) {
            return false;
        }
    }

    return true;
}

} // namespace

// ============================================================================
// FlowingFluid 实现
// ============================================================================

FluidState FlowingFluid::getFlowingState(i32 level, bool falling) const
{
    // level范围是1-SOURCE_LEVEL
    level = std::clamp(level, 1, SOURCE_LEVEL);
    const FluidState& state = const_cast<FlowingFluid*>(this)->getFlowing().defaultState();
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto& fallingProp = FluidProperties::FALLING();
    return state.with(levelProp, level).with(fallingProp, falling);
}

FluidState FlowingFluid::getStillState(bool falling) const
{
    // 源头level=8，但isSource=true
    const FluidState& state = const_cast<FlowingFluid*>(this)->getStill().defaultState();
    auto& fallingProp = FluidProperties::FALLING();
    if (falling) {
        return state.with(fallingProp, true);
    }
    return state;
}

void FlowingFluid::tick(IWorld& world, const BlockPos& pos, FluidState& state)
{
    MC_TRACE_EVENT("fluid.tick",
        "FlowingFluid::tick",
        "position",
        pos.toString(),
        "fluidState",
        state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    if (!state.isSource()) {
        const BlockState* currentBlock = world.getBlockState(pos);
        FluidState correctState = calculateCorrectFlowingState(world, pos, currentBlock);
        const i32 tickDelay = getTickDelay(world, pos, state, correctState);

        if (correctState.isEmpty()) {
            state = correctState;
            if (VanillaBlocks::AIR != nullptr) {
                world.setBlockState(pos, &VanillaBlocks::AIR->defaultState(), 3);
            }
        } else if (!(correctState == state)) {
            state = correctState;
            const BlockState* newBlockState = correctState.getBlockState();
            if (newBlockState != nullptr) {
                world.setBlockState(pos, newBlockState, 2);
            }
            world.tickManager().scheduleFluidTick(pos, correctState.getFluid(), tickDelay);
        }
    }

    flowAround(world, pos, state);
}

Vector3 FlowingFluid::getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const
{
    f32 flowX = 0.0f;
    f32 flowZ = 0.0f;
    BlockPos samplePos;

    for (Direction dir : Directions::horizontal()) {
        samplePos = pos.offset(Directions::toBlockFace(dir));
        const FluidState* neighborFluid = world.getFluidState(samplePos);
        if (neighborFluid == nullptr || !isSameOrEmpty(*neighborFluid)) {
            continue;
        }

        f32 heightDelta = 0.0f;
        f32 neighborHeight = neighborFluid->getHeight();
        if (neighborHeight == 0.0f) {
            const BlockState* neighborBlock = world.getBlockState(samplePos);
            if (neighborBlock != nullptr && neighborBlock->owner().material().blocksMovement()) {
                continue;
            }

            BlockPos belowPos = samplePos.down();
            const FluidState* belowFluid = world.getFluidState(belowPos);
            if (belowFluid != nullptr && isSameOrEmpty(*belowFluid)) {
                neighborHeight = belowFluid->getHeight();
                if (neighborHeight > 0.0f) {
                    // SOURCE_LEVEL/MAX_AMOUNT ≈ 0.889，表示下落流体的额外高度偏移
                    heightDelta = state.getHeight() -
                        (neighborHeight - static_cast<f32>(SOURCE_LEVEL) / static_cast<f32>(MAX_AMOUNT));
                }
            }
        } else {
            heightDelta = state.getHeight() - neighborHeight;
        }

        if (heightDelta != 0.0f) {
            flowX += static_cast<f32>(Directions::xOffset(dir)) * heightDelta;
            flowZ += static_cast<f32>(Directions::zOffset(dir)) * heightDelta;
        }
    }

    Vector3 flow(flowX, 0.0f, flowZ);
    if (state.isFalling()) {
        for (Direction dir : Directions::horizontal()) {
            samplePos = pos.offset(Directions::toBlockFace(dir));
            if (causesDownwardCurrent(world, samplePos, dir) || causesDownwardCurrent(world, samplePos.up(), dir)) {
                // 6.0f 是下落流体的向下流动强度系数
                flow = flow.normalized() + Vector3(0.0f, -6.0f, 0.0f);
                break;
            }
        }
    }

    return flow.normalized();
}

bool FlowingFluid::causesDownwardCurrent(IBlockReader& world, const BlockPos& pos, Direction side) const
{
    const BlockState* blockState = world.getBlockState(pos);
    const FluidState* fluidState = world.getFluidState(pos);

    if (fluidState != nullptr && fluidState->getFluid().isEquivalentTo(*this)) {
        return false;
    }

    if (side == Direction::Up) {
        return true;
    }

    if (blockState == nullptr) {
        return false;
    }

    const Material& material = blockState->owner().material();
    if (material == Material::ICE) {
        return false;
    }

    return blockState->isSolidSide(world, pos, side);
}

CollisionShape FlowingFluid::getShape(const FluidState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 检查是否为满高度
    if (state.getLevel() == SOURCE_LEVEL && isFullHeight(state, world, pos)) {
        return CollisionShape::fullBlock();
    }

    // 根据实际高度创建碰撞箱
    // IBlockReader 继承自 IWorld，可以安全转换
    IWorld& worldRef = static_cast<IWorld&>(world);
    f32 height = state.getActualHeight(worldRef, pos);
    return CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, height, 1.0f);
}

bool FlowingFluid::isFullHeight(const FluidState& state, IBlockReader& world, const BlockPos& pos) const
{
    BlockPos above = pos.up();
    const FluidState* aboveFluid = world.getFluidState(above);
    return aboveFluid != nullptr && aboveFluid->getFluid().isEquivalentTo(*this);
}

i32 FlowingFluid::getTickDelay(
    IWorld& world, const BlockPos& pos, const FluidState& state, const FluidState& correctState) const
{
    (void)pos;
    (void)state;
    (void)correctState;
    return Fluid::getTickDelay(world);
}

void FlowingFluid::flowAround(IWorld& world, const BlockPos& pos, const FluidState& state)
{
    MC_TRACE_EVENT("fluid.tick",
        "FlowingFluid::flowAround",
        "position",
        pos.toString(),
        "fluidState",
        state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    if (state.isEmpty()) {
        return;
    }

    const BlockState* currentBlock = world.getBlockState(pos);
    const BlockPos belowPos = pos.down();
    const BlockState* belowBlock = world.getBlockState(belowPos);
    FluidState belowFlowState = calculateCorrectFlowingState(world, belowPos, belowBlock);
    const FluidState* belowFluidPtr = world.getFluidState(belowPos);
    const FluidState& belowFluid = belowFluidPtr != nullptr ? *belowFluidPtr : emptyFluidState();

    if (canFlow(
            world, pos, currentBlock, Direction::Down, belowPos, belowBlock, belowFluid, belowFlowState.getFluid())) {
        flowInto(world, belowPos, belowBlock, Direction::Down, belowFlowState);
        if (getHorizontalSourceCount(world, pos) >= 3) {
            spreadHorizontally(world, pos, state, currentBlock);
        }
    } else if (state.isSource() ||
        !canFlowDown(world, belowFlowState.getFluid(), pos, currentBlock, belowPos, belowBlock)) {
        spreadHorizontally(world, pos, state, currentBlock);
    }
}

void FlowingFluid::spreadHorizontally(
    IWorld& world, const BlockPos& pos, const FluidState& state, const BlockState* blockState)
{
    MC_TRACE_EVENT("fluid.tick",
        "FlowingFluid::spreadHorizontally",
        "position",
        pos.toString(),
        "fluidState",
        state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    i32 spreadLevel = state.isFalling() ? 7 : state.getLevel() - getLevelDecrease(world);
    if (spreadLevel <= 0) {
        return;
    }

    auto flowDirections = getFlowDirections(world, pos, blockState);

    for (const auto& [dir, fluidState] : flowDirections) {
        BlockPos targetPos = pos.offset(Directions::toBlockFace(dir));
        const BlockState* targetBlock = world.getBlockState(targetPos);
        if (targetBlock == nullptr) {
            continue;
        }
        const FluidState* targetFluidPtr = world.getFluidState(targetPos);
        const FluidState& targetFluid = targetFluidPtr != nullptr ? *targetFluidPtr : emptyFluidState();
        if (canFlow(world, pos, blockState, dir, targetPos, targetBlock, targetFluid, fluidState.getFluid())) {
            flowInto(world, targetPos, targetBlock, dir, fluidState);
        }
    }
}

void FlowingFluid::flowInto(
    IWorld& world, const BlockPos& pos, const BlockState* blockState, Direction dir, const FluidState& state)
{
    MC_TRACE_EVENT("fluid.tick",
        "FlowingFluid::flowInto",
        "position",
        pos.toString(),
        "direction",
        Directions::toString(dir),
        "fluidState",
        state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    if (blockState != nullptr) {
        Block* blockRef = Block::getBlock(blockState->blockId());

        // 支持 ILiquidContainer（如大锅）优先接收流体，不直接替换为液体方块。
        if (blockRef != nullptr) {
            if (auto* container = dynamic_cast<ILiquidContainer*>(blockRef)) {
                if (container->canContainFluid(world, pos, *blockState, state.getFluid()) &&
                    container->receiveFluid(world, pos, *blockState, state)) {
                    return;
                }
            }
        }

        if (!blockState->isAir()) {
            beforeReplacingBlock(world, pos, blockState);
        }
    }

    // 设置流体方块
    // 必须使用传入状态自身的流体类型做方块映射，避免 source/fluid 实例错配。
    // 使用 flags=3 来通知邻居和更新客户端
    const BlockState* newBlockState = state.getBlockState();
    if (newBlockState != nullptr) {
        world.setBlockState(pos, newBlockState, 3);
    }
}

FluidState FlowingFluid::calculateCorrectFlowingState(
    IWorld& world, const BlockPos& pos, const BlockState* blockState) const
{
    if (blockState == nullptr) {
        return *Fluid::getFluidState(FluidRegistry::EMPTY_ID);
    }

    i32 maxLevel = 0;
    i32 sourceCount = 0;

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const BlockState* neighborBlock = world.getBlockState(neighborPos);
        if (neighborBlock == nullptr) {
            continue;
        }

        const FluidState* neighborFluid = neighborBlock->getFluidState();
        if (neighborFluid == nullptr || !neighborFluid->getFluid().isEquivalentTo(*this) ||
            !doesSideHaveHoles(dir, world, pos, blockState, neighborPos, neighborBlock)) {
            continue;
        }

        if (neighborFluid->isSource()) {
            ++sourceCount;
        }

        maxLevel = std::max(maxLevel, neighborFluid->getLevel());
    }

    if (sourceCount >= 2 && canSourcesMultiply()) {
        BlockPos belowPos = pos.down();
        const BlockState* belowBlock = world.getBlockState(belowPos);
        if (belowBlock != nullptr) {
            const FluidState* belowFluid = belowBlock->getFluidState();
            if (belowBlock->owner().material().isSolid() || (belowFluid != nullptr && isSameSource(*belowFluid))) {
                return getStillState(false);
            }
        }
    }

    BlockPos abovePos = pos.up();
    const BlockState* aboveBlock = world.getBlockState(abovePos);
    if (aboveBlock != nullptr) {
        const FluidState* aboveFluid = aboveBlock->getFluidState();
        if (aboveFluid != nullptr && !aboveFluid->isEmpty() && aboveFluid->getFluid().isEquivalentTo(*this) &&
            doesSideHaveHoles(Direction::Up, world, pos, blockState, abovePos, aboveBlock)) {
            return getFlowingState(SOURCE_LEVEL, true);
        }
    }

    i32 nextLevel = maxLevel - getLevelDecrease(world);
    return nextLevel <= 0 ? *Fluid::getFluidState(FluidRegistry::EMPTY_ID) : getFlowingState(nextLevel, false);
}

bool FlowingFluid::canFlow(IWorld& world,
    const BlockPos& fromPos,
    const BlockState* fromBlock,
    Direction dir,
    const BlockPos& toPos,
    const BlockState* toBlock,
    const FluidState& toFluid,
    const Fluid& fluid) const
{
    // 检查目标流体是否可以被替换
    if (!toFluid.canDisplace(world, toPos, fluid, dir)) {
        return false;
    }

    // 检查侧面是否有孔洞
    if (!doesSideHaveHoles(dir, world, fromPos, fromBlock, toPos, toBlock)) {
        return false;
    }

    // 检查目标方块是否可被流体替换。
    // isBlocked() 返回 true 表示方块阻挡流体（不可被替换），因此"可流动"需取反。
    // 对应 MC Java FlowingFluid#canMaybePassThrough / canHoldAnyFluid：方块能容纳流体才允许流入。
    return !isBlocked(world, toPos, toBlock, fluid);
}

bool FlowingFluid::doesSideHaveHoles(Direction dir,
    IWorld& world,
    const BlockPos& pos,
    const BlockState* state,
    const BlockPos& neighborPos,
    const BlockState* neighborState) const
{
    if (state == nullptr || neighborState == nullptr) {
        return true;
    }

    const CollisionShape& fromShape = state->getCollisionShape();
    const CollisionShape& toShape = neighborState->getCollisionShape();

    if (fromShape.isEmpty() || toShape.isEmpty()) {
        return true;
    }

    return !facesFillSquare(fromShape, toShape, dir);
}

bool FlowingFluid::canFormSource(IWorld& world, const BlockPos& pos)
{
    if (!canSourcesMultiply()) {
        return false;
    }

    i32 sourceCount = getHorizontalSourceCount(world, pos);
    if (sourceCount < 2) {
        return false;
    }

    // 检查下方
    BlockPos below = pos.down();
    const BlockState* belowBlock = world.getBlockState(below);
    if (belowBlock == nullptr) {
        return false;
    }

    // 下方必须是固体或同种流体源头
    if (belowBlock->isSolid()) {
        return true;
    }

    const FluidState* belowFluid = world.getFluidState(below);
    return belowFluid != nullptr && isSameSource(*belowFluid);
}

i32 FlowingFluid::getHorizontalSourceCount(IWorld& world, const BlockPos& pos) const
{
    i32 count = 0;

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const FluidState* neighborFluid = world.getFluidState(neighborPos);

        if (neighborFluid != nullptr && isSameSource(*neighborFluid)) {
            count++;
        }
    }

    return count;
}

bool FlowingFluid::isBlocked(IWorld& world, const BlockPos& pos, const BlockState* block, const Fluid& fluid) const
{
    if (block == nullptr) {
        return false;
    }

    const Block& blockRef = block->owner();

    // ILiquidContainer 方块按自身规则接收流体
    if (auto* container = dynamic_cast<const ILiquidContainer*>(&blockRef)) {
        return !container->canContainFluid(world, pos, *block, fluid);
    }

    // MC Java 的 canHoldAnyFluid 黑名单排除：
    // 这些方块的 canBeReplacedByFluid() 返回 true（因为 isSolid=false），
    // 但 MC Java 明确禁止流体流入这些方块
    const std::string& path = blockRef.blockLocation().path();
    if (hasSuffix(path, "_door") || hasSuffix(path, "_sign") || path == "ladder" || path == "sugar_cane" ||
        path == "bubble_column") {
        return true;
    }

    const Material& material = blockRef.material();

    // 传送门和结构空位不可被流体替换
    if (material == Material::PORTAL || material == Material::STRUCTURE_VOID) {
        return true;
    }

    // 使用 canBeReplacedByFluid() 判断方块是否可被流体替换
    // 对应 MC Java 的 BlockBehaviour.canBeReplaced(BlockState, Fluid)：canBeReplaced() || !isSolid()
    (void)world;
    (void)pos;
    (void)fluid;
    return !block->canBeReplacedByFluid();
}

bool FlowingFluid::isSameOrEmpty(const FluidState& state) const
{
    return state.isEmpty() || isEquivalentTo(state.getFluid());
}

bool FlowingFluid::isSameSource(const FluidState& state) const
{
    return isEquivalentTo(state.getFluid()) && state.isSource();
}

i32 FlowingFluid::calculateFlowDecay(IWorld& world,
    const BlockPos& pos,
    i32 decay,
    Direction excludeDir,
    const BlockState* blockState,
    const BlockPos& sourcePos,
    std::unordered_map<i16, std::pair<const BlockState*, const FluidState*>>& stateCache,
    std::unordered_map<i16, bool>& fallCache) const
{
    // 1000 作为初始大值，用于寻找最小衰减值
    i32 minDecay = 1000;

    for (Direction dir : Directions::horizontal()) {
        if (dir == excludeDir) {
            continue;
        }

        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        i16 key = packRelativePos(sourcePos, neighborPos);

        auto it = stateCache.find(key);
        const BlockState* neighborBlock;
        const FluidState* neighborFluid;

        if (it == stateCache.end()) {
            neighborBlock = world.getBlockState(neighborPos);
            if (neighborBlock == nullptr) {
                continue;
            }
            neighborFluid = world.getFluidState(neighborPos);
            if (neighborFluid == nullptr) {
                continue;
            }
            stateCache.emplace(key, std::make_pair(neighborBlock, neighborFluid));
        } else {
            neighborBlock = it->second.first;
            neighborFluid = it->second.second;
        }

        if (canFlowInto(world, pos, blockState, dir, neighborPos, neighborBlock, *neighborFluid, *this)) {
            bool canFall = false;
            auto fallIt = fallCache.find(key);
            if (fallIt == fallCache.end()) {
                BlockPos below = neighborPos.down();
                const BlockState* belowBlock = world.getBlockState(below);
                canFall = canFlowDown(world, *this, neighborPos, neighborBlock, below, belowBlock);
                fallCache.emplace(key, canFall);
            } else {
                canFall = fallIt->second;
            }

            if (canFall) {
                return decay;
            }

            if (decay < getSpreadDistance(world)) {
                i32 neighborDecay = calculateFlowDecay(world,
                    neighborPos,
                    decay + 1,
                    Directions::opposite(dir),
                    neighborBlock,
                    sourcePos,
                    stateCache,
                    fallCache);
                if (neighborDecay < minDecay) {
                    minDecay = neighborDecay;
                }
            }
        }
    }

    return minDecay;
}

bool FlowingFluid::canFlowDown(IWorld& world,
    const Fluid& fluid,
    const BlockPos& pos,
    const BlockState* blockState,
    const BlockPos& belowPos,
    const BlockState* belowBlock) const
{
    if (!doesSideHaveHoles(Direction::Down, world, pos, blockState, belowPos, belowBlock)) {
        return false;
    }

    const FluidState* belowFluid = world.getFluidState(belowPos);
    if (belowFluid != nullptr && belowFluid->getFluid().isEquivalentTo(fluid)) {
        return true;
    }

    // isBlocked() 返回 true 表示方块阻挡流体（不可被替换），因此"可向下流动"需取反。
    // 对应 MC Java FlowingFluid#isWaterHole -> canHoldFluid：方块能容纳流体才视为可向下流动。
    return !isBlocked(world, belowPos, belowBlock, fluid);
}

bool FlowingFluid::canFlowInto(IWorld& world,
    const BlockPos& fromPos,
    const BlockState* fromBlock,
    Direction dir,
    const BlockPos& toPos,
    const BlockState* toBlock,
    const FluidState& toFluid,
    const Fluid& fluidIn) const
{
    if (!isSameSource(toFluid) && doesSideHaveHoles(dir, world, fromPos, fromBlock, toPos, toBlock)) {
        // isBlocked() 返回 true 表示方块阻挡流体（不可被替换），因此"可流入"需取反。
        // 对应 MC Java FlowingFluid#canMaybePassThrough -> canHoldAnyFluid。
        return !isBlocked(world, toPos, toBlock, fluidIn);
    }
    return false;
}

i16 FlowingFluid::packRelativePos(const BlockPos& source, const BlockPos& target) const noexcept
{
    i32 dx = target.x - source.x;
    i32 dz = target.z - source.z;
    // 使用 128 作为偏移量将 [-128, 127] 范围的相对坐标映射到 [0, 255]
    return static_cast<i16>(((dx + 128) & 0xFF) << 8 | ((dz + 128) & 0xFF));
}

std::unordered_map<Direction, FluidState> FlowingFluid::getFlowDirections(
    IWorld& world, const BlockPos& pos, const BlockState* blockState)
{
    std::unordered_map<Direction, FluidState> result;
    // 1000 作为初始大值，用于寻找最小衰减值
    i32 minDecay = 1000;

    std::unordered_map<i16, std::pair<const BlockState*, const FluidState*>> stateCache;
    std::unordered_map<i16, bool> fallCache;

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        i16 key = packRelativePos(pos, neighborPos);

        const BlockState* neighborBlock = world.getBlockState(neighborPos);
        if (neighborBlock == nullptr) {
            continue;
        }
        const FluidState* neighborFluid = world.getFluidState(neighborPos);
        if (neighborFluid == nullptr) {
            continue;
        }

        stateCache.emplace(key, std::make_pair(neighborBlock, neighborFluid));

        FluidState targetState = calculateCorrectFlowingState(world, neighborPos, neighborBlock);

        if (canFlowInto(
                world, pos, blockState, dir, neighborPos, neighborBlock, *neighborFluid, targetState.getFluid())) {
            BlockPos below = neighborPos.down();
            const BlockState* belowBlock = world.getBlockState(below);
            bool canFall = canFlowDown(world, *this, neighborPos, neighborBlock, below, belowBlock);
            fallCache.emplace(key, canFall);

            i32 decay;
            if (canFall) {
                decay = 0;
            } else {
                decay = calculateFlowDecay(
                    world, neighborPos, 1, Directions::opposite(dir), neighborBlock, pos, stateCache, fallCache);
            }

            if (decay < minDecay) {
                result.clear();
            }

            if (decay <= minDecay) {
                result.emplace(dir, targetState);
                minDecay = decay;
            }
        }
    }

    return result;
}

} // namespace fluid
} // namespace mc
