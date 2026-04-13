#include "FlowingFluid.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../block/ILiquidContainer.hpp"
#include "../block/BlockPos.hpp"
#include "../block/Material.hpp"
#include "../block/VanillaBlocks.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/Direction.hpp"
#include "../../physics/collision/CollisionShape.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <cmath>
#include <algorithm>
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

[[nodiscard]] bool approximatelyEqual(f32 lhs, f32 rhs) {
    return std::fabs(lhs - rhs) <= FACE_EPSILON;
}

[[nodiscard]] bool hasSuffix(const String& value, const char* suffix) {
    const size_t suffixLength = std::strlen(suffix);
    return value.size() >= suffixLength &&
           value.compare(value.size() - suffixLength, suffixLength, suffix) == 0;
}

void addFaceRectangles(std::vector<FaceRectangle>& rectangles,
                       const CollisionShape& shape,
                       Direction direction) {
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

[[nodiscard]] bool intervalsCoverUnit(std::vector<std::pair<f32, f32>> intervals) {
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

[[nodiscard]] bool facesFillSquare(const CollisionShape& firstShape,
                                   const CollisionShape& secondShape,
                                   Direction direction) {
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

FluidState FlowingFluid::getFlowingState(i32 level, bool falling) const {
    // level范围是1-8，但源头是level=8
    level = std::clamp(level, 1, 8);
    const FluidState& state = const_cast<FlowingFluid*>(this)->getFlowing().defaultState();
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto& fallingProp = FluidProperties::FALLING();
    return state.with(levelProp, level).with(fallingProp, falling);
}

FluidState FlowingFluid::getStillState(bool falling) const {
    // 源头level=8，但isSource=true
    const FluidState& state = const_cast<FlowingFluid*>(this)->getStill().defaultState();
    auto& fallingProp = FluidProperties::FALLING();
    if (falling) {
        return state.with(fallingProp, true);
    }
    return state;
}

void FlowingFluid::tick(IWorld& world, const BlockPos& pos, FluidState& state) {
    MC_TRACE_EVENT("fluid.tick", "FlowingFluid::tick",
        "position", pos.toString(),
        "fluidState", state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    // 非源头才需要计算正确状态
    if (!isSource(state)) {
        MC_TRACE_EVENT("fluid.tick", "CalculatingSourceState",
            "position", pos.toString(),
            "currentState", state.toString());

        const BlockState* currentBlock = world.getBlockState(pos.x, pos.y, pos.z);
        FluidState correctState = calculateCorrectFlowingState(world, pos, currentBlock);
        const i32 tickDelay = getTickDelay(world);

        if (correctState.isEmpty()) {
            // 应该消失 - 设置为空气方块
            if (VanillaBlocks::AIR != nullptr) {
                world.setBlock(pos.x, pos.y, pos.z, &VanillaBlocks::AIR->defaultState());
            }
        } else if (!(correctState == state)) {
            // 状态改变
            state = correctState;
            const BlockState* newBlockState = getBlockState(state);
            if (newBlockState != nullptr) {
                world.setBlock(pos.x, pos.y, pos.z, newBlockState);
            }
            world.scheduleFluidTick(pos, *this, tickDelay);
        }

        if (correctState.isEmpty()) {
            return;
        }
    }

    // 执行流动
    flowAround(world, pos, state);
}

Vector3 FlowingFluid::getFlow(IBlockReader& world, const BlockPos& pos,
                               const FluidState& state) const {
    f32 dx = 0.0f;
    f32 dz = 0.0f;

    // 检查四个水平方向
    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const FluidState* neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborFluid != nullptr && isSameOrEmpty(*neighborFluid)) {
            f32 neighborHeight = neighborFluid->getHeight();
            f32 heightDiff = 0.0f;

            if (neighborHeight == 0.0f) {
                // 相邻位置没有流体，检查下方
                if (neighborFluid->isEmpty()) {
                    const BlockState* neighborBlock = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
                    if (neighborBlock == nullptr || !neighborBlock->owner().material().blocksMovement()) {
                        BlockPos below = neighborPos.down();
                        const FluidState* belowFluid = world.getFluidState(below.x, below.y, below.z);
                        if (belowFluid != nullptr && isSameOrEmpty(*belowFluid)) {
                            f32 belowHeight = belowFluid->getHeight();
                            if (belowHeight > 0.0f) {
                                // 流体从上方流下
                                heightDiff = state.getHeight() - (belowHeight - 8.0f / 9.0f);
                            }
                        }
                    }
                }
            } else {
                heightDiff = state.getHeight() - neighborHeight;
            }

            if (heightDiff != 0.0f) {
                dx += static_cast<f32>(Directions::xOffset(dir)) * heightDiff;
                dz += static_cast<f32>(Directions::zOffset(dir)) * heightDiff;
            }
        }
    }

    // 下落流体有额外的向下力
    if (state.isFalling()) {
        for (Direction dir : Directions::horizontal()) {
            BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
            if (causesDownwardCurrent(world, neighborPos, dir) ||
                causesDownwardCurrent(world, neighborPos.up(), dir)) {
                // 归一化后添加向下分量
                f32 len = std::sqrt(dx * dx + dz * dz);
                if (len > 0.0f) {
                    dx /= len;
                    dz /= len;
                }
                return Vector3(dx, -6.0f, dz);
            }
        }
    }

    // 归一化
    f32 len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0f) {
        dx /= len;
        dz /= len;
    }

    return Vector3(dx, 0.0f, dz);
}

bool FlowingFluid::causesDownwardCurrent(IBlockReader& world, const BlockPos& pos, Direction side) const {
    const BlockState* blockState = world.getBlockState(pos.x, pos.y, pos.z);
    const FluidState* fluidState = world.getFluidState(pos.x, pos.y, pos.z);

    if (fluidState != nullptr && fluidState->getFluid().isEquivalentTo(*this)) {
        return false;
    }

    if (side == Direction::Up) {
        return true;
    }

    // 检查方块是否在该侧面为固体
    if (blockState == nullptr) {
        return false;
    }

    // 冰块不产生向下流
    const Material& material = blockState->owner().material();
    if (material == Material::ICE) {
        return false;
    }

    return blockState->isSolid();
}

CollisionShape FlowingFluid::getShape(const FluidState& state, IBlockReader& world,
                                       const BlockPos& pos) const {
    // 检查是否为满高度
    if (state.getLevel() == 8 && isFullHeight(state, world, pos)) {
        return CollisionShape::fullBlock();
    }

    // 根据实际高度创建碰撞箱
    // IBlockReader 继承自 IWorld，可以安全转换
    IWorld& worldRef = static_cast<IWorld&>(world);
    f32 height = state.getActualHeight(worldRef, pos);
    return CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, height, 1.0f);
}

