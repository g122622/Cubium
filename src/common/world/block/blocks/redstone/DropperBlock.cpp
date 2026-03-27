#include "DropperBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/interactive/DispenserBlockEntity.hpp"

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
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // TODO: 获取方块实体并投掷物品
    // 投掷器的投掷逻辑与发射器类似，但不使用特殊行为
    // 1. 获取 DispenserBlockEntity（投掷器使用相同的方块实体类型）
    // 2. 随机选择非空槽位（使用储水池采样算法）
    // 3. 检查前方是否有容器
    //    - 如果有容器，尝试将物品放入容器
    //    - 如果没有容器，投掷物品到世界中
    // 4. 减少物品数量

    return false;
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
