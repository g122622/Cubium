#include "DropperBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/interactive/DispenserBlockEntity.hpp"
#include "../../../../item/ItemStack.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

DropperBlock::DropperBlock(const BlockProperties& properties)
    : DispenserBlock(properties) {
    // 投掷器继承自发射器，复用基本功能
}

void DropperBlock::dispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 投掷器使用简单的投掷行为
    if (tryDispense(world, pos, state)) {
        playDispenseSound(world, pos);
    }
}

bool DropperBlock::tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return false;
    }

    // 转换为发射器方块实体（投掷器使用相同的方块实体类型）
    blockentity::DispenserBlockEntity* dropper =
        dynamic_cast<blockentity::DispenserBlockEntity*>(blockEntity);
    if (dropper == nullptr) {
        return false;
    }

    // 使用储水池采样算法选择非空槽位
    i32 slot = dropper->getDispenseSlot();
    if (slot < 0) {
        return false;  // 没有物品可投掷
    }

    // 获取物品
    IInventory* inventory = dropper->getInventory();
    if (inventory == nullptr) {
        return false;
    }

    ItemStack stack = inventory->getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }

    // 获取投掷方向
    Direction facing = getFacing(state);
    BlockPos targetPos = pos.offset(facing);

    // 发射物品 - 分离一个物品
    ItemStack dispensedStack = stack.split(1);
    inventory->setItem(slot, stack.getCount() > 0 ? stack : ItemStack::EMPTY);

    // 计算投掷位置（方块前方中心）
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
                dropper->setChanged();
                return true;
            }

            // 容器无法完全接收，将物品返回原槽位
            stack.grow(1);
            inventory->setItem(slot, stack);
        }
    }

    // TODO: 需要世界支持 spawnEntity 来创建物品实体
    // 投掷器的投掷速度比发射器慢
    // f32 vx = 0.0f, vy = 0.0f, vz = 0.0f;
    // switch (facing) {
    //     case Direction::North: vz = -0.1f; break;  // 投掷器速度较慢
    //     case Direction::South: vz = 0.1f; break;
    //     case Direction::East:  vx = 0.1f; break;
    //     case Direction::West:  vx = -0.1f; break;
    //     case Direction::Up:    vy = 0.3f; break;
    //     case Direction::Down:  vy = -0.1f; break;
    //     default: break;
    // }
    // auto itemEntity = std::make_unique<ItemEntity>(
    //     EntityId(0), dispensedStack, dispensePos.x, dispensePos.y, dispensePos.z, vx, vy, vz
    // );
    // world.spawnEntity(std::move(itemEntity));

    dropper->setChanged();
    return true;
}

void DropperBlock::playDispenseSound(IWorld& world, const BlockPos& pos) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 播放投掷音效
    // 投掷器使用与发射器不同的音效
    // world.playSound(pos, SoundEvents::BLOCK_DISPENSER_DROP, 1.0f, 1.0f);
}

} // namespace blocks
} // namespace mc
