#include "RedstonePower.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../block/BlockPos.hpp"
#include "../../util/Direction.hpp"

namespace mc {
namespace world {
namespace redstone {

// ========== 强信号 ==========

i32 RedstonePower::getStrongPower(IWorld& world, const BlockPos& pos, Direction side) {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state || state->isAir()) {
        return MIN_POWER;
    }

    // 获取方块在指定方向的强信号输出
    return state->getBlock().getStrongPower(*state, world, pos, side);
}

i32 RedstonePower::getStrongPower(IWorld& world, const BlockPos& pos) {
    i32 maxPower = MIN_POWER;

    // 遍历六个方向
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);
        i32 power = getStrongPower(world, neighborPos, oppositeDir);
        maxPower = std::max(maxPower, power);

        if (maxPower >= MAX_POWER) {
            break;  // 已经是最大值，无需继续
        }
    }

    return maxPower;
}

// ========== 弱信号 ==========

i32 RedstonePower::getWeakPower(IWorld& world, const BlockPos& pos, Direction side) {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (!state || state->isAir()) {
        return MIN_POWER;
    }

    // 获取方块在指定方向的弱信号输出
    return state->getBlock().getWeakPower(*state, world, pos, side);
}

i32 RedstonePower::getWeakPower(IWorld& world, const BlockPos& pos) {
    i32 maxPower = MIN_POWER;

    // 遍历六个方向
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);
        i32 power = getWeakPower(world, neighborPos, oppositeDir);
        maxPower = std::max(maxPower, power);

        if (maxPower >= MAX_POWER) {
            break;  // 已经是最大值，无需继续
        }
    }

    return maxPower;
}

// ========== 充能检测 ==========

bool RedstonePower::isPowered(IWorld& world, const BlockPos& pos) {
    // 检查是否被间接充能（相邻方块有强信号输出）
    return isIndirectlyPowered(world, pos);
}

bool RedstonePower::isIndirectlyPowered(IWorld& world, const BlockPos& pos) {
    // 遍历六个方向检查是否有强信号
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (neighborState && !neighborState->isAir()) {
            if (neighborState->getBlock().getStrongPower(*neighborState, world, neighborPos, oppositeDir) > 0) {
                return true;
            }
        }
    }

    return false;
}

bool RedstonePower::isSidePowered(IWorld& world, const BlockPos& pos, Direction side) {
    BlockPos neighborPos = pos.offset(side);
    Direction oppositeDir = Directions::opposite(side);

    const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
    if (!neighborState || neighborState->isAir()) {
        return false;
    }

    return neighborState->getBlock().getStrongPower(*neighborState, world, neighborPos, oppositeDir) > 0;
}

// ========== 特殊信号计算 ==========

i32 RedstonePower::getWireInputPower(IWorld& world, const BlockPos& pos) {
    i32 maxPower = MIN_POWER;

    // 1. 从相邻信号源获取强信号
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 检查强信号
        if (neighborBlock.canProvidePower(*neighborState)) {
            i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
            if (strongPower > maxPower) {
                maxPower = strongPower;
            }
        }
    }

    // 2. 从相邻红石线获取信号（每格衰减1）
    if (maxPower < MAX_POWER) {
        for (Direction dir : Directions::horizontal()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

            if (!neighborState || neighborState->isAir()) {
                continue;
            }

            // 检查是否是红石线（这里需要后续实现 RedstoneWireBlock 后补充）
            // 暂时跳过，在实现红石线时会修改这个方法

            // 检查向上/向下连接
            // 如果相邻是实体方块，检查其上方是否有红石线
            if (isNormalCube(*neighborState)) {
                BlockPos upPos = neighborPos.up();
                const BlockState* upState = world.getBlockState(upPos.x, upPos.y, upPos.z);
                // 检查上方红石线信号（待实现）
                (void)upState;  // 暂时忽略
            } else {
                // 相邻不是实体方块，检查其下方是否有红石线
                BlockPos downPos = neighborPos.down();
                const BlockState* downState = world.getBlockState(downPos.x, downPos.y, downPos.z);
                // 检查下方红石线信号（待实现）
                (void)downState;  // 暂时忽略
            }
        }
    }

    return maxPower;
}

i32 RedstonePower::getComparatorInput(IWorld& world, const BlockPos& pos, Direction facing) {
    // 输入端在比较器的背面（朝向的反方向）
    BlockPos inputPos = pos.offset(Directions::opposite(facing));

    const BlockState* inputState = world.getBlockState(inputPos.x, inputPos.y, inputPos.z);
    if (!inputState || inputState->isAir()) {
        return MIN_POWER;
    }

    const Block& inputBlock = inputState->getBlock();

    // 1. 检查是否有比较器输入覆盖（容器等）
    if (inputBlock.hasComparatorInputOverride(*inputState)) {
        return inputBlock.getComparatorInputOverride(*inputState, world, inputPos);
    }

    // 2. 检查红石线信号
    // 如果输入端是红石线，获取其信号强度
    // （待红石线实现后补充）

    // 3. 获取普通的红石信号
    i32 power = MIN_POWER;

    // 从输入端方向获取信号
    Direction inputDir = facing;  // 信号从输入端传来
    i32 weakPower = inputBlock.getWeakPower(*inputState, world, inputPos, inputDir);
    i32 strongPower = inputBlock.getStrongPower(*inputState, world, inputPos, inputDir);

    power = std::max(weakPower, strongPower);

    return power;
}

i32 RedstonePower::getRedstonePowerFromNeighbors(IWorld& world, const BlockPos& pos) {
    i32 maxPower = MIN_POWER;

    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 只检测能输出红石信号的方块
        if (!neighborBlock.canProvidePower(*neighborState)) {
            continue;
        }

        // 获取强信号
        i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
        if (strongPower > maxPower) {
            maxPower = strongPower;
        }

        // 获取弱信号
        i32 weakPower = neighborBlock.getWeakPower(*neighborState, world, neighborPos, oppositeDir);
        if (weakPower > maxPower) {
            maxPower = weakPower;
        }

        if (maxPower >= MAX_POWER) {
            break;
        }
    }

    return maxPower;
}

// ========== 私有方法 ==========

bool RedstonePower::isNormalCube(const BlockState& state) {
    // 检查是否是实体方块（可以传导红石信号）
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstonePower::canConnectRedstone(const BlockState& state) {
    // 检查方块是否可以连接红石
    // 可以输出红石信号或可以被红石充能的方块
    return state.getBlock().canProvidePower(state);
}

} // namespace redstone
} // namespace world
} // namespace mc
