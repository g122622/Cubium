#pragma once

#include "../../core/Item.hpp"
#include "../../../core/Types.hpp"

namespace mc {

// 前向声明
namespace fluid {
class Fluid;
class FluidState;
}

/**
 * @brief 桶物品
 *
 * 实现水桶、岩浆桶、空桶的功能。
 * - 空桶：从水源方块或含水方块中取出流体
 * - 装满的桶：放置流体方块或向含水方块注入流体
 *
 * 参考 MC 1.16.5: net.minecraft.item.BucketItem
 */
class BucketItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param containedFluid 桶中装的流体（nullptr表示空桶）
     * @param properties 物品属性
     */
    BucketItem(
        fluid::Fluid* containedFluid,
        const ItemProperties& properties);

    ~BucketItem() override = default;

    /**
     * @brief 获取桶中装的流体
     * @return 流体指针，空桶返回 nullptr
     */
    [[nodiscard]] fluid::Fluid* getContainedFluid() const { return m_containedFluid; }

    /**
     * @brief 检查是否为空桶
     */
    [[nodiscard]] bool isEmpty() const { return m_containedFluid == nullptr; }

    /**
     * @brief 在方块上使用物品
     *
     * 对于空桶：尝试从方块中取出流体
     * 对于装满的桶：尝试放置流体或向含水方块注水
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用物品
     *
     * 处理桶的交互逻辑。
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 获取填充后的桶物品
     *
     * 当空桶装满流体后，返回对应的桶物品。
     *
     * @param fluid 流体
     * @return 对应的桶物品，如果流体无效则返回 nullptr
     */
    [[nodiscard]] static BucketItem* getFilledBucket(fluid::Fluid& fluid);

    /**
     * @brief 获取空桶物品
     */
    [[nodiscard]] static BucketItem* getEmptyBucket();

protected:
    /**
     * @brief 尝试放置流体
     *
     * @param player 玩家
     * @param world 世界
     * @param pos 放置位置
     * @param hit 射线检测结果
     * @return 是否成功放置
     */
    bool tryPlaceContainedLiquid(
        Player* player,
        IWorld& world,
        const BlockPos& pos,
        const BlockRaycastResult& hit);

    /**
     * @brief 检查方块是否可以容纳流体
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @return 是否可以容纳流体
     */
    bool canBlockContainFluid(
        IWorld& world,
        const BlockPos& pos,
        const BlockState& state) const;

private:
    fluid::Fluid* m_containedFluid;
};

} // namespace mc
