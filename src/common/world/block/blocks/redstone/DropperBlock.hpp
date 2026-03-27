#pragma once

#include "DispenserBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 投掷器方块
 *
 * 投掷器简单地投掷物品，没有特殊行为。
 *
 * ## 特性
 * - 9格存储空间
 * - 红石激活时随机投掷物品
 * - 向容器输出物品
 * - 方向性：可向6个方向投掷
 *
 * ## 与发射器的区别
 * - 投掷器只投掷物品，没有特殊行为
 * - 发射器对特定物品有特殊行为（如箭矢）
 *
 * 参考: net.minecraft.block.DropperBlock
 */
class DropperBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DropperBlock(const BlockProperties& properties);

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

    // ========== 投掷器特有方法 ==========

    /**
     * @brief 检查投掷器是否被触发
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
     * @brief 获取投掷器朝向
     *
     * @param state 方块状态
     * @return Direction 投掷方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 投掷物品
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void drop(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /**
     * @brief 尝试从投掷器位置投掷物品
     *
     * @param world 世界引用
     * @param pos 投掷器位置
     * @param state 当前方块状态
     * @return true 如果成功投掷
     */
    bool tryDrop(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 播放投掷音效
     *
     * @param world 世界引用
     * @param pos 方块位置
     */
    void playDropSound(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
