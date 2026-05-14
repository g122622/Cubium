#include "FlintAndSteelItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/blocks/decorative/CampfireBlock.hpp"
#include "../../../world/block/blocks/nether/FireBlock.hpp"
#include "../../context/ItemUseContext.hpp"

namespace mc {
namespace item {
namespace tool {

FlintAndSteelItem::FlintAndSteelItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType FlintAndSteelItem::onItemUse(ItemUseContext& context)
{
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

bool FlintAndSteelItem::canLightBlock(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: AbstractFireBlock.canLightBlock
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return false;
    }

    const BlockState& state = *statePtr;

    // 如果位置不是空气，不能点燃
    if (!state.isAir()) {
        return false;
    }

    // 获取应该放置的火焰方块，并检查其是否能在该位置有效存在
    Block* fireBlock = getFireForPlacement(world, pos);
    if (fireBlock == nullptr) {
        return false;
    }

    // 检查火焰方块是否能放置在该位置
    const BlockState& fireState = fireBlock->getDefaultState();
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    return fireBlock->isValidPosition(fireState, blockReader, pos);
}

Block* FlintAndSteelItem::getFireForPlacement(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: AbstractFireBlock.getFireForPlacement
    // 检查下方是否是灵魂沙/灵魂土，如果是则返回灵魂火
    const BlockState* belowStatePtr = world.getBlockState(pos.down());

    if (belowStatePtr != nullptr && BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*belowStatePtr)) {
        // 灵魂火基座方块上放置灵魂火
        return VanillaBlocks::SOUL_FIRE;
    }

    // 其他情况放置普通火
    return VanillaBlocks::FIRE;
}

} // namespace tool
} // namespace item
} // namespace mc
