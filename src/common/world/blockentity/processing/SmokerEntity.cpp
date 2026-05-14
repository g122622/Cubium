#include "world/blockentity/processing/SmokerEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

SmokerEntity::SmokerEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Smoker, pos)
{}

const ResourceLocation& SmokerEntity::getFireCrackleSound() const
{
    return SoundEvents::BLOCK_SMOKER_SMOKE;
}

std::unique_ptr<BlockEntity> SmokerEntity::clone() const
{
    auto cloned = std::make_unique<SmokerEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "SmokerEntity clone load failed");

    return cloned;
}

bool SmokerEntity::canSmelt(IWorld& world) const
{
    // MC 1.16.5: 烟熏炉只能烹饪食物
    // 通过配方类型过滤：只有 Smoking 类型的配方才能使用
    const crafting::SmeltingRecipe* recipe = getRecipe(world);
    if (recipe == nullptr) {
        return false;
    }

    // 检查配方类型是否为烟熏炉配方
    if (recipe->getType() != crafting::RecipeType::Smoking) {
        return false;
    }

    return canSmeltWithRecipe(recipe);
}

} // namespace blockentity
} // namespace mc