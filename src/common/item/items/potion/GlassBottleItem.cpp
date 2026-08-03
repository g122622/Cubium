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

#include "GlassBottleItem.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/block/blocks/CauldronBlock.hpp"
#include "common/world/block/blocks/LayeredCauldronBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace {

[[nodiscard]] bool canFillFromWaterSource(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::fluid::FluidState* fluidState = world.getFluidState(pos);
    return fluidState != nullptr && !fluidState->isEmpty() && fluidState->isSource() && world.isWaterAt(pos);
}

[[nodiscard]] bool canFillFromCauldron(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    // 水炼药锅（LayeredCauldronBlock）始终有水（水位1-3），可取水
    if (state->is(mc::VanillaBlocks::WATER_CAULDRON)) {
        return true;
    }
    // 空炼药锅不可取水（没有水位属性）
    return false;
}

[[nodiscard]] bool hasBottleFillTarget(const mc::IWorld& world, const mc::Ray& ray, mc::f32 maxDistance)
{
    constexpr mc::f32 SAMPLE_STEP = 0.1f;

    for (mc::f32 distance = 0.0f; distance <= maxDistance; distance += SAMPLE_STEP) {
        const mc::Vector3 sample = ray.at(distance);
        const mc::BlockPos pos(sample);
        if (canFillFromWaterSource(world, pos) || canFillFromCauldron(world, pos)) {
            return true;
        }
    }

    return false;
}

} // namespace

namespace mc {
namespace item {

// ========== GlassBottleItem 实现 ==========

GlassBottleItem::GlassBottleItem(const ItemProperties& properties) noexcept
    : Item(properties)
{}

ItemActionResult GlassBottleItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    const ItemStack heldStack = player.getHeldItem(hand);

    const Vector3 eyePosition(player.x(), player.y() + player.eyeHeight(), player.z());
    const Ray ray = Ray::fromAngles(eyePosition, player.pitch(), player.yaw());
    constexpr f32 MAX_DISTANCE = 5.0f;

    const BlockRaycastResult hit = raycastBlocks(RaycastContext(ray, MAX_DISTANCE), world);
    const f32 searchDistance = hit.isHit() ? hit.distance() : MAX_DISTANCE;

    if (hasBottleFillTarget(world, ray, searchDistance)) {
        return ItemActionResult::consume(mc::potion::PotionUtils::createPotionItem(mc::potion::Potions::WATER));
    }

    if (hit.isHit()) {
        const BlockState* hitState = world.getBlockState(hit.blockPos());
        // 水炼药锅（LayeredCauldronBlock）可取水
        if (hitState != nullptr && hitState->is(VanillaBlocks::WATER_CAULDRON)) {
            return ItemActionResult::consume(mc::potion::PotionUtils::createPotionItem(mc::potion::Potions::WATER));
        }
    }

    return ItemActionResult::pass(heldStack);
}

} // namespace item
} // namespace mc
