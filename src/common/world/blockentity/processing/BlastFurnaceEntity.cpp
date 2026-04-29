#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

BlastFurnaceEntity::BlastFurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::BlastFurnace, pos) {
}

std::unique_ptr<BlockEntity> BlastFurnaceEntity::clone() const {
    auto cloned = std::make_unique<BlastFurnaceEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "BlastFurnaceEntity clone load failed");

    return cloned;
}

bool BlastFurnaceEntity::canSmelt(IWorld& world) const {
    // MC 1.16.5: 高炉只能熔炼矿石和金属物品
    // 通过配方类型过滤：只有 Blasting 类型的配方才能使用
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    if (recipe == nullptr) {
        return false;
    }

    // 检查配方类型是否为高炉配方
    if (recipe->getType() != crafting::RecipeType::Blasting) {
        return false;
    }

    return canSmeltWithRecipe(recipe);
}

} // namespace blockentity
} // namespace mc