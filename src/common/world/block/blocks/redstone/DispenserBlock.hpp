#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 发射器方块
 *
 * 发射器可以发射物品，对特定物品有特殊行为。
 *
 * ## 特性
 * - 9格存储空间
 * - 红石激活时随机发射物品
 * - 特殊行为：箭矢、火焰弹、TNT等
 * - 方向性：可向6个方向发射
 *
 * ## 容易踩的坑
 * - 发射器朝向和发射方向的关系
 * - 需要与 DispenserBlockEntity 配合
 * - 特殊物品行为处理
 *
 * 参考: net.minecraft.block.DispenserBlock
 */
class DispenserBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DispenserBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Normal;
    }

    // ========== 发射器特有方法 ==========

    /**
     * @brief 检查发射器是否被触发
     *
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isTriggered(const BlockState& state);

    /**
     * @brief 设置触发状态
     *
     * @param state 方块状态
     * @param triggered 是否触发
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withTriggered(BlockState state, bool triggered);

    /**
     * @brief 获取发射器朝向
     *
     * @param state 方块状态
     * @return Direction 发射方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 发射物品
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void dispense(IWorld& world, const BlockPos& pos, const BlockState& state);

protected:
    /**
     * @brief 尝试从发射器位置发射物品
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param state 当前方块状态
     * @return true 如果成功发射
     */
    bool tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 播放发射音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     */
    void playDispenseSound(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
