#include "PistonBlock.hpp"
#include "PistonStructureHelper.hpp"
#include "MovingPistonBlock.hpp"
#include "PistonHeadBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../blockentity/interactive/PistonBlockEntity.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

PistonBlock::PistonBlock(const BlockProperties& properties, bool sticky)
    : Block(properties)
    , m_sticky(sticky) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::EXTENDED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::EXTENDED(), false));
}

bool PistonBlock::isExtended(const BlockState& state) {
    return state.get(BlockStateProperties::EXTENDED());
}

BlockState PistonBlock::withExtended(BlockState state, bool extended) {
    return state.with(BlockStateProperties::EXTENDED(), extended);
}

Direction PistonBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

void PistonBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // MC 1.16.5: 放置时检查是否需要伸出
    checkForMove(world, pos, state);
}

void PistonBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                   const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // MC 1.16.5: 邻居变化时检查是否需要改变状态
    const BlockState* state = world.getBlockState(pos);
    if (state) {
        checkForMove(world, pos, *state);
    }
}

void PistonBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // 活塞动画由 PistonBlockEntity 处理
}

BlockState PistonBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    return state;
}

bool PistonBlock::shouldBeExtended(IWorld& world, const BlockPos& pos, const BlockState& state) const {
    Direction facing = getFacing(state);

    // MC 1.16.5: 检查活塞本体除前面外5个方向是否被充能
    // 注意：不检查活塞朝向方向（前面）的信号

    for (Direction dir : Directions::all()) {
        if (dir == facing) {
            // 不检查活塞朝向方向（前面）
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);
        // MC 1.16.5: worldIn.isSidePowered(pos.offset(direction), direction)
        // 检查相邻方块在该方向是否被充能（从该方向接收强信号）
        if (world::redstone::RedstonePower::isSidePowered(world, neighborPos, dir)) {
            return true;
        }
    }

    // MC 1.16.5: 额外检查活塞上方位置的水平信号
    // 这是 MC Java 的特殊逻辑
    if (world::redstone::RedstonePower::isSidePowered(world, pos, Direction::Down)) {
        return true;
    }

    BlockPos abovePos = pos.up();
    for (Direction dir : Directions::all()) {
        if (dir != Direction::Down) {
            BlockPos checkPos = abovePos.offset(dir);
            if (world::redstone::RedstonePower::isSidePowered(world, checkPos, dir)) {
                return true;
            }
        }
    }

    return false;
}

bool PistonBlock::canPush(
    const BlockState& blockState,
    IWorld& world,
    const BlockPos& pos,
    Direction facing,
    bool destroyBlocks,
    Direction direction) {

    // MC 1.16.5: PistonBlock.canPush

    // 检查高度限制
    if (pos.y < 0 || pos.y >= world.getHeight(pos.x, pos.z)) {
        return false;
    }

    // 检查世界边界
    // TODO: if (!world.getWorldBorder().contains(pos)) return false;

    // 空气可以推动
    if (blockState.isAir()) {
        return true;
    }

    // 不可推动的方块
    // 参考 MC 1.16.5: PistonBlock.canPush - obsidian, crying_obsidian, respawn_anchor, etc.
    if (blockState.is(VanillaBlocks::OBSIDIAN) ||
        blockState.is(VanillaBlocks::CRYING_OBSIDIAN) ||
        blockState.is(VanillaBlocks::RESPAWN_ANCHOR)) {
        return false;
    }

    // 检查高度边界（向下推时检查底部，向上推时检查顶部）
    if (facing == Direction::Down && pos.y == 0) {
        return false;
    }
    if (facing == Direction::Up && pos.y >= world.getHeight(pos.x, pos.z) - 1) {
        return false;
    }

    // 检查活塞本身
    if (blockState.is(VanillaBlocks::PISTON) || blockState.is(VanillaBlocks::STICKY_PISTON)) {
        // 已伸出的活塞不能被推动
        if (isExtended(blockState)) {
            return false;
        }
    } else {
        // 检查硬度和推动反应
        if (blockState.hardness() < 0.0f) {
            // 不可破坏的方块
            return false;
        }

        Material::PushReaction reaction = blockState.getMaterial().getPushReaction();
        switch (reaction) {
            case Material::PushReaction::Block:
                return false;
            case Material::PushReaction::Destroy:
                return destroyBlocks;
            case Material::PushReaction::PushOnly:
                // 只能被活塞面推动
                return facing == direction;
            default:
                break;
        }
    }

    // 有方块实体的方块不能被推动
    const Block& block = blockState.getBlock();
    if (block.hasBlockEntity()) {
        return false;
    }

    return true;
}

void PistonBlock::checkForMove(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // MC 1.16.5: checkForMove
    Direction facing = getFacing(state);
    bool shouldExtend = shouldBeExtended(world, pos, state);
    bool isCurrentlyExtended = isExtended(state);

    if (shouldExtend != isCurrentlyExtended) {
        if (shouldExtend) {
            // 应该伸出
            PistonStructureHelper helper(world, pos, facing, true);
            if (helper.canMove()) {
                extend(world, pos, state);
            }
        } else {
            // 应该收回
            retract(world, pos, state);
        }
    }
}

bool PistonBlock::extend(IWorld& world, const BlockPos& pos, const BlockState& state) {
    Direction facing = getFacing(state);

    // 使用 PistonStructureHelper 计算推动链
    PistonStructureHelper helper(world, pos, facing, true);
    if (!helper.canMove()) {
        return false;
    }

    // 执行移动
    if (!doMove(world, pos, facing, true)) {
        return false;
    }

    // 更新活塞状态为伸出
    BlockState newState = withExtended(state, true);
    world.setBlockState(pos, &newState, 67);

    return true;
}

