/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

BlastFurnaceEntity::BlastFurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::BlastFurnace, pos)
{}

const ResourceLocation& BlastFurnaceEntity::getFireCrackleSound() const
{
    return SoundEvents::BLOCK_BLASTFURNACE_FIRE_CRACKLE;
}

std::unique_ptr<BlockEntity> BlastFurnaceEntity::clone() const
{
    auto cloned = std::make_unique<BlastFurnaceEntity>(m_pos);

    nlohmann::json state;
    save(state);
    const bool loaded = cloned->load(state);
    MC_ASSERT(loaded && "BlastFurnaceEntity clone load failed");

    return cloned;
}

bool BlastFurnaceEntity::canSmelt(IWorld& world) const
{
    // 高炉只能熔炼矿石和金属物品，通过配方类型过滤：只有 Blasting 类型的配方才能使用
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