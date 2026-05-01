#include "DispenserBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/interactive/DispenserBlockEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

DispenserBlock::DispenserBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::TRIGGERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::TRIGGERED(), false));
}

bool DispenserBlock::isTriggered(const BlockState& state) {
    return state.get(BlockStateProperties::TRIGGERED());
}

BlockState DispenserBlock::withTriggered(BlockState state, bool triggered) {
    return state.with(BlockStateProperties::TRIGGERED(), triggered);
}

Direction DispenserBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::FACING());
}

void DispenserBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 发射器放置时不触发
}

void DispenserBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                     const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查是否应该触发
    bool shouldTrigger = world::redstone::RedstonePower::isPowered(world, pos);
    bool isCurrentlyTriggered = isTriggered(*state);

    if (shouldTrigger != isCurrentlyTriggered) {
        if (shouldTrigger) {
            // 被激活，调度发射
            world.tickManager().scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
        // 更新触发状态
        BlockState newState = withTriggered(*state, shouldTrigger);
        world.setBlockState(pos, &newState, 2);
    }
}

BlockState DispenserBlock::updatePostPlacement(
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

void DispenserBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 尝试发射物品
    dispense(world, pos, state);
}

void DispenserBlock::dispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 尝试发射物品
    if (tryDispense(world, pos, state)) {
        playDispenseSound(world, pos);
    }
}

bool DispenserBlock::tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return false;
    }

    // 转换为发射器方块实体
    blockentity::DispenserBlockEntity* dispenser =
        dynamic_cast<blockentity::DispenserBlockEntity*>(blockEntity);
    if (dispenser == nullptr) {
        return false;
    }

    // 使用储水池采样算法选择非空槽位
    i32 slot = dispenser->getDispenseSlot();
    if (slot < 0) {
        return false;  // 没有物品可发射
    }

    // 获取物品
    IInventory* inventory = dispenser->getInventory();
    if (inventory == nullptr) {
        return false;
    }

    ItemStack stack = inventory->getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }

    // 获取发射方向
    Direction facing = getFacing(state);
    BlockPos targetPos = pos.offset(facing);

    // TODO: 检查特殊物品行为（如箭、药水、TNT等）
    // 目前使用默认行为：发射物品实体

    // 发射物品 - 分离一个物品
    ItemStack dispensedStack = stack.split(1);
    inventory->setItem(slot, stack.getCount() > 0 ? stack : ItemStack::EMPTY);

    // 计算发射位置（方块前方中心）
    Vector3 dispensePos(
        static_cast<f32>(targetPos.x) + 0.5f,
        static_cast<f32>(targetPos.y) + 0.5f,
        static_cast<f32>(targetPos.z) + 0.5f
    );

    // 检查目标位置是否有容器
    BlockEntity* targetEntity = world.getBlockEntity(targetPos);
    if (targetEntity != nullptr) {
        IInventory* targetInventory = dynamic_cast<IInventory*>(targetEntity);
        if (targetInventory != nullptr) {
            // 尝试将物品放入容器
            // 首先尝试堆叠到现有槽位
            for (i32 i = 0; i < targetInventory->getContainerSize(); ++i) {
                ItemStack existingStack = targetInventory->getItem(i);
                if (!existingStack.isEmpty() && existingStack.isSameItem(dispensedStack)) {
                    i32 space = existingStack.getMaxStackSize() - existingStack.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, dispensedStack.getCount());
                        existingStack.grow(toAdd);
                        targetInventory->setItem(i, existingStack);
                        dispensedStack.shrink(toAdd);
                        if (dispensedStack.isEmpty()) {
                            break;
                        }
                    }
                }
            }

            // 如果还有剩余，尝试放入空槽位
            if (!dispensedStack.isEmpty()) {
                i32 emptySlot = targetInventory->getFirstEmptySlot();
                if (emptySlot >= 0) {
                    targetInventory->setItem(emptySlot, dispensedStack);
                    dispensedStack = ItemStack::EMPTY;
                }
            }

            // 物品完全放入容器
            if (dispensedStack.isEmpty()) {
                dispenser->setChanged();
                return true;
            }

            // 容器无法完全接收，将物品返回原槽位
            stack.grow(1);
            inventory->setItem(slot, stack);
        }
    }

    // TODO: 需要世界支持 spawnEntity 来创建物品实体
    // 目前只更新发射器状态
    // 计算发射速度
    // f32 vx = 0.0f, vy = 0.0f, vz = 0.0f;
    // switch (facing) {
    //     case Direction::North: vz = -0.3f; break;
    //     case Direction::South: vz = 0.3f; break;
    //     case Direction::East:  vx = 0.3f; break;
    //     case Direction::West:  vx = -0.3f; break;
    //     case Direction::Up:    vy = 0.3f; break;
    //     case Direction::Down:  vy = -0.3f; break;
    //     default: break;
    // }
    // auto itemEntity = std::make_unique<ItemEntity>(
    //     EntityId(0), dispensedStack, dispensePos.x, dispensePos.y, dispensePos.z, vx, vy, vz
    // );
    // world.spawnEntity(std::move(itemEntity));

    dispenser->setChanged();
    return true;
}

void DispenserBlock::playDispenseSound(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放发射音效
    // world.playSound(pos, SoundEvents::BLOCK_DISPENSER_DISPENSE, 1.0f, 1.0f);
}

} // namespace blocks
} // namespace mc
