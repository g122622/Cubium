#pragma once

#include "../../core/Item.hpp"
#include "../../context/BlockItemUseContext.hpp"
#include "../../../world/block/Block.hpp"

namespace mc {

/**
 * @brief 方块物品类
 *
 * 继承自 Item 类，用于处理方块的放置。
 * 每个 BlockItem 与一个 Block 关联。
 *
 * 参考: net.minecraft.item.BlockItem
 */
class BlockItem : public Item {
public:
    /**
     * @brief 构造方块物品
     * @param block 关联的方块
     * @param properties 物品属性
     */
    BlockItem(const Block& block, ItemProperties properties);

    ~BlockItem() override = default;

    // ========== 基本属性 ==========

    /**
     * @brief 获取关联的方块
     */
    [[nodiscard]] const Block& block() const { return *m_block; }

    // ========== 放置逻辑 ==========

    /**
     * @brief 尝试放置方块
     *
     * 检查放置条件，获取放置状态，并执行放置。
     *
     * @param context 放置上下文
     * @return 是否放置成功
     */
    [[nodiscard]] bool tryPlace(BlockItemUseContext& context) const;

    /**
     * @brief 获取放置时的方块状态
     *
     * 子类可重写以支持有方向/状态的方块。
     * 默认实现返回方块的默认状态。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不能放置返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getStateForPlacement(const BlockItemUseContext& context) const;

    /**
     * @brief 检查是否可以在此位置放置
     *
     * 检查方块状态是否可以放置在指定位置。
     *
     * @param context 放置上下文
     * @param state 要放置的方块状态
     * @return 是否可以放置
     */
    [[nodiscard]] bool canPlace(const BlockItemUseContext& context, const BlockState& state) const;

protected:
    /**
     * @brief 执行方块放置
     *
     * 在世界中设置方块状态。
     * 子类可重写以添加额外效果（如播放声音）。
     *
     * @param context 放置上下文
     * @param state 要放置的方块状态
     * @return 是否放置成功
     */
    [[nodiscard]] virtual bool placeBlock(BlockItemUseContext& context, const BlockState* state) const;

    /**
     * @brief 检查放置位置是否有效
     *
     * 检查放置位置是否在方块碰撞范围内。
     *
     * @param context 放置上下文
     * @return 是否有效
     */
    [[nodiscard]] bool checkPositionValid(const BlockItemUseContext& context) const;

private:
    const Block* m_block;
};

} // namespace mc
