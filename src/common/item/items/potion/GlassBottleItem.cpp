#include "GlassBottleItem.hpp"

#include "../../../core/BlockRaycastResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/blocks/CauldronBlock.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"

namespace {

[[nodiscard]] bool canFillFromWaterSource(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::fluid::FluidState* fluidState = world.getFluidState(pos);
    return fluidState != nullptr && !fluidState->isEmpty() && fluidState->isSource() && world.isWaterAt(pos);
}

[[nodiscard]] bool canFillFromCauldron(const mc::IWorld& world, const mc::BlockPos& pos)
{
    const mc::BlockState* state = world.getBlockState(pos);
    return state != nullptr && state->is(mc::VanillaBlocks::CAULDRON) &&
        mc::blocks::CauldronBlock::getLevel(*state) > 0;
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

GlassBottleItem::GlassBottleItem(const ItemProperties& properties)
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
        if (hitState != nullptr && hitState->is(VanillaBlocks::CAULDRON) &&
            blocks::CauldronBlock::getLevel(*hitState) > 0) {
            return ItemActionResult::consume(mc::potion::PotionUtils::createPotionItem(mc::potion::Potions::WATER));
        }
    }

    return ItemActionResult::pass(heldStack);
}

} // namespace item
} // namespace mc
