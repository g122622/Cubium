#pragma once

#include "../../core/Item.hpp"

// Forward declarations
namespace mc {
namespace math {
class IRandom;
}
class Player;
class IWorld;
class BlockPos;
class BlockState;
class ItemStack;
}

namespace mc {
namespace item::items {

/**
 * @brief 骨粉物品
 *
 * 骨粉是一种特殊的物品，可以用于加速植物生长。
 * 主要功能：
 * 1. 对 IGrowable 方块使用：加速生长
 * 2. 在水下使用：生成海草
 *
 * 参考: net.minecraft.item.BoneMealItem
 */
class BoneMealItem : public Item {
public:
    /**
     * @brief 构造骨粉物品
     * @param properties 物品属性
     */
    explicit BoneMealItem(ItemProperties properties);

    ~BoneMealItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 对植物使用骨粉，加速生长。
     * 成功时消耗一个骨粉并生成快乐村民粒子。
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 应用骨粉效果
     *
     * 静态方法，可直接调用以应用骨粉效果。
     * 检查目标方块是否实现 IGrowable，如果是则调用 grow()。
     *
     * @param stack 物品堆
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家（可为nullptr）
     * @return 是否成功应用
     */
    static bool applyBonemeal(ItemStack& stack, IWorld& world, const BlockPos& pos, Player* player);

    /**
     * @brief 在水下生成海草
     *
     * 在水下使用骨粉时，有概率在周围生成海草。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param random 随机数生成器
     * @return 是否成功生成
     */
    static bool growSeagrass(IWorld& world, const BlockPos& pos, math::IRandom& random);

private:
    /**
     * @brief 生成快乐村民粒子效果
     *
     * @param world 世界
     * @param pos 方块位置
     */
    static void spawnBonemealParticles(IWorld& world, const BlockPos& pos);
};

} // namespace item::items
} // namespace mc
