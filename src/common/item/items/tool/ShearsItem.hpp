#pragma once

#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../core/Types.hpp"
#include "../../../world/block/Material.hpp"
#include <unordered_set>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class IWorld;
class BlockPos;
class LivingEntity;
class Player;

namespace item {
namespace tool {

/**
 * @brief 剪刀物品
 *
 * 剪刀是一种特殊工具，用于：
 * - 剪羊毛（对羊使用）
 * - 高效破坏蜘蛛网、树叶、羊毛
 * - 采集蜘蛛网、红石线、绊线
 *
 * 参考: net.minecraft.item.ShearsItem
 */
class ShearsItem : public Item {
public:
    /**
     * @brief 构造剪刀
     * @param properties 物品属性（耐久度等）
     */
    explicit ShearsItem(ItemProperties properties);

    ~ShearsItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对蜘蛛网和树叶返回 15.0（高效率）
     * 对羊毛返回 5.0
     * 其他方块返回 1.0
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack,
                                       const BlockState& state) const override;

    /**
     * @brief 检查是否能采集方块
     *
     * 剪刀可以采集：蜘蛛网、红石线、绊线
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 破坏方块时调用
     *
     * 如果方块硬度 > 0 且不是火，消耗 1 点耐久度。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param state 被破坏的方块状态
     * @param pos 方块位置
     * @param entity 破坏者实体
     * @return 是否成功
     */
    bool onBlockDestroyed(ItemStack& stack,
                          IWorld& world,
                          const BlockState& state,
                          const BlockPos& pos,
                          LivingEntity& entity) override;

    /**
     * @brief 与实体交互
     *
     * 用于剪羊毛。如果实体实现 IForgeShearable 接口，
     * 调用其 onSheared 方法掉落物品。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack,
                                   Player& player,
                                   LivingEntity& target,
                                   Hand hand) override;
};

} // namespace tool
} // namespace item
} // namespace mc
