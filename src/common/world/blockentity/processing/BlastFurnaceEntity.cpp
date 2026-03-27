#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/World.hpp"

namespace mc {
namespace blockentity {

BlastFurnaceEntity::BlastFurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::BlastFurnace, pos) {
}

std::unique_ptr<BlockEntity> BlastFurnaceEntity::clone() const {
    auto cloned = std::make_unique<BlastFurnaceEntity>(m_pos);
    // TODO: 复制熔炉状态（燃烧时间、熔炼进度等）
    return cloned;
}

bool BlastFurnaceEntity::canSmelt(World& world) const {
    // 首先检查基础条件
    if (!AbstractFurnaceEntity::canSmelt(world)) {
        return false;
    }

    // 高炉只能熔炼矿石和金属物品
    const ItemStack& input = getFurnaceInventory().getInputItem();
    if (input.isEmpty()) {
        return false;
    }

    // TODO: 检查输入物品是否是矿石或金属
    // 可以通过检查物品是否在高炉配方中来判断
    // 或者检查物品标签（ores, raw_materials 等）

    // 目前暂时允许所有物品
    return true;
}

} // namespace blockentity
} // namespace mc
