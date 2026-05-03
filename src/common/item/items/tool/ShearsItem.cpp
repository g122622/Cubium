#include "ShearsItem.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../core/Types.hpp"

namespace mc {
namespace item {
namespace tool {

ShearsItem::ShearsItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

f32 ShearsItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    (void)stack;

    // MC 1.16.5: 对蜘蛛网和树叶返回 15.0
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return 15.0f;
    }

    // 使用 BlockTags 检查树叶
    if (BlockTags::LEAVES().contains(state)) {
        return 15.0f;
    }

    // MC 1.16.5: 对羊毛返回 5.0
    if (BlockTags::WOOL().contains(state)) {
        return 5.0f;
    }

    // 其他方块返回基础速度
    return 1.0f;
}

bool ShearsItem::canHarvestBlock(const BlockState& state) const {
    // MC 1.16.5: 剪刀可以采集蜘蛛网、红石线、绊线
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return true;
    }
    if (VanillaBlocks::REDSTONE_WIRE && &state.owner() == VanillaBlocks::REDSTONE_WIRE) {
        return true;
    }
    if (VanillaBlocks::TRIPWIRE && &state.owner() == VanillaBlocks::TRIPWIRE) {
        return true;
    }

    // 其他方块使用默认采集规则
    return state.getHarvestTool() == TOOL_TYPE_NONE;
}

bool ShearsItem::onBlockDestroyed(ItemStack& stack,
                                   IWorld& world,
                                   const BlockState& state,
                                   const BlockPos& pos,
                                   LivingEntity& entity) {
    (void)world;
    (void)pos;
    (void)entity;

    // MC 1.16.5: 如果方块硬度 > 0 且不是火，消耗耐久
    // 注意：火方块不消耗耐久
    if (state.hardness() > 0.0f) {
        // 检查是否是火方块（使用 BlockTags）
        if (BlockTags::FIRE().contains(state)) {
            return true;  // 火不消耗耐久
        }
        stack.attemptDamageItem(1);
    }
    return true;
}

bool ShearsItem::itemInteractionForEntity(ItemStack& stack,
                                           Player& player,
                                           LivingEntity& target,
                                           Hand hand) {
    (void)player;
    (void)hand;

    // MC 1.16.5: 剪羊毛逻辑
    // 参考: ShearsItem.itemInteractionForEntity
    // 如果实体实现 IForgeShearable 接口，调用其 onSheared 方法
    //
    // 目前项目中的羊实体需要实现 IForgeShearable 或类似接口
    // 简化实现：检查实体类型是否为羊
    //
    // TODO: 当 SheepEntity 实现 IShearable 接口后，完善此逻辑
    // 目前返回 false 表示未处理
    (void)stack;
    (void)target;

    return false;
}

} // namespace tool
} // namespace item
} // namespace mc
