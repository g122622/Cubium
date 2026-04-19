#include "BlockItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/block/Material.hpp"

namespace mc {

BlockItem::BlockItem(const Block& block, ItemProperties properties)
    : Item(properties)
    , m_block(&block)
{
}

bool BlockItem::tryPlace(BlockItemUseContext& context) const
{
    // 检查是否可以放置
    if (!context.canPlace()) {
        return false;
    }

    // 获取放置状态
    const BlockState* state = getStateForPlacement(context);
    if (state == nullptr) {
        return false;
    }

    // 检查状态是否可以放置
    if (!canPlace(context, *state)) {
        return false;
    }

    // 执行放置
    return placeBlock(context, state);
}

const BlockState* BlockItem::getStateForPlacement(const BlockItemUseContext& /* context */) const
{
    // 默认实现返回方块的默认状态
    // 子类可以重写以支持有方向的方块（如楼梯、门等）
    return &m_block->defaultState();
}

bool BlockItem::canPlace(const BlockItemUseContext& context, const BlockState& /* state */) const
{
    // 检查位置是否有效
    if (!checkPositionValid(context)) {
        return false;
    }

    // 检查放置位置是否可替换
    const BlockPos& pos = context.placementPos();
    if (!context.world().isWithinWorldBounds(pos)) {
        return false;
    }

    // 获取当前方块
    const BlockState* currentState = context.world().getBlockState(pos);
    if (currentState != nullptr && !currentState->isAir()) {
        // 检查材质是否可替换
        const Material& material = currentState->owner().material();
        if (!material.isReplaceable() && !material.isLiquid()) {
            return false;
        }
    }

    return true;
}

bool BlockItem::checkPositionValid(const BlockItemUseContext& context) const
{
    const BlockPos& pos = context.placementPos();

    // TODO: 检查世界边界
    // Y 范围通常在 0-255 或 0-383（取决于世界类型）
    // 这里使用简单的检查，实际应该从世界获取限制
    // 正确做法是使用world::MIN_BUILD_HEIGHT和world::MAX_BUILD_HEIGHT
    if (pos.y < 0 || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    return true;
}

bool BlockItem::placeBlock(BlockItemUseContext& /* context */, const BlockState* state) const
{
    if (state == nullptr) {
        return false;
    }

    // 注意：实际放置方块的逻辑需要由调用者实现
    // BlockItem 只负责验证和状态计算
    // 这里的设计是为了分离只读检查和写入操作

    // 放置成功后的标记
    // 实际的世界修改由调用者执行
    return true;
}

} // namespace mc
