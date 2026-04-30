#include "BlockItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/block/Material.hpp"
#include "../../../world/IWorld.hpp"

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

bool BlockItem::canPlace(const BlockItemUseContext& context, const BlockState& state) const
{
    // 检查位置是否有效
    if (!checkPositionValid(context)) {
        return false;
    }

    const BlockPos& pos = context.placementPos();

    // 检查放置位置是否在世界边界内
    if (!context.getWorld().isWithinWorldBounds(pos)) {
        return false;
    }

    // 获取当前方块
    const BlockState* currentState = context.getBlockStateAtPlacementPos();
    if (currentState != nullptr && !currentState->isAir()) {
        // 检查材质是否可替换
        const Material& material = currentState->owner().material();
        if (!material.isReplaceable() && !material.isLiquid()) {
            return false;
        }
    }

    // 关键：调用方块的 isValidPosition 检查植物放置条件等
    // 参考 MC 1.16.5: BlockItem.canPlace
    // 注意：IWorld 继承关系允许我们将 const IWorld& 转换为 IBlockReader&
    // 因为 isValidPosition 是只读操作，这里使用 const_cast 是安全的
    IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(context.getWorld()));
    if (!m_block->isValidPosition(state, blockReader, pos)) {
        return false;
    }

    // TODO: 实体碰撞检查
    // 参考 MC 1.16.5: world.func_226663_a_(state, pos, ISelectionContext.dummy())
    // 这需要 ISelectionContext 实现

    return true;
}

bool BlockItem::checkPositionValid(const BlockItemUseContext& context) const
{
    const BlockPos& pos = context.placementPos();

    // 检查世界边界
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
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
