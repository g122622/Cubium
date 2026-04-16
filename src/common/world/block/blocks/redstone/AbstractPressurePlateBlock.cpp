#include "AbstractPressurePlateBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../util/AxisAlignedBB.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

AbstractPressurePlateBlock::AbstractPressurePlateBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::POWER_0_15())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::POWER_0_15(), 0));
}

i32 AbstractPressurePlateBlock::getPower(const BlockState& state) {
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState AbstractPressurePlateBlock::withPower(BlockState state, i32 power) {
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

void AbstractPressurePlateBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 压力板放置时不触发信号
}

void AbstractPressurePlateBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                                 const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查下方支撑是否还在
    BlockPos supportPos = pos.down();
    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        // 压力板掉落 - 设置为空气方块
        world.setBlockState(pos, nullptr, 2);
    }
}

void AbstractPressurePlateBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    i32 oldPower = getPower(state);
    i32 newPower = calculateSignalStrength(world, pos);

    if (newPower != oldPower) {
        // 状态改变
        BlockState newState = withPower(state, newPower);
        world.setBlockState(pos, &newState, 2);

        // 播放音效
        playClickSound(world, pos, newPower > 0);

        // 通知相邻方块更新
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState && !neighborState->isAir()) {
                Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
                neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
            }
        }
    } else if (newPower > 0) {
        // 仍然有压力，继续检测
        world.scheduleBlockTick(pos, *this, getTickDelay(oldPower, newPower), world::tick::TickPriority::High);
    }
}

i32 AbstractPressurePlateBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 压力板向所有方向输出信号
    return getPower(state);
}

i32 AbstractPressurePlateBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    // 压力板向下方输出强信号
    if (side == Direction::Down) {
        return getPower(state);
    }
    return 0;
}

const CollisionShape& AbstractPressurePlateBlock::getShape(const BlockState& state) const {
    static const CollisionShape unpressedShape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 1.0f, 15.0f);
    static const CollisionShape pressedShape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 0.5f, 15.0f);
    return getPower(state) > 0 ? pressedShape : unpressedShape;
}

bool AbstractPressurePlateBlock::hasEntityOnPlate(IWorld& world, const BlockPos& pos) const {
    // 创建压力板上方的碰撞箱
    // 压力板检测范围为方块上方的一个薄层
    AxisAlignedBB detectionBox(
        static_cast<f32>(pos.x) + 0.125f,  // 略微收缩水平范围
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,   // 检测向上0.25格
        static_cast<f32>(pos.z) + 0.875f
    );

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 过滤：只检测可以触发压力板的实体（玩家、生物、物品等）
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            // 检查实体类型：玩家、生物、物品实体都可以触发压力板
            LegacyEntityType type = entity->legacyType();
            if (type == LegacyEntityType::Player || type == LegacyEntityType::Item) {
                return true;
            }
            // 后续可以添加更多类型：Mob, Animal 等
        }
    }

    return false;
}

void AbstractPressurePlateBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state) {
    i32 oldPower = getPower(state);
    i32 newPower = calculateSignalStrength(world, pos);

    if (newPower != oldPower) {
        // 状态改变，调度tick
        world.scheduleBlockTick(pos, *this, getTickDelay(oldPower, newPower), world::tick::TickPriority::High);
    }
}

} // namespace blocks
} // namespace mc