bool PistonBlock::retract(IWorld& world, const BlockPos& pos, const BlockState& state) {
    Direction facing = getFacing(state);

    // 更新活塞状态为收回
    BlockState newState = withExtended(state, false);
    world.setBlockState(pos, &newState, 67);

    if (m_sticky) {
        // 粘性活塞：尝试拉回方块
        BlockPos frontPos = pos.offset(facing);  // 活塞头位置
        BlockPos pullPos = frontPos.offset(facing);  // 活塞头前面的方块

        const BlockState* pullState = world.getBlockState(pullPos);
        if (pullState && !pullState->isAir()) {
            // 检查方块是否可以被拉回
            if (canPush(*pullState, world, pullPos, Directions::opposite(facing), false, facing)) {
                Material::PushReaction reaction = pullState->getMaterial().getPushReaction();
                if (reaction == Material::PushReaction::Normal ||
                    pullState->is(VanillaBlocks::PISTON) ||
                    pullState->is(VanillaBlocks::STICKY_PISTON)) {
                    // 执行拉回
                    doMove(world, pos, facing, false);
                    return true;
                }
            }
        }

        // 移除活塞头
        world.setBlockState(frontPos, nullptr, 20);
    } else {
        // 普通活塞：移除活塞头
        BlockPos frontPos = pos.offset(facing);
        world.setBlockState(frontPos, nullptr, 20);
    }

    return true;
}

bool PistonBlock::doMove(IWorld& world, const BlockPos& pos, Direction facing, bool extending) {
    // MC 1.16.5: doMove
    BlockPos frontPos = pos.offset(facing);

    // 收回时先清除活塞头
    if (!extending) {
        const BlockState* headState = world.getBlockState(frontPos);
        if (headState && headState->is(VanillaBlocks::PISTON_HEAD)) {
            world.setBlockState(frontPos, nullptr, 20);
        }
    }

    PistonStructureHelper helper(world, pos, facing, extending);
    if (!helper.canMove()) {
        return false;
    }

    // 获取要移动和要破坏的方块
    const std::vector<BlockPos>& toMove = helper.getBlocksToMove();
    const std::vector<BlockPos>& toDestroy = helper.getBlocksToDestroy();

    // 先破坏需要破坏的方块（从远到近）
    for (auto it = toDestroy.rbegin(); it != toDestroy.rend(); ++it) {
        const BlockPos& destroyPos = *it;
        const BlockState* destroyState = world.getBlockState(destroyPos);
        if (destroyState && !destroyState->isAir()) {
            // 参考 MC 1.16.5: PistonBlock.doMove
            // 破坏方块时掉落物品
            const Block* destroyBlock = &destroyState->getBlock();
            if (destroyBlock != nullptr) {
                const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*destroyBlock);
                if (blockItem != nullptr) {
                    ItemStack dropStack(blockItem, 1);
                    math::Random rng;
                    ItemDropHelper::spawnItemEntity(&world, dropStack,
                        static_cast<f64>(destroyPos.x) + 0.5,
                        static_cast<f64>(destroyPos.y) + 0.5,
                        static_cast<f64>(destroyPos.z) + 0.5,
                        rng);
                }
            }
            world.setBlockState(destroyPos, nullptr, 18);
        }
    }

    // 移动方块（从远到近）
    Direction moveDir = extending ? facing : Directions::opposite(facing);
    for (auto it = toMove.rbegin(); it != toMove.rend(); ++it) {
        const BlockPos& movePos = *it;
        const BlockState* moveState = world.getBlockState(movePos);
        if (!moveState) continue;

        BlockPos newPos = movePos.offset(moveDir);

        // 创建移动活塞方块（暂时，需要 PistonBlockEntity 来处理动画）
        BlockState movingState = VanillaBlocks::MOVING_PISTON->defaultState()
            .with(BlockStateProperties::FACING(), facing);
        world.setBlockState(newPos, &movingState, 68);

        // 创建 PistonBlockEntity
        auto entity = std::make_unique<blockentity::PistonBlockEntity>(
            newPos, moveState, facing, extending, false);
        world.setBlockEntity(newPos, entity.release());

        // 清除原位置
        world.setBlockState(movePos, nullptr, 68);
    }

    // 如果是伸出，在活塞位置创建移动活塞（用于活塞头动画）
    if (extending) {
        // 创建活塞头状态
        BlockState pistonHeadState = VanillaBlocks::PISTON_HEAD->defaultState()
            .with(BlockStateProperties::FACING(), facing)
            .with(PistonHeadBlock::getTypeProperty(), m_sticky ? PistonHeadBlock::Type::Sticky : PistonHeadBlock::Type::Normal);

        // 创建移动活塞方块
        BlockState movingState = VanillaBlocks::MOVING_PISTON->defaultState()
            .with(BlockStateProperties::FACING(), facing)
            .with(PistonHeadBlock::getTypeProperty(), m_sticky ? PistonHeadBlock::Type::Sticky : PistonHeadBlock::Type::Normal);

        world.setBlockState(pos, &movingState, 68);

        // 创建 PistonBlockEntity 用于活塞头
        // 注意：pistonHeadState 是局部变量，需要获取其指针
        const BlockState* pistonHeadStatePtr = world.getBlockState(pos);
        // 暂时跳过创建 PistonBlockEntity，因为 pistonHeadState 是局部变量
        // TODO: 需要从 BlockRegistry 获取持久化的 BlockState 指针
    }

    return true;
}

Material::PushReaction PistonBlock::getBlockPushReaction(const BlockState& state) const {
    // 已伸出的活塞不能被推动
    if (isExtended(state)) {
        return Material::PushReaction::Block;
    }
    return Material::PushReaction::Normal;
}

} // namespace blocks
} // namespace mc