bool FlowingFluid::isFullHeight(const FluidState& state, IBlockReader& world, const BlockPos& pos) const {
    BlockPos above = pos.up();
    const FluidState* aboveFluid = world.getFluidState(above.x, above.y, above.z);
    return aboveFluid != nullptr && aboveFluid->getFluid().isEquivalentTo(*this);
}

void FlowingFluid::flowAround(IWorld& world, const BlockPos& pos, const FluidState& state) {
    MC_TRACE_EVENT("fluid.tick", "FlowingFluid::flowAround",
        "position", pos.toString(),
        "fluidState", state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    if (state.isEmpty()) {
        return;
    }

    const BlockState* currentBlock = world.getBlockState(pos.x, pos.y, pos.z);

    // 1. 优先向下流动
    BlockPos below = pos.down();
    const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
    const FluidState* belowFluid = world.getFluidState(below.x, below.y, below.z);
    FluidState flowingDownState = calculateCorrectFlowingState(world, below, belowBlock);

    // 检查是否可以向下流动
    if (belowFluid != nullptr && canFlow(world, pos, currentBlock, Direction::Down, below, belowBlock, *belowFluid, flowingDownState.getFluid())) {
        flowInto(world, below, belowBlock, Direction::Down, flowingDownState);

        // 向下流动后检查是否需要水平流动
        if (getHorizontalSourceCount(world, pos) >= 3) {
            spreadHorizontally(world, pos, state, currentBlock);
        }
        return;
    }

    // 2. 不能向下流动时，只有源头或下方不能继续下沉才进行水平扩散
    if (state.isSource() || !canFlowDown(world, flowingDownState.getFluid(), pos, currentBlock, below, belowBlock)) {
        spreadHorizontally(world, pos, state, currentBlock);
    }
}

void FlowingFluid::spreadHorizontally(IWorld& world, const BlockPos& pos,
                                        const FluidState& state, const BlockState* blockState) {
    MC_TRACE_EVENT("fluid.tick", "FlowingFluid::spreadHorizontally",
        "position", pos.toString(),
        "fluidState", state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });
    
    i32 newLevel = isSource(state) ? getSpreadDistance(world) : state.getLevel() - getLevelDecrease(world);

    // 下落流体的level固定为7
    if (state.isFalling()) {
        newLevel = 7;
    }

    if (newLevel <= 0) {
        return;
    }

    // 计算流向方向
    auto flowDirections = getFlowDirections(world, pos, blockState);

    for (const auto& [dir, fluidState] : flowDirections) {
        BlockPos targetPos = pos.offset(Directions::toBlockFace(dir));
        const BlockState* targetBlock = world.getBlockState(targetPos.x, targetPos.y, targetPos.z);
        if (targetBlock != nullptr) {
            flowInto(world, targetPos, targetBlock, dir, fluidState);
        }
    }
}

