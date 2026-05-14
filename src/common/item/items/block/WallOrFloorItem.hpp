#pragma once

#include "../../../world/block/Block.hpp"
#include "BlockItem.hpp"
#include <vector>

namespace mc {

/**
 * @brief 墙壁或地板放置物品
 *
 * 用于可以在地板上放置或在墙上放置的物品（如告示牌、旗帜、头颅等）。
 * 根据点击位置自动选择放置在地板上还是墙上。
 *
 * 参考: net.minecraft.item.WallOrFloorItem
 */
class WallOrFloorItem : public BlockItem {
public:
    /**
     * @brief 构造墙壁或地板放置物品
     * @param floorBlock 地板方块（放在地上）
     * @param wallBlock 墙壁方块（贴在墙上）
     * @param properties 物品属性
     */
    WallOrFloorItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties);

    ~WallOrFloorItem() override = default;

    /**
     * @brief 获取墙壁方块
     * @return 墙壁方块引用
     */
    [[nodiscard]] const Block& wallBlock() const { return *m_wallBlock; }

protected:
    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家视线的最近方向，决定放置地板方块还是墙壁方块。
     * 如果方向是 DOWN，尝试放置地板方块。
     * 如果方向是水平方向，尝试放置墙壁方块。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不能放置返回 nullptr
     */
    [[nodiscard]] const BlockState* getStateForPlacement(const BlockItemUseContext& context) const override;

private:
    const Block* m_wallBlock;
};

} // namespace mc
