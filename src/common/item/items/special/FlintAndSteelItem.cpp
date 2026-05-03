#include "FlintAndSteelItem.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/blocks/decorative/CampfireBlock.hpp"
#include "../../../world/block/blocks/nether/FireBlock.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"

namespace mc {
namespace item {
namespace tool {

FlintAndSteelItem::FlintAndSteelItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

ActionResultType FlintAndSteelItem::onItemUse(ItemUseContext& context) {
    // MC 1.16.5: FlintAndSteelItem.onItemUse
    Player* player = context.getPlayer();
    IWorld& world = context.getWorld();
    const BlockPos& blockPos = context.getBlockPos();
    Direction face = context.getFace();
    const BlockState* blockStatePtr = world.getBlockState(blockPos);

    if (blockStatePtr == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查是否可以点燃营火
    // 参考: CampfireBlock.canBeLit(blockstate)
    if (blockStatePtr->hasProperty(BlockStateProperties::LIT())) {
        if (!blockStatePtr->get(BlockStateProperties::LIT())) {
            // 点燃营火
            BlockState newState = blockStatePtr->with(BlockStateProperties::LIT(), true);
            world.setBlockState(blockPos, &newState, 11);

            // 消耗耐久
            if (player != nullptr) {
                context.getItemStackMut().attemptDamageItem(1);
            }
            return ActionResultType::Success;
        }
    }

    // 否则尝试在点击面的相邻位置放置火焰
    BlockPos firePos = blockPos.offset(face);

    // 检查是否可以放置火焰
    if (canLightBlock(world, firePos)) {
        // 获取应该放置的火焰方块（普通火或灵魂火）
        Block* fireBlock = getFireForPlacement(world, firePos);
        if (fireBlock != nullptr) {
            // 放置火焰
            const BlockState& fireState = fireBlock->getDefaultState();
            world.setBlockState(firePos, &fireState, 11);

            // 消耗耐久
            if (player != nullptr) {
                context.getItemStackMut().attemptDamageItem(1);
            }

            return ActionResultType::Success;
        }
    }

    return ActionResultType::Fail;
}

bool FlintAndSteelItem::canLightBlock(IWorld& world, const BlockPos& pos) {
    // MC 1.16.5: AbstractFireBlock.canLightBlock
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return false;
    }

    const BlockState& state = *statePtr;

    // 如果位置已经是空气或其他可替换方块
    if (state.isAir() || state.getMaterial().isReplaceable()) {
        // 检查下方方块是否可以支撑火焰
        const BlockState* belowStatePtr = world.getBlockState(pos.down());
        if (belowStatePtr == nullptr) {
            return false;
        }

        // 火焰需要有可燃物支撑或在灵魂土上
        // 简化实现：检查下方方块是否固体
        return belowStatePtr->isSolid();
    }

    return false;
}

Block* FlintAndSteelItem::getFireForPlacement(IWorld& world, const BlockPos& pos) {
    // MC 1.16.5: AbstractFireBlock.getFireForPlacement
    // 检查下方是否是灵魂土，如果是则返回灵魂火
    const BlockState* belowStatePtr = world.getBlockState(pos.down());

    // TODO: 检查是否是灵魂土 (SOUL_SOIL 或 SOUL_SAND)
    // 目前简单返回普通火
    if (VanillaBlocks::FIRE != nullptr) {
        return VanillaBlocks::FIRE;
    }

    return nullptr;
}

} // namespace tool
} // namespace item
} // namespace mc