void FlowingFluid::flowInto(IWorld& world, const BlockPos& pos, const BlockState* blockState,
                             Direction dir, const FluidState& state) {
    MC_TRACE_EVENT("fluid.tick", "FlowingFluid::flowInto",
        "position", pos.toString(),
        "direction", Directions::toString(dir),
        "fluidState", state.toString(),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    if (blockState != nullptr) {
        Block* blockRef = Block::getBlock(blockState->blockId());

        // 支持 ILiquidContainer（如大锅）优先接收流体，不直接替换为液体方块。
        if (blockRef != nullptr) {
            if (auto* container = dynamic_cast<ILiquidContainer*>(blockRef)) {
                if (container->canContainFluid(world, pos, *blockState, state.getFluid()) &&
                    container->receiveFluid(world, pos, blockState, state)) {
                    return;
                }
            }
        }

        if (!blockState->isAir()) {
            beforeReplacingBlock(world, pos, blockState);
        }
    }

    // 设置流体方块
    const BlockState* newBlockState = getBlockState(state);
    if (newBlockState != nullptr) {
        world.setBlock(pos.x, pos.y, pos.z, newBlockState);
    }
}

FluidState FlowingFluid::calculateCorrectFlowingState(IWorld& world,
                                                       const BlockPos& pos,
                                                       const BlockState* blockState) const {
    i32 maxLevel = 0;
    i32 sourceCount = 0;

    // 检查四个水平方向
    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const BlockState* neighborBlock = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        const FluidState* neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborFluid != nullptr &&
            neighborFluid->getFluid().isEquivalentTo(*this) &&
            doesSideHaveHoles(dir, world, pos, blockState, neighborPos, neighborBlock)) {

            if (neighborFluid->isSource()) {
                sourceCount++;
            }
            maxLevel = std::max(maxLevel, neighborFluid->getLevel());
        }
    }

    // 检查是否可以形成源头（水可以，岩浆不可以）
    if (sourceCount >= 2 && canSourcesMultiply()) {
        BlockPos below = pos.down();
        const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
        const FluidState* belowFluid = world.getFluidState(below.x, below.y, below.z);

        // 下方必须是固体或同种流体源头
        if ((belowBlock != nullptr && belowBlock->isSolid()) ||
            (belowFluid != nullptr && isSameSource(*belowFluid))) {
            return getStillState(false);
        }
    }

    // 检查上方是否有下落流体
    BlockPos above = pos.up();
    const BlockState* aboveBlock = world.getBlockState(above.x, above.y, above.z);
    const FluidState* aboveFluid = world.getFluidState(above.x, above.y, above.z);

    if (aboveFluid != nullptr &&
        !aboveFluid->isEmpty() &&
        aboveFluid->getFluid().isEquivalentTo(*this) &&
        doesSideHaveHoles(Direction::Up, world, pos, blockState, above, aboveBlock)) {
        return getFlowingState(8, true);  // 下落流体
    }

    // 计算新等级
    i32 newLevel = maxLevel - getLevelDecrease(world);
    if (newLevel <= 0) {
        // 返回空流体
        return Fluid::getFluid(0)->defaultState();  // EmptyFluid
    }

    return getFlowingState(newLevel, false);
}

