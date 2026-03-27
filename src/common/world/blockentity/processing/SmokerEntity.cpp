#include "world/blockentity/processing/SmokerEntity.hpp"

namespace mc {
namespace blockentity {

SmokerEntity::SmokerEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Smoker, pos) {
}

bool SmokerEntity::canSmelt(IWorld& world) const {
    // 首先检查基础条件
    if (!AbstractFurnaceEntity::canSmelt(world)) {
        return false;
    }

    // 烟熏炉只能烹饪食物
    const ItemStack& input = getFurnaceInventory().getInputItem();
    if (input.isEmpty()) {
        return false;
    }

    // TODO: 检查输入物品是否是食物
    // 可以通过检查物品是否在烟熏配方中来判断
    // 或者检查物品标签（foods 等）

    // 目前暂时允许所有物品
    return true;
}

} // namespace blockentity
} // namespace mc