bool FlowingFluid::canFlow(IWorld& world, const BlockPos& fromPos,
                            const BlockState* fromBlock, Direction dir,
                            const BlockPos& toPos, const BlockState* toBlock,
                            const FluidState& toFluid, const Fluid& fluid) const {
    // 检查目标流体是否可以被替换
    if (!toFluid.canDisplace(world, toPos, fluid, dir)) {
        return false;
    }

    // 检查侧面是否有孔洞
    if (!doesSideHaveHoles(dir, world, fromPos, fromBlock, toPos, toBlock)) {
        return false;
    }

    // 检查目标方块是否可被流体替换
    return isBlocked(world, toPos, toBlock, fluid);
}

bool FlowingFluid::doesSideHaveHoles(Direction dir, IWorld& world,
                                      const BlockPos& pos, const BlockState* state,
                                      const BlockPos& neighborPos,
                                      const BlockState* neighborState) const {
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

bool FlowingFluid::canFormSource(IWorld& world, const BlockPos& pos) {
    if (!canSourcesMultiply()) {
        return false;
    }

    i32 sourceCount = getHorizontalSourceCount(world, pos);
    if (sourceCount < 2) {
        return false;
    }

    // 检查下方
    BlockPos below = pos.down();
    const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
    if (belowBlock == nullptr) {
        return false;
    }

    // 下方必须是固体或同种流体源头
    if (belowBlock->isSolid()) {
        return true;
    }

    const FluidState* belowFluid = world.getFluidState(below.x, below.y, below.z);
    return belowFluid != nullptr && isSameSource(*belowFluid);
}

i32 FlowingFluid::getHorizontalSourceCount(IWorld& world, const BlockPos& pos) const {
    i32 count = 0;

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        const FluidState* neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);

        if (neighborFluid != nullptr && isSameSource(*neighborFluid)) {
            count++;
        }
    }

    return count;
}

bool FlowingFluid::isBlocked(IWorld& world, const BlockPos& pos,
                              const BlockState* block, const Fluid& fluid) const {
    if (block == nullptr) {
        return false;
    }

    const Block& blockRef = block->owner();

    // 允许实现 ILiquidContainer 的方块按自身规则接收流体。
    if (auto* container = dynamic_cast<const ILiquidContainer*>(&blockRef)) {
        return container->canContainFluid(world, pos, *block, fluid);
    }

    const String& path = blockRef.blockLocation().path();
    if (hasSuffix(path, "_door") || hasSuffix(path, "_sign") ||
        path == "ladder" || path == "sugar_cane" || path == "bubble_column") {
        return false;
    }

    const Material& material = blockRef.material();

    if (material == Material::PORTAL || material == Material::STRUCTURE_VOID ||
        material == Material::OCEAN_PLANT || material == Material::SEA_GRASS) {
        return false;
    }

    // 默认：非固体方块可以被流体替换
    (void)world;
    (void)pos;
    (void)fluid;
    return !material.blocksMovement();
}

bool FlowingFluid::isSameOrEmpty(const FluidState& state) const {
    return state.isEmpty() || isEquivalentTo(state.getFluid());
}

bool FlowingFluid::isSameSource(const FluidState& state) const {
    return isEquivalentTo(state.getFluid()) && state.isSource();
}

i32 FlowingFluid::calculateFlowDecay(IWorld& world, const BlockPos& pos, i32 decay, Direction excludeDir,
                                      const BlockState* blockState, const BlockPos& sourcePos,
                                      std::unordered_map<i16, std::pair<const BlockState*, const FluidState*>>& stateCache,
                                      std::unordered_map<i16, bool>& fallCache) const {
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
            neighborBlock = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
            if (neighborBlock == nullptr) {
                continue;
            }
            neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);
            if (neighborFluid == nullptr) {
                continue;
            }
            stateCache.emplace(key, std::make_pair(neighborBlock, neighborFluid));
        } else {
            neighborBlock = it->second.first;
            neighborFluid = it->second.second;
        }

        FluidState targetState = calculateCorrectFlowingState(world, neighborPos, neighborBlock);

        if (canFlowInto(world, pos, blockState, dir, neighborPos, neighborBlock, *neighborFluid, targetState.getFluid())) {
            bool canFall = false;
            auto fallIt = fallCache.find(key);
            if (fallIt == fallCache.end()) {
                BlockPos below = neighborPos.down();
                const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
                canFall = canFlowDown(world, *this, neighborPos, neighborBlock, below, belowBlock);
                fallCache.emplace(key, canFall);
            } else {
                canFall = fallIt->second;
            }

            if (canFall) {
                return decay;
            }

            if (decay < getSpreadDistance(world)) {
                i32 neighborDecay = calculateFlowDecay(world, neighborPos, decay + 1,
                                                        Directions::opposite(dir), neighborBlock,
                                                        sourcePos, stateCache, fallCache);
                if (neighborDecay < minDecay) {
                    minDecay = neighborDecay;
                }
            }
        }
    }

    return minDecay;
}

bool FlowingFluid::canFlowDown(IWorld& world, const Fluid& fluid,
                                const BlockPos& pos, const BlockState* blockState,
                                const BlockPos& belowPos, const BlockState* belowBlock) const {
    if (!doesSideHaveHoles(Direction::Down, world, pos, blockState, belowPos, belowBlock)) {
        return false;
    }

    const FluidState* belowFluid = world.getFluidState(belowPos.x, belowPos.y, belowPos.z);
    if (belowFluid != nullptr && belowFluid->getFluid().isEquivalentTo(fluid)) {
        return true;
    }

    return isBlocked(world, belowPos, belowBlock, fluid);
}

bool FlowingFluid::canFlowInto(IWorld& world, const BlockPos& fromPos,
                                const BlockState* fromBlock, Direction dir,
                                const BlockPos& toPos, const BlockState* toBlock,
                                const FluidState& toFluid, const Fluid& fluidIn) const {
    if (!isSameSource(toFluid) && doesSideHaveHoles(dir, world, fromPos, fromBlock, toPos, toBlock)) {
        return isBlocked(world, toPos, toBlock, fluidIn);
    }
    return false;
}

i16 FlowingFluid::packRelativePos(const BlockPos& source, const BlockPos& target) const {
    i32 dx = target.x - source.x;
    i32 dz = target.z - source.z;
    return static_cast<i16>(((dx + 128) & 0xFF) << 8 | ((dz + 128) & 0xFF));
}

std::unordered_map<Direction, FluidState> FlowingFluid::getFlowDirections(
    IWorld& world, const BlockPos& pos, const BlockState* blockState) {
    std::unordered_map<Direction, FluidState> result;
    i32 minDecay = 1000;

    std::unordered_map<i16, std::pair<const BlockState*, const FluidState*>> stateCache;
    std::unordered_map<i16, bool> fallCache;

    for (Direction dir : Directions::horizontal()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        i16 key = packRelativePos(pos, neighborPos);

        const BlockState* neighborBlock = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (neighborBlock == nullptr) {
            continue;
        }
        const FluidState* neighborFluid = world.getFluidState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (neighborFluid == nullptr) {
            continue;
        }

        stateCache.emplace(key, std::make_pair(neighborBlock, neighborFluid));

        FluidState targetState = calculateCorrectFlowingState(world, neighborPos, neighborBlock);

        if (canFlowInto(world, pos, blockState, dir, neighborPos, neighborBlock, *neighborFluid, targetState.getFluid())) {
            BlockPos below = neighborPos.down();
            const BlockState* belowBlock = world.getBlockState(below.x, below.y, below.z);
            bool canFall = canFlowDown(world, *this, neighborPos, neighborBlock, below, belowBlock);
            fallCache.emplace(key, canFall);

            i32 decay;
            if (canFall) {
                decay = 0;
            } else {
                decay = calculateFlowDecay(world, neighborPos, 1,
                                           Directions::opposite(dir), neighborBlock,
                                           pos, stateCache, fallCache);
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
